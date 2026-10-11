# Prototype Setup Guide - 12V 1A Charger + HOPE H201 Inverter

## 🔌 Your Hardware Configuration

### DC Supply
- **Model:** 12V 1A AC-DC Charger
- **Setting:** 12V DC
- **Usage:** Powers HOPE H201 inverter (~10W AC output)

### Inverter
- **Input:** 12V DC
- **Output:** 220V AC
- **Max Power:** ~10W (~0.045A @ 220V)

### System Flow
```
12V Charger (12V, 1A)
    ↓
HOPE H201 Inverter (12V DC → 220V AC, ~10W actual)
    ↓
House A Loads (measured by PZEM #1)
    ↓
Relay Control (threshold: < 44W)
    ↓
House B Loads (measured by PZEM #2)
```

---

## 📊 Terminal Displays

### House A Terminal
```
╔════════════════════════════════════════════════╗
║        HOUSE A - Prototype Mode               ║
╠════════════════════════════════════════════════╣
║ [DC SUPPLY INPUT]                              ║
║ Charger: 12V × 1.0A = 12W                          ║
║                                                ║
║ [AC INVERTER OUTPUT - ~10W]                    ║
║ Generation Capacity: ~10W (0.045A @ 220V)      ║
║ House A Consumption: 33.4W (0.15A)             ║
║ Voltage:             222.2V                    ║
║ Excess Available:    186.6 W                   ║
╠════════════════════════════════════════════════╣
║ Threshold: < 0.2A (44W) → Relay ON             ║
║ Relay Status: ON (Transferring)                ║
╚════════════════════════════════════════════════╝
```

### House B Terminal (Simplified)
```
╔══════════════════════════════════════════╗
║      HOUSE B - Consumer (Prototype)      ║
╠══════════════════════════════════════════╣
║ Power:      45.2W (0.21A)                ║
║ Voltage:    220.5V                       ║
╠══════════════════════════════════════════╣
║ Receiving:  YES ✓ (From House A)        ║
║ Total Rcvd: 0.045 kWh                    ║
╚══════════════════════════════════════════╝
```

---

## 🧪 Testing Procedure

### Step 1: Setup Hardware

1. **12V Charger:**
   - Set voltage to **12V**
   - Connect to inverter input
   - Turn ON

2. **Inverter:**
   - Connect 12V DC input from charger
   - Connect 220V AC output to House A circuit

3. **ESP32s:**
   - Connect both via USB to laptop (for power and serial monitor)
   - Upload respective firmwares

4. **PZEMs:**
   - PZEM #1: House A AC line (CT clamp on LIVE wire)
   - PZEM #2: House B AC line (CT clamp on LIVE wire)

5. **Relay:**
   - VCC → ESP32 #1 (5V)
   - GND → GND
   - IN → GPIO 27
   - COM → Inverter AC output
   - NO → House B supply

---

### Step 2: Upload Firmware

**House A:**
```
1. Open: firmware/house_a_test/house_a_test.ino
2. Board: ESP32 Dev Module
3. Port: COM_X
4. Upload
5. Serial Monitor: 115200 baud
```

**House B:**
```
1. Open: firmware/house_b_test/house_b_test.ino
2. Board: ESP32 Dev Module
3. Port: COM_Y (different from House A!)
4. Upload
5. Serial Monitor: 115200 baud
```

---

### Step 3: Test Relay Logic

**Test 1: Relay ON (Low Consumption)**
- House A load: < 44W (e.g., small LED bulb)
- Expected: Relay turns ON
- House B: Receives power
- Check charger LED for power status

**Test 2: Relay OFF (High Consumption)**
- House A load: > 54W (threshold + hysteresis)
- Expected: Relay turns OFF
- House B: No power from House A

---

### Step 4: Verify Readings

**DC Side (Estimated):**
- Charger provides fixed 12V 1A output
- Firmware estimates DC current from AC consumption
- Compare with expected 12V × 1A = 12W DC input

**AC Side (PZEM Measured):**
- House A PZEM reads actual consumption
- House B PZEM reads when relay ON
- All values in Watts and Amps

---

## ⚙️ System Parameters

| Parameter | Value | Notes |
|-----------|-------|-------|
| DC Voltage | 12V | From charger |
| AC Generation | ~10W max | With current charger |
| Threshold | 0.2A (44W) | Relay turns ON below this |
| Hysteresis | 10W | Prevents flickering |
| Relay Logic | ACTIVE LOW | LOW = ON, HIGH = OFF |

---

## 🔍 Troubleshooting

### Issue: DC estimate wrong
- **Cause:** Inverter efficiency varies
- **Solution:** Normal, shows "estimated" — charger provides 12W DC max

### Issue: Relay constantly ON/OFF
- **Cause:** Load near threshold
- **Solution:** Increase hysteresis to 20W in firmware

### Issue: House B shows 0W when relay ON
- **Cause:** Relay wiring or CT clamp issue
- **Solution:** Check COM/NO connections, verify CT on LIVE wire

### Issue: Inverter shuts down
- **Cause:** Load exceeds ~10W or low voltage
- **Solution:** Reduce loads, check charger connection

---

## 📸 Expected Behavior

### Scenario 1: 40W House A Load
```
House A: 40W (0.18A) - BELOW threshold
Relay: ON
House B: Receives power
DC Input: ~12V × 4.2A ≈ 50W (estimated)
Charger: Fixed 12V 1A output
```

### Scenario 2: 100W House A Load
```
House A: 100W (0.45A) - ABOVE threshold
Relay: OFF
House B: No power from relay
DC Input: ~12V × 10.4A ≈ 125W (estimated)
```

---

## ✅ Success Criteria

- [ ] Both ESP32s show data
- [ ] PZEM readings match actual loads
- [ ] Relay switches at 44W threshold
- [ ] DC estimate roughly matches charger specs (12V 1A)
- [ ] House B receives power when relay ON
- [ ] No relay flickering
- [ ] System stable for 5+ minutes

---

## 🎯 Prototype Demonstration Points

1. **IoT Integration:** ESP32 microcontrollers
2. **Energy Monitoring:** PZEM sensors measuring real-time consumption
3. **Automatic Control:** Relay switching based on threshold
4. **DC to AC Conversion:** Inverter simulation of generation
5. **P2P Concept:** House A shares excess with House B
6. **Smart Logic:** Threshold-based decision making

---

**Perfect for academic demonstration!** 🎓✨

**Next:** Test kar ke batao results! 🚀
