# Testing Guide - Fixed Generation Model (2-PZEM System)

## Overview

This guide walks you through testing the **Fixed Generation Model** with:
- ✅ **Assumed Generation:** 0.045A (~10W) - fixed, not measured
- ✅ **2 PZEMs:** Measure House A and House B consumption
- ✅ **Threshold:** 1W consumption-based relay control

---

## Pre-Testing Checklist

### Hardware Verification

- [ ] **DC Power Supply (12V 1A charger)** connected and powered  
- [ ] **ESP32 #1 (House A)** powered via buck converter
- [ ] **ESP32 #2 (House B)** powered via buck converter
- [ ] **PZEM #1** CT clamp on House A live wire
- [ ] **PZEM #2** CT clamp on House B live wire
- [ ] **Relay module** wired to ESP32 #1 GPIO 27
- [ ] **Relay AC side** connected (COM → Main Supply, NO → House B)
- [ ] All grounds properly connected

### Software Verification

- [ ] Firmware `house_a_controller.ino` uploaded to ESP32 #1
- [ ] Firmware `house_b_controller.ino` uploaded to ESP32 #2  
- [ ] WiFi credentials configured in `secrets.h`
- [ ] Server URL configured correctly
- [ ] Backend server running (optional for basic testing)

---

## Test 1: ESP32 and PZEM Basic Functionality

### Steps

1. **Connect ESP32 #1 to PC via USB**
2. **Open Serial Monitor** (115200 baud)
3. **Observe startup messages:**

**Expected Output:**
```
╔════════════════════════════════════════════════╗
║   Smart Microgrid - House A Controller v3.0   ║
║           Fixed Generation Model               ║
╚════════════════════════════════════════════════╝

Connecting to WiFi: YourSSID
✓ WiFi Connected!
  IP Address: 192.168.x.x

Testing PZEM Sensor...
✓ PZEM #1 (House A Consumption) OK

📊 Fixed Generation Configuration:
   Assumed Generation: 0.045A (~10W)
   Consumption Threshold: 1.0W

Testing server connection: http://x.x.x.x:3000/api/transaction
✓ Server connected!
✓ Blockchain server ready

╔════════════════════════════════════════════════╗
║      HOUSE A - Fixed Generation Model v3.0     ║
╠════════════════════════════════════════════════╣
║ Assumed Generation:  10W (0.045A) [FIXED]      ║
║ House A Consumption: 0.0W (0.00A)             ║
║ Excess Available:    220.0 W                  ║
╠════════════════════════════════════════════════╣
║ Threshold: < 1.0W → Relay ON                  ║
║ Relay Status: OFF (Standby)                   ║
║ Mode: AUTOMATIC                                ║
║ Transactions: 0                                ║
║ Server: Connected ✓                            ║
╚════════════════════════════════════════════════╝
```

**✅ Pass Criteria:**
- PZEM #1 reports "OK"
- Fixed generation shows 0.045A (~10W)
- Threshold shows < 1W triggers relay

---

## Test 2: Consumption Measurement (House A)

### Steps

1. **Turn ON a small House A load** (e.g., 40W bulb)
2. **Observe serial monitor** for updated readings

**Expected Output:**
```
║ Assumed Generation:  10W (0.045A) [FIXED]      ║
║ House A Consumption: 40.5W (0.18A)            ║
║ Excess Available:    179.5 W                  ║
```

**✅ Pass Criteria:**
- House A consumption updates in real-time
- Current reading matches actual load
- Excess = 10W - consumption

**⚠️ Troubleshooting:**
| Issue | Solution |
|-------|----------|
| Consumption shows 0W | Check CT clamp orientation (live wire only) |
| Wrong values | Ensure PZEM calibrated (reset PZEM if needed) |
| No updates | Verify GPIO 18/19 connections |

---

## Test 3: Relay Activation (Low Consumption)

### Objective
Verify relay turns ON when House A < 1W

### Steps

1. **Ensure House A load < 1W** (e.g., turn OFF all loads)
2. **Wait 2-3 seconds** for relay activation
3. **Observe serial monitor:**

**Expected Output:**
```
🔋 LOW CONSUMPTION DETECTED - Starting transfer to House B
   House A Consumption: 0.0W (0.000A)
   Excess Available: 220.0W
✓ Relay ON - Energy flowing to House B

╠════════════════════════════════════════════════╣
║ Relay Status: ON (Transferring)               ║
```

4. **Physical Verification:**
   - [ ] Relay module LED turns ON
   - [ ] Audible relay click
   - [ ] House B loads receive power

**✅ Pass Criteria:**
- Relay activates when consumption < 1W
- Serial shows "LOW CONSUMPTION DETECTED"
- House B can power loads

---

## Test 4: Relay Deactivation (High Consumption)

### Objective
Verify relay turns OFF when House A ≥ 1W

### Steps

1. **Turn ON House A loads** to exceed **1W** (e.g., any small bulb)
2. **Example:** Use combination of bulbs/appliances
3. **Observe serial monitor:**

**Expected Output:**
```
⏸️  HIGH CONSUMPTION DETECTED - Stopping transfer
   House A Consumption: 40.5W
✓ Relay OFF - No energy transfer

╠════════════════════════════════════════════════╣
║ Relay Status: OFF (Standby)                   ║
```

4. **Physical Verification:**
   - [ ] Relay module LED turns OFF
   - [ ] Audible relay click
   - [ ] House B loads stop receiving power (if only powered by relay)

**✅ Pass Criteria:**
- Relay deactivates when consumption ≥ 1W
- Serial shows "HIGH CONSUMPTION DETECTED"
- House B loses transferred power

---

## Test 5: Hysteresis Behavior

### Objective
Prevent relay flickering near threshold

### Steps

1. **Set House A load near threshold** (~1W)
2. **Observe relay for 2 minutes**

**Expected Behavior:**
- Relay should NOT flicker repeatedly
- Relay ON when consumption = 0W
- Relay OFF when consumption ≥ 1W

**✅ Pass Criteria:**
- No rapid ON/OFF switching
- Stable operation at threshold boundary

---

## Test 6: Web Dashboard Integration

### Steps

1. **Open web dashboard** at http://localhost:3000
2. **Verify live data:**

| Data Point | Expected Value | Notes |
|------------|----------------|-------|
| House A Power | ~actual | Measured from PZEM |
| House A Voltage | ~220V | Measured |
| Relay Status | ON or OFF | Real-time |
| House B Power | ~actual | Measured from PZEM |
| Transaction Count | Increasing | Every 60s when relay ON |

**✅ Pass Criteria:**
- Dashboard shows live data for both houses
- Relay status matches physical relay
- Transaction history visible

---

## Test 7: Blockchain Transaction Recording

### Steps

1. **Ensure backend server is running:**
   ```powershell
   cd backend
   npm start
   ```

2. **Activate relay** (House A < 1W)
3. **Wait 60 seconds** (default transaction interval)
4. **Check serial monitor:**

**Expected Output:**
```
📡 Sending transaction to blockchain server...
✓ Blockchain TX: 0x1a2b3c4d...
✓ Block Number: 123
✓ Gas Used: 85432
```

5. **Verify on backend server console:**
```
📊 New Transaction from House_A
   Power: 4200 W, Energy: 35 Wh
   ⛓️  Recording to blockchain...
   ✓ Blockchain TX: 0x1a2b3c4d...
```

**✅ Pass Criteria:**
- Transaction sent every 60 seconds
- Blockchain hash returned
- No errors in server logs

---

## Test 8: House B Controller

### Steps

1. **Connect ESP32 #2 to PC via USB**
2. **Open Serial Monitor**
3. **With relay ON** (House A < 1W), observe:

**Expected Output:**
```
╔════════════════════════════════════════════════╗
║      HOUSE B - Consumer Controller v2.0        ║
╠════════════════════════════════════════════════╣
║ Power Consumption:   120.5 W                   ║
║ Receiving from House A: YES                    ║
║ Total Received:      0.45 kWh                  ║
╚════════════════════════════════════════════════╝
```

**✅ Pass Criteria:**
- PZEM #2 measures House B loads correctly
- "Receiving from House A" updates based on relay status
- Cumulative energy tracked

---

## Test 9: Web Dashboard Monitoring

### Steps

1. **Open web dashboard** at http://localhost:3000
2. **Verify both houses show live data**
3. **Toggle House A load ON/OFF**

**Expected Behavior:**
- Relay status updates in dashboard
- Power readings update in real-time
- Transaction history shows blockchain hashes

**✅ Pass Criteria:**
- Dashboard reflects physical state
- Data updates within 1 second

---

## Test 10: WiFi Reconnection

### Steps

1. **Disable WiFi router** temporarily
2. **Observe serial monitor:**

**Expected Output:**
```
⚠️  WiFi Connection Lost!
⚠️  Blockchain not connected - saving locally only
```

3. **Re-enable WiFi**
4. **ESP32 should reconnect** within 30 seconds

**✅ Pass Criteria:**
- System continues operating offline
- Auto-reconnects when WiFi available
- No data loss (local backup)

---

## Complete System Test Sequence

### Full Flow Test

1. **Initial State:** Both ESP32s powered, all loads OFF
2. **Turn ON House A small load** (< 1W)
   - ✅ Relay activates
   - ✅ House B receives power
   - ✅ Dashboard updates
   - ✅ Blockchain transaction sent

3. **Turn ON House A load** (≥ 1W)
   - ✅ Relay deactivates
   - ✅ House B stops receiving
   - ✅ Dashboard reflects change

4. **Monitor for 5 minutes:**
   - ✅ Stable operation
   - ✅ No unexpected resets
   - ✅ Consistent readings

---

## Performance Metrics

Track these metrics during testing:

| Metric | Target | Notes |
|--------|--------|-------|
| PZEM read interval | 0.5 seconds | Defined in firmware |
| Dashboard update rate | 0.5 seconds | Live telemetry |
| Blockchain transaction | 60 seconds | When relay is ON |
| Relay response time | < 1 second | Threshold detection |
| WiFi reconnect time | < 30 seconds | Auto-recovery |

---

## Common Issues & Solutions

| Problem | Cause | Solution |
|---------|-------|----------|
| Relay constantly ON/OFF | Load near threshold | Increase hysteresis (100W → 200W) |
| PZEM reads 0W always | CT not clamped | Ensure live wire passes through CT |
| Blockchain TX fail | Server offline | Check backend server status |
| ESP32 reboots | Power insufficient | Use 2A buck converter minimum |
| Wrong current values | CT reversed | Correct CT clamp orientation |

---

## Final Acceptance Criteria

✅ **Hardware:**
- [ ] Both PZEMs measure consumption accurately
- [ ] Relay switches at 1W threshold
- [ ] No loose connections or overheating

✅ **Software:**
- [ ] Firmware compiles without errors
- [ ] Both ESP32s connect to WiFi
- [ ] Blockchain transactions recorded

✅ **Functionality:**
- [ ] Automatic relay control works
- [ ] Manual override functional
- [ ] System stable for 10+ minutes continuous operation

---

## Next Steps After Testing

Once all tests pass:
1. ✅ **Secure all AC wiring** with electrical tape
2. ✅ **Mount components** in enclosure
3. ✅ **Label all connections** for maintenance
4. ✅ **Document any threshold adjustments**
5. ✅ **Prepare for final demonstration**

---

**Version:** 3.0 - Fixed Generation Model  
**Last Updated:** January 2027
