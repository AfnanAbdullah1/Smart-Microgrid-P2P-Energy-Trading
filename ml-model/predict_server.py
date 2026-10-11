"""
Smart Microgrid — AI Prediction Server
========================================

Flask API that loads the trained LSTM model and serves real-time
power consumption predictions to the Node.js backend.

Endpoints:
    GET  /health              →  Health check
    POST /predict             →  Predict from provided sensor readings
    GET  /predict/latest      →  Predict using latest sensor_log.csv data
    GET  /model/info          →  Model metadata and accuracy metrics
    POST /anomaly/detect      →  Detect anomalies in sensor readings

Runs on port 5000 alongside the Node.js backend on port 3000.

Usage:
    python predict_server.py
"""

import os, json, pickle, sys, time
import numpy as np
import pandas as pd
from datetime import datetime
from collections import deque

# ── Check dependencies ───────────────────────────────────────────
try:
    from flask import Flask, request, jsonify
    from flask_cors import CORS
except ImportError:
    print("✗ Flask not installed.  Run:  pip install flask flask-cors")
    sys.exit(1)

try:
    import tensorflow as tf
    tf.get_logger().setLevel('ERROR')  # Suppress TF warnings
except ImportError:
    print("✗ TensorFlow not installed.  Run:  pip install tensorflow")
    sys.exit(1)


# ==================== CONFIGURATION ====================

MODEL_DIR     = 'models'
MODEL_FILE    = os.path.join(MODEL_DIR, 'best_model.keras')
SCALER_FILE   = os.path.join(MODEL_DIR, 'scaler.pkl')
METADATA_FILE = os.path.join(MODEL_DIR, 'model_metadata.json')
CSV_FILE      = os.path.join('..', 'backend', 'data', 'sensor_log.csv')

PORT = 5000
HOST = '0.0.0.0'

# Prediction history (in-memory ring buffer)
MAX_HISTORY = 200
prediction_history = deque(maxlen=MAX_HISTORY)

# Anomaly detection thresholds
ANOMALY_POWER_HIGH = 9.5     # Unusual spike
ANOMALY_POWER_LOW  = -0.1    # Negative reading = sensor error
ANOMALY_VOLTAGE_HIGH = 250.0
ANOMALY_VOLTAGE_LOW  = 180.0
anomaly_log = deque(maxlen=100)

# Relay prediction hysteresis — prevents rapid ON/OFF toggling
# The relay stays in its current state unless the predicted power
# crosses OUTSIDE the dead-band [RELAY_ON_BELOW, RELAY_OFF_ABOVE].
RELAY_ON_BELOW   = 0.7   # predict < 0.7 W → switch to ON
RELAY_OFF_ABOVE  = 1.5   # predict > 1.5 W → switch to OFF
_last_relay_pred = 'ON'   # default starting state


# ==================== LOAD MODEL ====================

def load_model_and_scaler():
    """Load trained model, scaler, and metadata at startup."""
    print("\n" + "=" * 50)
    print("  Loading AI Model...")
    print("=" * 50)

    if not os.path.exists(MODEL_FILE):
        print(f"✗ Model file not found: {MODEL_FILE}")
        print("  Run:  python train_model.py  first")
        return None, None, None

    if not os.path.exists(SCALER_FILE):
        print(f"✗ Scaler file not found: {SCALER_FILE}")
        return None, None, None

    # Load model
    model = tf.keras.models.load_model(MODEL_FILE)
    print(f"✓ Model loaded: {MODEL_FILE}")
    print(f"  Input shape:  {model.input_shape}")
    print(f"  Output shape: {model.output_shape}")

    # Load scaler
    with open(SCALER_FILE, 'rb') as f:
        scaler = pickle.load(f)
    print(f"✓ Scaler loaded: {SCALER_FILE}")

    # Load metadata
    metadata = {}
    if os.path.exists(METADATA_FILE):
        with open(METADATA_FILE, 'r') as f:
            metadata = json.load(f)
        print(f"✓ Metadata loaded: {METADATA_FILE}")
        print(f"  Model version: {metadata.get('model_version', 'unknown')}")
        print(f"  Training MAE:  {metadata.get('training_metrics', {}).get('mae_watts', '?')} W")
    else:
        print("⚠ No metadata file — using defaults")
        metadata = {
            'sequence_length': 20,
            'feature_columns': [
                'houseA_voltage', 'houseA_current', 'houseA_power',
                'houseA_frequency', 'houseA_pf',
                'houseB_power', 'houseB_current',
                'relay_state', 'receiving_from_A',
                'hour', 'day_of_week'
            ],
            'target_column': 'houseA_power',
            'training_metrics': {}
        }

    print("=" * 50 + "\n")
    return model, scaler, metadata


# ==================== PREDICTION LOGIC ====================

def predict_power(model, scaler, metadata, input_data):
    """
    Run LSTM prediction on input sensor data.

    Args:
        input_data: list of dicts with sensor readings (at least seq_len rows)

    Returns:
        dict with prediction results
    """
    seq_len = metadata.get('sequence_length', 20)
    feature_cols = metadata.get('feature_columns', [])
    target_col = metadata.get('target_column', 'houseA_power')

    # Convert to DataFrame
    df = pd.DataFrame(input_data)

    # Ensure all feature columns exist
    for col in feature_cols:
        if col not in df.columns:
            df[col] = 0.0

    # Select features in correct order
    df = df[feature_cols].copy()
    df = df.ffill().bfill().fillna(0)

    if len(df) < seq_len:
        return {
            'error': f'Need at least {seq_len} rows, got {len(df)}',
            'success': False
        }

    # Take last seq_len rows
    data = df.iloc[-seq_len:].values.astype(np.float32)

    # Scale
    scaled = scaler.transform(data)

    # Reshape for LSTM: (1, seq_len, n_features)
    X = scaled.reshape(1, seq_len, len(feature_cols))

    # Predict
    pred_scaled = model.predict(X, verbose=0).flatten()[0]

    # Inverse transform to get real Watts
    target_idx = feature_cols.index(target_col)
    dummy = np.zeros((1, len(feature_cols)))
    dummy[0, target_idx] = pred_scaled
    pred_watts = scaler.inverse_transform(dummy)[0, target_idx]

    # Get current actual value for comparison
    actual_watts = float(df.iloc[-1][target_col])

    # Confidence — based on how close prediction is to recent trend
    recent_std = df[target_col].iloc[-seq_len:].std()
    confidence = max(0.0, min(1.0, 1.0 - (recent_std / 10.0)))

    # Relay prediction with hysteresis to avoid rapid toggling
    global _last_relay_pred
    if pred_watts < RELAY_ON_BELOW:
        _last_relay_pred = 'ON'
    elif pred_watts > RELAY_OFF_ABOVE:
        _last_relay_pred = 'OFF'
    # else: keep previous state (inside dead-band)

    relay_reason = (
        'Low consumption predicted → share energy'
        if _last_relay_pred == 'ON'
        else 'High consumption predicted → keep energy'
    )

    result = {
        'success': True,
        'predicted_power_watts': round(float(pred_watts), 3),
        'current_power_watts': round(actual_watts, 3),
        'confidence': round(confidence, 3),
        'relay_prediction': _last_relay_pred,
        'relay_reason': relay_reason,
        'timestamp': datetime.now().isoformat(),
    }

    # Store in history
    prediction_history.append({
        'timestamp': result['timestamp'],
        'predicted': result['predicted_power_watts'],
        'actual': result['current_power_watts'],
        'confidence': result['confidence'],
    })

    return result


def read_latest_csv(n_rows=100):
    """Read the last N rows from sensor_log.csv."""
    if not os.path.exists(CSV_FILE):
        return None

    # Read only the tail efficiently
    try:
        df = pd.read_csv(CSV_FILE)
        if len(df) < 20:
            return None
        return df.tail(n_rows)
    except Exception as e:
        print(f"Error reading CSV: {e}")
        return None


def detect_anomalies(data_dict):
    """Check sensor readings for anomalies."""
    anomalies = []
    timestamp = datetime.now().isoformat()

    power = data_dict.get('houseA_power', 0)
    voltage = data_dict.get('houseA_voltage', 220)
    current = data_dict.get('houseA_current', 0)

    if power > ANOMALY_POWER_HIGH:
        anomalies.append({
            'type': 'power_spike',
            'severity': 'warning',
            'message': f'Unusual power spike: {power:.1f}W (threshold: {ANOMALY_POWER_HIGH}W)',
            'value': power,
            'timestamp': timestamp,
        })

    if power < ANOMALY_POWER_LOW:
        anomalies.append({
            'type': 'negative_power',
            'severity': 'critical',
            'message': f'Negative power reading: {power:.2f}W — sensor error',
            'value': power,
            'timestamp': timestamp,
        })

    if voltage > ANOMALY_VOLTAGE_HIGH:
        anomalies.append({
            'type': 'overvoltage',
            'severity': 'critical',
            'message': f'Overvoltage detected: {voltage:.1f}V',
            'value': voltage,
            'timestamp': timestamp,
        })

    if 0 < voltage < ANOMALY_VOLTAGE_LOW:
        anomalies.append({
            'type': 'undervoltage',
            'severity': 'warning',
            'message': f'Undervoltage detected: {voltage:.1f}V',
            'value': voltage,
            'timestamp': timestamp,
        })

    # Store anomalies
    for a in anomalies:
        anomaly_log.append(a)

    return anomalies


# ==================== FLASK APP ====================

app = Flask(__name__)
CORS(app)

# Global model references
_model = None
_scaler = None
_metadata = None


@app.route('/health', methods=['GET'])
def health():
    """Health check endpoint."""
    return jsonify({
        'status': 'online',
        'service': 'Smart Microgrid AI Prediction Server',
        'model_loaded': _model is not None,
        'timestamp': datetime.now().isoformat(),
    })


@app.route('/predict', methods=['POST'])
def predict():
    """
    Predict next power consumption from provided sensor readings.

    Expects JSON body:
    {
        "readings": [
            {"houseA_voltage": 220, "houseA_current": 0.02, ...},
            ... (at least 20 readings)
        ]
    }
    """
    if _model is None:
        return jsonify({'success': False, 'error': 'Model not loaded'}), 503

    data = request.get_json()
    if not data or 'readings' not in data:
        return jsonify({'success': False, 'error': 'Missing "readings" array'}), 400

    result = predict_power(_model, _scaler, _metadata, data['readings'])
    return jsonify(result)


@app.route('/predict/latest', methods=['GET'])
def predict_latest():
    """Predict using the latest data from sensor_log.csv."""
    if _model is None:
        return jsonify({'success': False, 'error': 'Model not loaded'}), 503

    df = read_latest_csv(100)
    if df is None:
        return jsonify({
            'success': False,
            'error': 'No sensor data available. Run the system or generate data first.'
        }), 404

    readings = df.to_dict('records')
    result = predict_power(_model, _scaler, _metadata, readings)

    # Also run anomaly detection on latest reading
    if len(readings) > 0:
        anomalies = detect_anomalies(readings[-1])
        result['anomalies'] = anomalies

    return jsonify(result)


@app.route('/predict/history', methods=['GET'])
def predict_history():
    """Return recent prediction history."""
    limit = request.args.get('limit', 50, type=int)
    history = list(prediction_history)[-limit:]
    return jsonify({
        'success': True,
        'count': len(history),
        'history': history,
    })


@app.route('/model/info', methods=['GET'])
def model_info():
    """Return model metadata and accuracy metrics."""
    if _metadata is None:
        return jsonify({'success': False, 'error': 'No model metadata'}), 503

    info = {
        'success': True,
        'model_loaded': _model is not None,
        **_metadata,
        'prediction_history_size': len(prediction_history),
        'anomaly_log_size': len(anomaly_log),
    }

    # Calculate live accuracy from prediction history
    if len(prediction_history) > 10:
        recent = list(prediction_history)[-50:]
        actuals = [p['actual'] for p in recent]
        preds = [p['predicted'] for p in recent]
        live_mae = np.mean(np.abs(np.array(actuals) - np.array(preds)))
        info['live_accuracy'] = {
            'mae_watts': round(float(live_mae), 4),
            'samples': len(recent),
        }

    return jsonify(info)


@app.route('/anomaly/detect', methods=['POST'])
def anomaly_detect():
    """Detect anomalies in provided sensor reading."""
    data = request.get_json()
    if not data:
        return jsonify({'success': False, 'error': 'No data provided'}), 400

    anomalies = detect_anomalies(data)
    return jsonify({
        'success': True,
        'anomalies': anomalies,
        'is_anomalous': len(anomalies) > 0,
    })


@app.route('/anomaly/log', methods=['GET'])
def anomaly_get_log():
    """Return recent anomaly detections."""
    limit = request.args.get('limit', 20, type=int)
    log = list(anomaly_log)[-limit:]
    return jsonify({
        'success': True,
        'count': len(log),
        'anomalies': log,
    })


# ==================== STARTUP ====================

def main():
    global _model, _scaler, _metadata

    print("\n" + "=" * 50)
    print("  Smart Microgrid — AI Prediction Server")
    print("=" * 50)

    # Load model
    _model, _scaler, _metadata = load_model_and_scaler()

    if _model is None:
        print("\n⚠  Running in DEMO mode (no model loaded)")
        print("   Prediction endpoints will return errors")
        print("   Run:  python train_model.py  to train first\n")

    # Start Flask
    print(f"✓ Starting server on http://{HOST}:{PORT}")
    print(f"✓ Endpoints:")
    print(f"    GET  http://localhost:{PORT}/health")
    print(f"    POST http://localhost:{PORT}/predict")
    print(f"    GET  http://localhost:{PORT}/predict/latest")
    print(f"    GET  http://localhost:{PORT}/predict/history")
    print(f"    GET  http://localhost:{PORT}/model/info")
    print(f"    POST http://localhost:{PORT}/anomaly/detect")
    print(f"    GET  http://localhost:{PORT}/anomaly/log")
    print("=" * 50 + "\n")

    app.run(host=HOST, port=PORT, debug=False)


if __name__ == '__main__':
    main()
