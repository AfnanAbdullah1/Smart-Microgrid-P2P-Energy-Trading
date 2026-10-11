# Simple Test Firmware Guide

## Overview

These are **simplified test firmwares** without WiFi, server, or Blockchain - just basic PZEM reading and relay control!

**Use these for initial hardware testing before deploying the full system.**

---

## Test Firmware Files

### 1. House A Test
**File:** `firmware/house_a_test/house_a_test.ino`

**Features:**
- ✅ Reads PZEM #1 (House A consumption)
- ✅ Controls relay based on threshold (1A)
- ✅ Serial Monitor output
- ❌ No WiFi
- ❌ No Blynk
- ❌ No Blockchain

**Hardware:**
- PZEM #1: GPIO 18 (RX), GPIO 19 (TX)
- Relay: GPIO 27
- LED: GPIO 2

---

### 2. House B Test
**File:** `firmware/house_b_test/house_b_test.ino`

**Features:**
- ✅ Reads PZEM #2 (House B consumption)
- ✅ Detects when receiving power from House A
- ✅ Serial Monitor output
- ❌ No WiFi
- ❌ No Blynk
- ❌ No Blockchain

**Hardware:**
- PZEM #2: GPIO 16 (RX), GPIO 17 (TX)
- LED: GPIO 2

---

## Required Library

**Only one library needed:**
```
PZEM-004T-v30 by Jakub Mandula
```

**Installation:**
1. Arduino IDE → Tools → Manage Libraries
2. Search "PZEM-004T-v30"
3. Install latest version

---

## Upload Instructions

### Step 1: Upload House A Test

1. Open `house_a_test.ino` in Arduino IDE
2. Select Board: **ESP32 Dev Module**
3. Select COM Port
4. Click **Upload**
5. Open **Serial Monitor** (115200 baud)

**Expected Output:**
```
╔════════════════════════════════════════════════╗
║   House A - Test Firmware v1.0 (SIMPLIFIED)   ║
║   No WiFi | No Blynk | No Blockchain          ║
╚════════════════════════════════════════════════╝

✓ Pins configured
  - Relay: GPIO 27
  - LED: GPIO 2

Testing PZEM...
✓ PZEM #1 OK

📊 Configuration:
  Fixed Generation: 3.0A (660W)
  Threshold: < 0.045A (~10W) → Relay ON

🚀 System Ready! Reading every 2 seconds...

╔════════════════════════════════════════════════╗
║      HOUSE A - Test Mode (Simplified)         ║
╠════════════════════════════════════════════════╣
║ Assumed Generation:  660W (3.0A) [FIXED]      ║
║ House A Consumption: 45.2W (0.21A)            ║
║ House A Voltage:     220.5V                   ║
║ Excess Available:    614.8 W                  ║
╠════════════════════════════════════════════════╣
║ Threshold: < 0.045A (~10W) → Relay ON           ║
║ Relay Status: ON (Transferring)               ║
╚════════════════════════════════════════════════╝
```

---

### Step 2: Upload House B Test

1. Open `house_b_test.ino` in Arduino IDE
2. Select Board: **ESP32 Dev Module**
3. Select COM Port (different from House A!)
4. Click **Upload**
5. Open **Serial Monitor** (115200 baud)

**Expected Output:**
```
╔════════════════════════════════════════════════╗
║   House B - Test Firmware v1.0 (SIMPLIFIED)   ║
║   No WiFi | No Blynk | No Blockchain          ║
╚════════════════════════════════════════════════╝

✓ Pins configured
  - LED: GPIO 2

Testing PZEM...
✓ PZEM #2 OK

🚀 System Ready! Reading every 2 seconds...

╔════════════════════════════════════════════════╗
║      HOUSE B - Test Mode (Simplified)         ║
╠════════════════════════════════════════════════╣
║ Power Consumption:   120.5W (0.55A)           ║
║ Voltage:             220.2V                    ║
║ Total Energy:        0.045 kWh                ║
╠════════════════════════════════════════════════╣
║ Receiving from A:    YES ✓                     ║
║ Total Received:      0.045 kWh                ║
╚════════════════════════════════════════════════╝
```

---

## Testing Procedure

### Test 1: PZEM Verification

**Goal:** Verify both PZEMs are reading correctly

**Steps:**
1. Upload both firmwares
2. Check serial monitors
3. Both should show "✓ PZEM #X OK"

**If PZEM NOT responding:**
- Check TX/RX wiring
- Verify PZEM power (5V)
- Check CT clamp connection

---

### Test 2: House A Relay Control

**Goal:** Verify relay activates at threshold

**Steps:**

1. **Start with NO House A load** (or very small < 1A)
2. **Check serial monitor:**
   ```
   🔋 RELAY ON - Low consumption detected!
      House A: 45.2W (0.21A)
      Excess: 614.8W available for House B
   ```
3. **Relay should turn ON** (check LED/click sound)

4. **Add House A load > 1A** (e.g., 300W appliance)
5. **Check serial monitor:**
   ```
   ⏸️  RELAY OFF - High consumption detected!
      House A: 320.0W
   ```
6. **Relay should turn OFF**

---

### Test 3: Cross-Verification

**Goal:** Verify House B detects relay state

**Setup:**
1. Both ESP32s connected via USB
2. Open **2 Serial Monitors** (2 Arduino IDE windows)

**When House A Relay ON:**

House A shows:
```
║ Relay Status: ON (Transferring)               ║
```

House B shows:
```
🔌 RECEIVING POWER FROM HOUSE A!
║ Receiving from A:    YES ✓                     ║
```

**When House A Relay OFF:**

House A shows:
```
║ Relay Status: OFF (Standby)                   ║
```

House B shows:
```
🔌 Power from House A STOPPED
║ Receiving from A:    NO                        ║
```

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "PZEM NOT responding" | Check GPIO pins, 5V power, CT clamp |
| Relay never turns ON | Reduce House A load to < 10W |
| Relay constantly ON/OFF | Check hysteresis (50W default) |
| House B shows 0W always | Check relay wiring (COM/NO) |
| Wrong current readings | Ensure only LIVE wire through CT |

---

## Success Criteria

✅ **House A:**
- PZEM reads consumption correctly
- Relay turns ON when < 1A
- Relay turns OFF when > 1A + hysteresis
- Serial output shows values

✅ **House B:**
- PZEM reads consumption correctly
- Detects relay state (YES/NO)
- Serial output shows values

---

## Next Steps

Once basic testing passes:

1. ✅ **Hardware Verified** - PZEMs working, relay switching
2. 📝 **Upload Full Firmware** - Add WiFi, Dashboard, Blockchain
3. 🧪 **Complete System Test** - End-to-end with backend

---

## Quick Reference

| Feature | House A Test | House B Test |
|---------|--------------|--------------|
| WiFi | ❌ | ❌ |
| Blynk | ❌ | ❌ |
| Blockchain | ❌ | ❌ |
| PZEM Reading | ✅ | ✅ |
| Relay Control | ✅ | ❌ |
| Serial Output | ✅ | ✅ |
| Library Required | PZEM-004T-v30 | PZEM-004T-v30 |

---

**Perfect for quick hardware testing!** 🚀
