"""
Smart Microgrid — Synthetic Sensor Data Generator
===================================================

Generates realistic 7-day sensor_log.csv data that matches exactly
what the backend server (server.js) would write from real ESP32 hardware.

The data models:
  • House A: 10W fixed solar generation (12V charger + HOPE H201 inverter)
  • PZEM-004T sensor readings with realistic noise patterns
  • Relay logic:  consumption < 1W → relay ON (share to House B)
  • Day/night consumption cycles with weekday/weekend variation
  • House B receives energy only when relay is ON

Usage:
    python generate_data.py                   # 7 days, default
    python generate_data.py --days 14         # 14 days
    python generate_data.py --interval 1.0    # 1-second intervals
"""

import os
import argparse
import numpy as np
import pandas as pd
from datetime import datetime, timedelta

# ==================== CONFIGURATION ====================

OUTPUT_DIR = os.path.join('..', 'backend', 'data')
OUTPUT_FILE = os.path.join(OUTPUT_DIR, 'sensor_log.csv')

# Must match exactly what server.js writes
CSV_COLUMNS = [
    'timestamp', 'hour', 'day_of_week',
    'houseA_voltage', 'houseA_current', 'houseA_power',
    'houseA_energy', 'houseA_frequency', 'houseA_pf',
    'houseB_voltage', 'houseB_current', 'houseB_power',
    'houseB_energy',
    'relay_state', 'receiving_from_A', 'solar_generation'
]

# Physical constants matching the real hardware
FIXED_GENERATION_WATTS = 10.0        # ~10W AC from inverter
NOMINAL_VOLTAGE       = 220.0        # Pakistan mains voltage
NOMINAL_FREQUENCY     = 50.0         # Pakistan mains frequency
RELAY_THRESHOLD_WATTS = 1.0          # < 1W → relay ON

# Noise levels (calibrated to real PZEM-004T behavior)
VOLTAGE_NOISE_STD  = 2.5             # ±5V swing around 220V
FREQUENCY_NOISE_STD = 0.15           # ±0.3Hz around 50Hz
CURRENT_NOISE_STD   = 0.002          # ±4mA noise on current
POWER_NOISE_STD     = 0.3            # ±0.6W noise on power


# ==================== CONSUMPTION PROFILES ====================

def get_base_consumption(hour: float, is_weekend: bool) -> float:
    """
    Returns expected House A consumption (Watts) based on time of day.
    
    This models a small residential load on the inverter output:
      - Night (23:00–05:00):  Very low — phone charger or standby (0–0.5W)
      - Early morning (05:00–07:00): Wake up, light use (1–3W)
      - Morning (07:00–10:00): Active use, breakfast (3–6W)
      - Midday (10:00–14:00): Moderate (2–5W), lower if away
      - Afternoon (14:00–17:00): Moderate-high (3–7W)
      - Evening (17:00–21:00): Peak — lighting, appliances (5–9W)  
      - Late evening (21:00–23:00): Winding down (2–4W)
    
    Weekend: slightly higher daytime, later wake-up
    """
    # Weekday profile
    if not is_weekend:
        profile = {
            (0, 5):    lambda: np.random.uniform(0.0, 0.5),
            (5, 7):    lambda: np.random.uniform(1.0, 3.0),
            (7, 10):   lambda: np.random.uniform(3.0, 6.0),
            (10, 14):  lambda: np.random.uniform(2.0, 5.0),
            (14, 17):  lambda: np.random.uniform(3.0, 7.0),
            (17, 21):  lambda: np.random.uniform(5.0, 9.0),
            (21, 24):  lambda: np.random.uniform(2.0, 4.0),
        }
    else:
        # Weekend — later wake-up, higher daytime usage
        profile = {
            (0, 7):    lambda: np.random.uniform(0.0, 0.5),
            (7, 9):    lambda: np.random.uniform(0.5, 2.0),
            (9, 12):   lambda: np.random.uniform(2.0, 5.0),
            (12, 15):  lambda: np.random.uniform(3.0, 7.0),
            (15, 18):  lambda: np.random.uniform(4.0, 8.0),
            (18, 22):  lambda: np.random.uniform(5.0, 9.5),
            (22, 24):  lambda: np.random.uniform(1.5, 3.5),
        }

    for (start, end), gen_fn in profile.items():
        if start <= hour < end:
            return gen_fn()
    return 0.3  # fallback


def add_temporal_smoothing(values: np.ndarray, window: int = 11) -> np.ndarray:
    """
    Apply Gaussian-like smoothing to prevent unrealistic jumps.
    Real consumption doesn't change 5W between adjacent 0.5s readings.
    """
    kernel = np.ones(window) / window
    padded = np.pad(values, (window // 2, window // 2), mode='edge')
    smoothed = np.convolve(padded, kernel, mode='valid')[:len(values)]
    return smoothed


# ==================== DATA GENERATION ====================

def generate_sensor_data(
    num_days: int = 7,
    interval_seconds: float = 0.5,
    start_date: datetime = None,
    seed: int = 42
) -> pd.DataFrame:
    """
    Generate a full multi-day sensor log.
    
    Args:
        num_days: Number of days to generate
        interval_seconds: Time between readings (0.5s matches real system)
        start_date: Starting timestamp
        seed: Random seed for reproducibility
    
    Returns:
        DataFrame matching the sensor_log.csv schema
    """
    np.random.seed(seed)
    
    if start_date is None:
        # Start 7 days ago so data looks "recent"
        start_date = datetime.now() - timedelta(days=num_days)
    
    total_seconds = num_days * 24 * 3600
    num_rows = int(total_seconds / interval_seconds)
    
    print(f"\n{'='*55}")
    print(f"  Synthetic Data Generator - Smart Microgrid")
    print(f"{'='*55}")
    print(f"  Days:       {num_days}")
    print(f"  Interval:   {interval_seconds}s")
    print(f"  Total rows: {num_rows:,}")
    print(f"  Start:      {start_date.strftime('%Y-%m-%d %H:%M')}")
    print(f"  End:        {(start_date + timedelta(days=num_days)).strftime('%Y-%m-%d %H:%M')}")
    print(f"{'='*55}\n")
    
    # ── Generate timestamps ──────────────────────────────────────
    timestamps = [start_date + timedelta(seconds=i * interval_seconds)
                  for i in range(num_rows)]
    hours      = [t.hour + t.minute / 60.0 for t in timestamps]
    day_of_week = [t.weekday() for t in timestamps]  # 0=Mon, 6=Sun
    int_hours   = [t.hour for t in timestamps]
    
    # ── Generate House A consumption ─────────────────────────────
    print("  Generating House A consumption profile...")
    raw_consumption = np.array([
        get_base_consumption(h, dow in [5, 6])
        for h, dow in zip(hours, day_of_week)
    ])
    
    # Smooth to prevent unrealistic jumps (real loads change gradually)
    smooth_window = max(3, int(10 / interval_seconds))  # 10 seconds window
    houseA_power = add_temporal_smoothing(raw_consumption, smooth_window)
    
    # Add realistic PZEM noise
    houseA_power += np.random.normal(0, POWER_NOISE_STD, num_rows)
    houseA_power = np.clip(houseA_power, 0.0, FIXED_GENERATION_WATTS)
    
    # ── Derive other House A sensor readings ─────────────────────
    houseA_voltage   = NOMINAL_VOLTAGE + np.random.normal(0, VOLTAGE_NOISE_STD, num_rows)
    houseA_voltage   = np.clip(houseA_voltage, 210.0, 235.0)
    
    houseA_current   = houseA_power / houseA_voltage
    houseA_current  += np.random.normal(0, CURRENT_NOISE_STD, num_rows)
    houseA_current   = np.clip(houseA_current, 0.0, 0.1)
    
    houseA_frequency = NOMINAL_FREQUENCY + np.random.normal(0, FREQUENCY_NOISE_STD, num_rows)
    houseA_frequency = np.clip(houseA_frequency, 49.5, 50.5)
    
    # Power factor — higher when power is higher, lower at very low loads
    houseA_pf = np.where(
        houseA_power > 1.0,
        np.clip(0.85 + np.random.normal(0, 0.05, num_rows), 0.70, 1.00),
        np.clip(0.30 + np.random.normal(0, 0.10, num_rows), 0.10, 0.60)
    )
    
    # Cumulative energy (kWh) — running integral of power over time
    houseA_energy_wh = np.cumsum(houseA_power * interval_seconds / 3600.0)
    houseA_energy    = houseA_energy_wh / 1000.0  # convert to kWh
    
    # ── Relay logic ────────────────────────────────────────────────
    print("  Computing relay state decisions...")
    relay_state = (houseA_power < RELAY_THRESHOLD_WATTS).astype(int)
    
    # Add relay hysteresis — relay doesn't toggle every 0.5s in reality
    # It stays ON for at least 30 seconds, OFF for at least 10 seconds
    min_on_samples  = int(30 / interval_seconds)
    min_off_samples = int(10 / interval_seconds)
    
    smoothed_relay = np.copy(relay_state)
    i = 1
    last_change = 0
    while i < num_rows:
        if smoothed_relay[i] != smoothed_relay[i - 1]:
            since_last = i - last_change
            min_hold = min_on_samples if smoothed_relay[i - 1] == 1 else min_off_samples
            if since_last < min_hold:
                smoothed_relay[i] = smoothed_relay[i - 1]
            else:
                last_change = i
        i += 1
    relay_state = smoothed_relay
    
    # ── House B readings (only meaningful when relay is ON) ───────
    print("  Generating House B consumption data...")
    receiving_from_A = relay_state.copy()
    
    # When relay is ON: House B gets excess power through PZEM #2
    excess_power = np.clip(FIXED_GENERATION_WATTS - houseA_power, 0, FIXED_GENERATION_WATTS)
    
    houseB_power   = np.where(relay_state == 1, excess_power, 0.0)
    houseB_power  += np.where(relay_state == 1,
                              np.random.normal(0, 0.2, num_rows), 0.0)
    houseB_power   = np.clip(houseB_power, 0.0, FIXED_GENERATION_WATTS)
    
    houseB_voltage = np.where(
        relay_state == 1,
        NOMINAL_VOLTAGE + np.random.normal(0, VOLTAGE_NOISE_STD, num_rows),
        0.0  # No voltage when relay is OFF
    )
    houseB_voltage = np.clip(houseB_voltage, 0.0, 235.0)
    
    houseB_current = np.where(
        relay_state == 1,
        houseB_power / np.where(houseB_voltage > 0, houseB_voltage, 1.0),
        0.0
    )
    houseB_current = np.clip(houseB_current, 0.0, 0.1)
    
    # Cumulative energy for House B
    houseB_energy_wh = np.cumsum(houseB_power * interval_seconds / 3600.0)
    houseB_energy    = houseB_energy_wh / 1000.0
    
    # ── Introduce occasional anomalies (for anomaly detection demo) ──
    print("  Injecting anomaly events...")
    num_anomalies = num_days * 3  # ~3 anomalies per day
    anomaly_indices = np.random.choice(num_rows, num_anomalies, replace=False)
    for idx in anomaly_indices:
        # Spike or drop lasting 5–20 readings
        spike_len = np.random.randint(5, min(20, num_rows - idx))
        spike_type = np.random.choice(['spike', 'drop'])
        if spike_type == 'spike':
            houseA_power[idx:idx+spike_len] = np.random.uniform(8.5, 10.0)
        else:
            houseA_power[idx:idx+spike_len] = np.random.uniform(0.0, 0.1)
    
    # ── Assemble DataFrame ───────────────────────────────────────
    print("  Assembling DataFrame...")
    df = pd.DataFrame({
        'timestamp':        [t.isoformat() for t in timestamps],
        'hour':             int_hours,
        'day_of_week':      [t.weekday() for t in timestamps],
        'houseA_voltage':   np.round(houseA_voltage, 2),
        'houseA_current':   np.round(houseA_current, 4),
        'houseA_power':     np.round(houseA_power, 2),
        'houseA_energy':    np.round(houseA_energy, 4),
        'houseA_frequency': np.round(houseA_frequency, 1),
        'houseA_pf':        np.round(houseA_pf, 3),
        'houseB_voltage':   np.round(houseB_voltage, 2),
        'houseB_current':   np.round(houseB_current, 4),
        'houseB_power':     np.round(houseB_power, 2),
        'houseB_energy':    np.round(houseB_energy, 4),
        'relay_state':      relay_state,
        'receiving_from_A': receiving_from_A,
        'solar_generation': 10  # Fixed
    })
    
    return df


# ==================== STATISTICS ====================

def print_statistics(df: pd.DataFrame):
    """Print summary statistics about the generated data."""
    print(f"\n{'='*55}")
    print(f"  Generated Data Statistics")
    print(f"{'='*55}")
    print(f"  Total rows:       {len(df):,}")
    print(f"  Time span:        {df['timestamp'].iloc[0][:19]} -> {df['timestamp'].iloc[-1][:19]}")
    print(f"  ")
    print(f"  House A Power:")
    print(f"    Mean:           {df['houseA_power'].mean():.2f} W")
    print(f"    Std:            {df['houseA_power'].std():.2f} W")
    print(f"    Min:            {df['houseA_power'].min():.2f} W")
    print(f"    Max:            {df['houseA_power'].max():.2f} W")
    print(f"  ")
    print(f"  Relay State:")
    relay_on_pct = df['relay_state'].mean() * 100
    print(f"    ON:             {relay_on_pct:.1f}% of the time")
    print(f"    OFF:            {100 - relay_on_pct:.1f}% of the time")
    print(f"  ")
    print(f"  House B Power (when receiving):")
    hb_on = df[df['relay_state'] == 1]['houseB_power']
    if len(hb_on) > 0:
        print(f"    Mean:           {hb_on.mean():.2f} W")
        print(f"    Total received: {df['houseB_energy'].iloc[-1]:.4f} kWh")
    print(f"  ")
    print(f"  House A Energy:")
    print(f"    Total consumed: {df['houseA_energy'].iloc[-1]:.4f} kWh")
    print(f"{'='*55}")


# ==================== MAIN ====================

def main():
    parser = argparse.ArgumentParser(
        description='Generate synthetic sensor data for Smart Microgrid ML training'
    )
    parser.add_argument('--days', type=int, default=7,
                        help='Number of days to generate (default: 7)')
    parser.add_argument('--interval', type=float, default=0.5,
                        help='Interval between readings in seconds (default: 0.5)')
    parser.add_argument('--seed', type=int, default=42,
                        help='Random seed for reproducibility (default: 42)')
    parser.add_argument('--output', type=str, default=OUTPUT_FILE,
                        help=f'Output CSV path (default: {OUTPUT_FILE})')
    args = parser.parse_args()
    
    # Generate data
    df = generate_sensor_data(
        num_days=args.days,
        interval_seconds=args.interval,
        seed=args.seed
    )
    
    # Print statistics
    print_statistics(df)
    
    # Save to CSV
    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    df.to_csv(args.output, index=False)
    
    file_size = os.path.getsize(args.output) / (1024 * 1024)
    print(f"\n  [OK] Saved to: {os.path.abspath(args.output)}")
    print(f"  [OK] File size: {file_size:.1f} MB")
    print(f"  [OK] Ready for training: python train_model.py\n")


if __name__ == '__main__':
    main()
