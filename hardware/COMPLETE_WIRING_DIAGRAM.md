# Complete Wiring Diagram - Fixed Generation Model (2-PZEM System)

## System Overview

Your system has:
- **2 ESP32 boards** (one for each house)
- **2 PZEM-004T sensors** (House A consumption, House B consumption)
- **1 Relay module** (controls energy transfer)
- **1 DC Power Supply** (12V 1A charger, assumed as fixed ~10W generation via HOPE H201 inverter)
- **1 1602A LCD with I2C** (House A status display, SDA=GPIO21, SCL=GPIO22)
- **2 Buck converters** (step down voltage for ESP32s)

---

## Complete System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      MAIN AC SUPPLY (WAPDA)                  │
└───────────────┬─────────────────────────────────────────────┘
                │
        ┌───────┴────────┐
        │                │
        ▼                ▼
    ┌─────────┐    ┌─────────┐
    │ HOUSE A │────│  RELAY  │───┐  
    │  Load   │    └─────────┘   │
    │ (< 1W)  │                  │
    └─────────┘                  │
        │                        │
        │ (Through CT)           │
        ▼                        ▼
    ┌─────────┐            ┌─────────┐
    │ PZEM #1 │            │ HOUSE B │
    │ (House  │            │  Load   │
    │  A      │            └─────────┘
    │Consume) │                  │
    └─────────┘                  │ (Through CT)
        │                        ▼
        ▼                    ┌─────────┐
    ┌─────────┐              │ PZEM #2 │
    │ ESP32 A │              │ (House  │
    │         │◄─────────────┤  B      │
    └─────────┘              │Consume) │
                             └─────────┘
                                 │
                                 ▼
                             ┌─────────┐
                             │ ESP32 B │
                             └─────────┘

DC Power Supply (12V 1A Charger) → HOPE H201 Inverter → 220V AC (Assumed Generation: ~10W)
```

---

## Key Concept: Fixed Generation Model

> [!IMPORTANT]
> **No PZEM for Generation Measurement**
> 
> Unlike traditional solar setups, this system does NOT measure generation in real-time. Instead:
> - **Fixed Assumption:** ~10W (1A at 220V) is assumed as available generation
> - **Threshold Logic:** If House A consumes < 1W, relay turns ON
> - **Excess Calculation:** Excess = ~10W - House A Consumption

---

## House A (Energy Provider) - ESP32 #1

### Hardware Setup

**Components:**
- ESP32 Development Board
- PZEM-004T #1 (CT sensor for House A consumption)
- Relay Module (1-channel, 5V)
- Buck Converter (12V/24V AC → 5V DC)
- AC loads (bulbs, appliances for House A)

### Wiring Connections

#### PZEM #1 → House A Consumption Measurement

**PZEM-004T Pin Connections:**
```
PZEM #1 (House A Consumption):
├─ VCC   → 5V (from ESP32 or external 5V)
├─ GND   → GND (common ground)
├─ TX    → GPIO 18 (ESP32 RX)
└─ RX    → GPIO 19 (ESP32 TX)
```

**Current Transformer (CT) Clamp:**
```
House A AC Load → Live Wire passes through CT Coil
                → CT Output → PZEM CT Input
```

> [!WARNING]
> **CRITICAL:** Only the **LIVE wire** (L) goes through the CT clamp, NOT the neutral!

---

#### Relay Module Wiring

**Relay Control:**
```
Relay Module:
├─ VCC   → 5V (ESP32)
├─ GND   → GND (common ground)
└─ IN    → GPIO 27 (ESP32 control signal)
```

**Relay AC Side (High Voltage):**
```
Main AC Supply:
├─ Live (L)     → Relay COM (Common)
├─ Neutral (N)  → Directly to House B

Relay Connections:
├─ COM → Main Supply Live Wire
├─ NO  → House B Live Wire (Normally Open - energy flows when relay ON)
└─ NC  → Not connected
```

> [!CAUTION]
> **High Voltage!** Relay AC side carries 220V. Ensure proper insulation and connections.

---

#### ESP32 Power Supply

```
Main AC (220V) → Buck Converter → 5V DC → ESP32 VIN + GND
```

---

### House A Complete Pin Configuration

| Component | ESP32 Pin | Description |
|-----------|-----------|-------------|
| PZEM #1 TX | GPIO 18 | House A consumption data (RX) |
| PZEM #1 RX | GPIO 19 | House A consumption control (TX) |
| Relay IN | GPIO 27 | Relay control signal |
| Built-in LED | GPIO 2 | Status indicator |
| Power | VIN (5V) | From buck converter |
| Ground | GND | Common ground |

---

## House B (Consumer) - ESP32 #2

### Hardware Setup

**Components:**
- ESP32 Development Board
- PZEM-004T #2 (CT sensor for House B consumption)
- Buck Converter (AC → 5V DC)
- AC loads (bulbs, appliances for House B)

### Wiring Connections

#### PZEM #2 → House B Consumption Measurement

**PZEM-004T Pin Connections:**
```
PZEM #2 (House B Consumption):
├─ VCC   → 5V (from ESP32 or external 5V)
├─ GND   → GND (common ground)
├─ TX    → GPIO 16 (ESP32 RX)
└─ RX    → GPIO 17 (ESP32 TX)
```

**Current Transformer (CT) Clamp:**
```
House B AC Load → Live Wire passes through CT Coil
                → CT Output → PZEM CT Input
```

---

#### ESP32 Power Supply

```
House B AC → Buck Converter → 5V DC → ESP32 VIN + GND
```

---

### House B Complete Pin Configuration

| Component | ESP32 Pin | Description |
|-----------|-----------|-------------|
| PZEM #2 TX | GPIO 16 | House B consumption data (RX) |
| PZEM #2 RX | GPIO 17 | House B consumption control (TX) |
| Built-in LED | GPIO 2 | Status indicator |
| Power | VIN (5V) | From buck converter |
| Ground | GND | Common ground |

---

## Power Distribution Flow

### Normal Operation (House A ≥ 1W consumption):

```
Main AC Supply
    │
    ├──> House A Loads (consuming ≥ 1W)
    │    └──> Measured by PZEM #1
    │
    └──> Relay OFF
         └──> House B receives NO power from House A
              (House B uses its own grid connection)
```

### Energy Sharing (House A < 1W consumption):

```
Main AC Supply
    │
    ├──> House A Loads (consuming < 1W)
    │    └──> Measured by PZEM #1
    │
    └──> Relay ON (ESP32 A controls)
         └──> House B receives power
              └──> Measured by PZEM #2
```

---

## Setup Steps

### Step 1: Prepare PZEM Sensors

1. **PZEM #1 (House A):**
   - Attach CT clamp to House A **live wire**
   - Connect power (5V)
   - Wire TX/RX to ESP32 #1

2. **PZEM #2 (House B):**
   - Attach CT clamp to House B **live wire**
   - Connect power (5V)
   - Wire TX/RX to ESP32 #2

---

### Step 2: Connect Relay Module

1. Connect relay module to ESP32 #1 (House A)
   - VCC → 5V
   - GND → GND
   - IN → GPIO 27

2. Wire AC side:
   - Main supply LIVE → Relay COM
   - Relay NO → House B supply

> [!WARNING]
> Turn OFF main power when wiring AC connections!

---

### Step 3: Power ESP32 Boards

1. Connect buck converters:
   - Input: 220V AC (from main supply)
   - Output: 5V DC
   
2. Wire to ESP32:
   - Buck converter + → ESP32 VIN
   - Buck converter - → ESP32 GND

---

### Step 4: Ground All Components

Ensure common ground for:
- Both ESP32s
- Both PZEM sensors
- Relay module
- Buck converters

```
All GND pins → Common Ground Rail
```

---

## Threshold Configuration

The system activates relay based on **House A consumption threshold**:

```cpp
// In firmware/house_a_controller.ino
const float CONSUMPTION_THRESHOLD_WATTS = 1.0;     // 1W
const float CONSUMPTION_THRESHOLD_AMPS = 0.0;      // ~0A
const float FIXED_GENERATION_WATTS = 10.0;         // ~10W (0.045A × 220V)
```

**Logic:**
```
IF House A Consumption < 1W:
    Excess = 10W - Consumption
    Relay = ON (LOW)
    House B receives power

ELSE:
    Relay = OFF (HIGH)
```

---

## Testing Procedure

### 1. Test PZEMs Individually

**Connect ESP32 to PC via USB, open Serial Monitor:**

```
✓ PZEM #1 (House A Consumption) OK
📊 Fixed Generation Configuration:
   Assumed Generation: 0.045A (~10W)
   Consumption Threshold: 1.0W
```

---

### 2. Test House A Consumption

Turn on House A loads:
- Small load (< 1W): **Relay should turn ON**
- Load ≥ 1W: **Relay should turn OFF**

---

### 3. Test Relay Switching

Monitor ESP32 serial output:
```
🔋 LOW CONSUMPTION DETECTED - Starting transfer to House B
   House A Consumption: 0.0W (0.000A)
   Excess Available: 220.0W
✓ Relay ON - Energy flowing to House B
```

---

### 4. Verify House B Readings

House B should show power consumption when relay is ON.

---

## Troubleshooting

| Problem | Possible Cause | Solution |
|---------|----------------|----------|
| PZEM not responding | Wrong TX/RX pins | Double-check GPIO 18/19 (House A) and 16/17 (House B) |
| Relay not switching | Wrong control pin | Verify GPIO 27 for relay |
| No power to House B | Relay wiring | Check AC wiring: COM and NO connections |
| Wrong current readings | CT orientation | Ensure only LIVE wire passes through CT |
| ESP32 resets | Power insufficient | Use proper 5V/2A buck converter |

---

## Safety Warnings

> [!CAUTION]
> **Electrical Safety Critical!**
> 
> - ⚠️ Turn OFF main power before any AC wiring
> - ⚠️ Use proper wire gauges (14 AWG or thicker for 5A+)
> - ⚠️ Secure all AC connections with electrical tape
> - ⚠️ Test relay with LOW power loads first
> - ⚠️ Never touch AC components while powered
> - ⚠️ Use electrical enclosures for final installation

---

## Final System Features

✅ **Fixed Generation Assumption:** 0.045A (~10W)  
✅ **2 PZEMs:** House A and House B consumption monitoring  
✅ **Automatic Relay Control:** Based on 1W threshold  
✅ **Blockchain Integration:** Transactions recorded immutably  
✅ **Web Dashboard:** Real-time monitoring at localhost:3000  
✅ **AI Prediction:** LSTM model for load forecasting  

---

**Version:** 3.0 - Fixed Generation Model  
**Last Updated:** January 2027
