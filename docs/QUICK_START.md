# Quick Start Guide - Dual House Smart Microgrid

## Your Hardware Setup

You have:
- ✅ **2x PZEM-004T sensors**
- ✅ **2x ESP32 boards**
- ✅ **1x Relay module**
- ✅ **2x Buck converters**

**Generation Source:**
- ✅ **Fixed Generation Model** — 10W assumed constant (simulated solar) + 12V 1A charger + HOPE H201 inverter

---

## System Overview

```
HOUSE A (Solar Producer - Fixed Generation ~10W)
12V Charger → HOPE H201 Inverter → 220V AC → House A Load → PZEM (consumption)
                                                        ↓
                                                   ESP32 #1
                                                        ↓
                                                    Relay
                                                        ↓
HOUSE B (Consumer)                                      ↓
Relay Output → PZEM (consumption) → House B Load ←─────┘
                    ↓
                ESP32 #2
```

**Energy Logic:**
- House A consumption measured by PZEM
- If consumption < 1W → Relay ON → Excess energy to House B
- If consumption ≥ 1W → Relay OFF → No transfer

---

## Step-by-Step Implementation

### Week 1: Software Installation

#### Day 1: Install Arduino IDE & ESP32 Support

1. **Download Arduino IDE**
   - Go to: https://www.arduino.cc/en/software
   - Install latest version

2. **Add ESP32 Board Support**
   - Open Arduino IDE
   - File → Preferences
   - In "Additional Board Manager URLs", paste:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Tools → Board → Boards Manager
   - Search "esp32" and install "esp32 by Espressif Systems"

3. **Install Libraries**
   - Sketch → Include Library → Manage Libraries
   - Install these:
     - **PZEM-004T-v30** by Jakub Mandula
     - **ArduinoJson** by Benoit Blanchon

#### Day 2: Install Node.js & Python

1. **Node.js** (for blockchain server)
   - Download from: https://nodejs.org/
   - Install LTS version
   - Verify: `node --version`

2. **Python** (for AI model)
   - Download from: https://www.python.org/
   - Install Python 3.9+
   - ✅ Check "Add Python to PATH"
   - Verify: `python --version`

---

### Week 2: Hardware Assembly

#### Day 3-4: Assemble House A Circuit

**See:** [hardware/COMPLETE_WIRING_DIAGRAM.md](hardware/COMPLETE_WIRING_DIAGRAM.md) for detailed diagrams

**Connections for ESP32 #1:**
```
PZEM (House A Consumption):
  VCC → 5V
  GND → GND
  TX → GPIO 18
  RX → GPIO 19

Relay Module:
  VCC → 5V
  GND → GND
  IN → GPIO 27

Buck Converter:
  Output (+5V) → ESP32 VIN
  Output (GND) → ESP32 GND
```

**Checklist:**
- [ ] All wires secure
- [ ] No short circuits
- [ ] Buck converter set to 5V output
- [ ] PZEM sensor has AC power (from inverter)

#### Day 5: Assemble House B Circuit

**Connections for ESP32 #2:**
```
PZEM (House B Consumption):
  VCC → 5V
  GND → GND
  TX → GPIO 16
  RX → GPIO 17

Buck Converter:
  Output (+5V) → ESP32 VIN
  Output (GND) → ESP32 GND
```

**Checklist:**
- [ ] All wires secure
- [ ] No short circuits
- [ ] Buck converter set to 5V output

---

### Week 3: Firmware Upload

#### Day 8: Program House A Controller

1. **Open Firmware**
   - File → Open → `firmware/house_a_controller/house_a_controller.ino`

2. **Update Configuration** in `secrets.h`:
   ```cpp
   #define WIFI_SSID "YOUR_WIFI_NAME"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   #define SERVER_IP "YOUR_PC_IP"  // Find with: ipconfig
   ```

3. **Upload**
   - Connect ESP32 #1 via USB
   - Tools → Board → ESP32 Dev Module
   - Tools → Port → Select COM port
   - Click Upload (→)

4. **Verify**
   - Tools → Serial Monitor (115200 baud)
   - Should see:
     ```
     HOUSE A - Fixed Generation Model v3.0
     ✓ WiFi connected
     ✓ Server connection OK!
     ✓ PZEM sensor OK
     HOUSE A - READY TO OPERATE
     ```

#### Day 9: Program House B Controller

1. **Open Firmware**
   - File → Open → `firmware/house_b_controller/house_b_controller.ino`

2. **Update Configuration** in `secrets.h`:
   ```cpp
   #define WIFI_SSID "YOUR_WIFI_NAME"           // Same WiFi
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   #define SERVER_IP "YOUR_PC_IP"               // Same server IP
   ```

3. **Upload to ESP32 #2**

4. **Verify**
   - Serial Monitor should show:
     ```
     Smart Microgrid - House B Controller v2.0
     ✓ WiFi Connected!
     ✓ PZEM #3 (House B Consumption) OK
     ✓ Server connected!
     System Ready!
     ```

---

### Week 4: Backend Server Setup

#### Day 10: Setup Node.js Server

1. **Install Dependencies**
   ```powershell
   cd D:\Academic\RumanProject\backend
   npm install
   ```

2. **Start Server**
   ```powershell
   npm start
   ```

3. **Verify**
   - Should see: "Server running on port 3000"
   - Open browser: http://localhost:3000
   - Should see JSON response

4. **Update ESP32 Code** (if server on different computer)
   - Find your PC's IP: `ipconfig`
   - Update in both ESP32 codes:
     ```cpp
     const char* serverUrl = "http://YOUR_PC_IP:3000/api/transaction";
     ```

---

### Week 5: Testing

#### Complete System Test

1. **Start Everything**
   - Backend server running
   - Both ESP32s powered on
   - Both show WiFi connected in Serial Monitor

2. **Open Web Dashboard**
   - Go to: http://localhost:3000
   - Both houses should show live data

3. **Test Automatic Transfer**
   - Increase solar production (or decrease House A load)
   - Watch Serial Monitor on ESP32 #1:
     ```
     House A Consumption: 0.0W
     🔋 LOW CONSUMPTION DETECTED
     ✓ Relay ON - Energy flowing to House B
     ```
   - Check web dashboard: Relay status should be ON
   - Check ESP32 #2: Should detect voltage (receiving energy)

4. **Test Manual Control**
   - Add load to House A (≥ 1W)
   - Relay should turn OFF
   - Serial Monitor shows "HIGH CONSUMPTION DETECTED"

---

### Week 6-7: Data Collection

**Run system continuously for 2+ weeks**

- Keep both ESP32s running
- Keep backend server running
- Data automatically saved to `backend/data/transactions.json`
- Monitor via web dashboard at http://localhost:3000

---

### Week 8: AI Model Training

1. **Install Python Libraries**
   ```powershell
   cd D:\Academic\RumanProject\ml-model
   pip install -r requirements.txt
   ```

2. **Train Model**
   ```powershell
   python train_model.py
   ```

3. **Check Results**
   - `models/load_prediction_model.h5` (trained model)
   - `models/training_history.png` (training graphs)
   - `models/predictions.png` (prediction results)

---

## Troubleshooting

### ESP32 Won't Upload
- Hold BOOT button while uploading
- Check correct COM port selected
- Try different USB cable

### PZEM Shows All Zeros
- Check TX/RX wiring (must be crossed: TX→RX, RX→TX)
- Verify 5V power to PZEM
- For testing: Use 12V DC + inverter (not just DC)

### Blynk Shows Offline
- Blynk has been removed from the firmware
- Use the local web dashboard at http://localhost:3000 instead

### Relay Won't Switch
- Check GPIO 27 connection
- Verify 5V power to relay
- Test with simple blink code first

### Backend Server Won't Start
- Check Node.js installed: `node --version`
- Run `npm install` first
- Check port 3000 not already in use

---

## Important Files

- **[hardware/COMPLETE_WIRING_DIAGRAM.md](hardware/COMPLETE_WIRING_DIAGRAM.md)** - Detailed wiring diagrams
- **[docs/PROJECT_DEMONSTRATION_GUIDE.md](docs/PROJECT_DEMONSTRATION_GUIDE.md)** - Demo preparation
- **[docs/NGROK_SETUP_GUIDE.md](docs/NGROK_SETUP_GUIDE.md)** - Remote access setup

---

## Safety Reminders

> ⚠️ **CRITICAL SAFETY RULES**
> 
> 1. **Start with LOW VOLTAGE** (12V DC + small inverter)
> 2. **Never work on live AC circuits**
> 3. **Double-check all connections** before powering on
> 4. **Use proper insulation** on all AC connections
> 5. **Have emergency shutoff** ready

---

## Success Checklist

Your system is working when:

- [ ] Both ESP32s connect to WiFi
- [ ] All 2 PZEM sensors show readings
- [ ] Relay switches automatically based on 1W threshold
- [ ] Web dashboard shows real-time data
- [ ] Backend server records transactions
- [ ] Blockchain transactions recorded in Ganache
- [ ] Data collected for 2+ weeks
- [ ] AI model trained successfully

---

**You're ready to start! Begin with Week 1, Day 1 and follow step-by-step.** 🚀

Good luck with your final year project!
