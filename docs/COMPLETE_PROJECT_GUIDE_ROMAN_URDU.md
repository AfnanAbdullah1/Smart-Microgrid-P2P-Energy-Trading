# Poore Project Ki Mukammal Rahnumai
# Smart Microgrid - Complete Project Guide (Roman Urdu)

**BS Electrical Engineering - Final Year Project**  
**Asaan Roman Urdu Mein Mukammal Guide**

---

## Fihrist (Table of Contents)

1. [Project Kya Hai?](#project-kya-hai)
2. [Hardware Ki List](#hardware-ki-list)
3. [Software Ki List](#software-ki-list)
4. [Installation Guide](#installation-guide)
5. [Wiring Diagram](#wiring-diagram)
6. [System Kaise Chalayen](#system-kaise-chalayen)
7. [Troubleshooting](#troubleshooting)

---

## Project Kya Hai?

### Asaan Alfaaz Mein:

Ye project aik **Smart Microgrid** hai jis mein:

🏠 **House A** (Bechnay Wala):
- Solar panel se ~10W bijli ban rahi hai
- Ghar mein sirf 1W istemaal ho rahi hai
- Bachi hui 219W bijli House B ko bech saktay hain

🏠 **House B** (Khareedne Wala):
- Koi solar panel nahi
- Bijli chahiye
- House A se khareedtay hain

⛓️ **Blockchain**:
- Tamam len-den (transaction) ka record
- Koi mita nahi sakta
- Koi badal nahi sakta
- Hamesha ke liye mehfooz

### Asli Zindagi Mein Istemaal:

```
Subah 9 Bajay:
- House A: Solar se ~10W ban rahi hai
- House A istemaal: sirf 0.7W (kuch nahi chal raha)
- Relay automatic ON ho jati hai
- 219.3W House B ko chali jati hai
- House B: pankha, lights chal rahe hain

30 Second Baad:
- Transaction blockchain pe save hota hai
- Cost: Rs 0.051 (5 paisay)
- Block #38 mein permanent record

Dopehar 12 Bajay:
- House A: Bulb chaalu kar diya (istemaal 5W)
- Relay automatic OFF ho jati hai
- House B ko bijli nahi milti
- House A apni bijli istemaal karta hai
```

---

## Hardware Ki List

### Mukammal List (Qeemton Ke Saath):

| No | Cheez Ka Naam | Tadaad | Tafseel | Qeemat (Rupay) |
|----|---------------|--------|---------|----------------|
| 1 | ESP32 (Microcontroller) | 2 | WiFi wala | 800 |
| 2 | PZEM-004T (Sensor) | 2 | Bijli napne wala | 1,200 |
| 3 | Relay Module | 1 | On/Off karne wala | 150 |
| 4 | 12V 1A Charger | 1 | 12V bijli | 500 |
| 5 | Inverter (220V) | 1 | DC se AC | 1,800 |
| 6 | 1602A LCD + I2C | 1 | House A display | 250 |
| 7 | Tarain (Wires) | - | Mukhtalif | 200 |
| 7 | Breadboard | 2 | Connection ke liye | 300 |
| 8 | USB Cables | 2 | ESP32 ke liye | 200 |

**Kul Qeemat: Taqreeban Rs 7,150**

### Har Cheez Kya Karti Hai?

#### 1. ESP32 (Dimaagh Ki Tarah)
```
Kaam:
✅ WiFi se jurta hai
✅ Sensor se data parhta hai
✅ Relay ko ON/OFF karta hai
✅ Server ko data bhejta hai
✅ Dashboard ko live data bhejta hai

Pin Istemaal:
- GPIO 18, 19 → PZEM sensor
- GPIO 27 → Relay control
- 3.3V, GND → Power
```

#### 2. PZEM-004T (Meter Ki Tarah)
```
Kaam:
✅ Voltage napta hai (Volt)
✅ Current napta hai (Ampere)
✅ Power calculate karta hai (Watt)
✅ Energy jorta hai (Wh)

Reading Ki Frequency:
Har 0.5 second (500ms)
```

#### 3. Relay (Switch Ki Tarah)
```
Kaam:
✅ House A se House B ka rabta
✅ Automatic ON/OFF
✅ ESP32 control karta hai

Logic:
- GPIO 27 = LOW → Relay ON (bijli ja rahi hai)
- GPIO 27 = HIGH → Relay OFF (bijli ruki hui hai)
```

#### 4. 12V 1A Charger (Solar Ki Jagah)
```
Kaam:
✅ 12V DC output
✅ ~10W power (12V × 1A = 12W DC, ~10W AC after inverter)
✅ Solar panel ki jagah (prototype)
```

#### 5. Inverter (Tabdeeli)
```
Kaam:
✅ 12V DC → 220V AC
✅ PZEM ke liye AC voltage
```

---

## Software Ki List

### 1. ESP32 Firmware (Arduino C++)

**Location:** `firmware/house_a_controller/` aur `firmware/house_b_controller/`

**Kaam:**
```cpp
✅ WiFi se jurna
✅ PZEM se data parhna (har 0.5 second)
✅ Relay ko ON/OFF karna (consumption dekh kar)
✅ Server ko transaction bhejna (har 60 second)
✅ Local dashboard ko update karna (har 0.5 second)
```

**Libraries:**
- WiFi.h
- HTTPClient.h
- PZEM004Tv30.h
- WiFiClientSecure.h
- ArduinoJson.h

### 2. Backend Server (Node.js)

**Location:** `backend/server.js`

**Kaam:**
```javascript
✅ ESP32 se transaction receive karna
✅ Blockchain pe save karna
✅ Web dashboard chalana
✅ API endpoints faraham karna
```

**Port:** 3000

**API Endpoints:**
```
POST /api/transaction        → Naya transaction
GET  /api/transactions       → Sab transactions
GET  /api/blockchain/status  → Blockchain ki halat
GET  /api/stats              → Adaad-o-shumaar
```

### 3. Blockchain (Ganache + Solidity)

**Location:** `blockchain/contracts/EnergyTrading.sol`

**Kaam:**
```solidity
✅ Transactions save karna (permanent)
✅ Cost calculate karna
✅ Data retrieve karna
```

**Contract Address:** `0x5f3541DD8401b2A0e1B3Af64a582bf3894d67ADb`

**Ganache:** http://127.0.0.1:7545

### 4. Web Dashboard

**Location:** `backend/public/index.html`

**Kaam:**
```
✅ Real-time monitoring
✅ Transaction history
✅ Statistics
✅ Auto-refresh (har 5 second)
```

**URL:** http://localhost:3000

### 5. Local Web Dashboard

**Kaam:**
```
✅ Live power readings
✅ Relay status
✅ Transaction count
✅ Blockchain status
```

**URL:** http://localhost:3000

---

## Installation Guide

### Step 1: Arduino IDE Install Karein

```
1. Download karein:
   https://www.arduino.cc/en/software

2. Install karein (Next, Next, Finish)

3. ESP32 board add karein:
   File → Preferences → Additional Board Manager URLs:
   https://dl.espressif.com/dl/package_esp32_index.json

4. ESP32 install karein:
   Tools → Board → Boards Manager
   Search: "ESP32"
   Install: "esp32 by Espressif Systems"

5. Libraries install karein:
   Sketch → Include Library → Manage Libraries
   
   Install karein:
   ✅ PZEM-004T-v30
   ✅ ArduinoJson
```

### Step 2: Node.js Install Karein

```
1. Download:
   https://nodejs.org/

2. Install (Next, Next, Finish)

3. Check karein:
   Command Prompt (CMD) kholein:
   
   > node --version
   v20.11.1  ✅
   
   > npm --version
   10.2.4  ✅
```

### Step 3: Ganache Install Karein

```
1. Download:
   https://trufflesuite.com/ganache/

2. Install karein

3. Open karein:
   - Click "Quickstart"
   - RPC Server: http://127.0.0.1:7545
   - 10 accounts dikhein ge (har aik mein 100 ETH)
```

### Step 4: Backend Setup

```
1. Folder open karein:
   cd "d:\Academic\Paid_Projects_Clients\Ruman Appi - Project _ P2P Current transfering\backend"

2. Install dependencies:
   npm install

3. Wait for completion
   node_modules folder ban jaye ga ✅
```

### Step 5: Smart Contract Deploy Karein

```
1. Ganache chaalu rakhein

2. Deploy karein:
   cd "d:\...\blockchain"
   npm install
   npx hardhat run scripts/deploy.js --network ganache

3. Contract address note karein:
   0x5f35...
```

### Step 6: Backend Configure Karein

```
File: backend/server.js

Line 15-16 mein:
const GANACHE_URL = 'http://127.0.0.1:7545';
const CONTRACT_ADDRESS = '0x5f3541DD8401b2A0e1B3Af64a582bf3894d67ADb';

(Apna contract address dalein)
```

### Step 7: ESP32 Firmware Configure Karein

```
File: firmware/house_a_controller/house_a_controller.ino

Lines 13-17:
const char* ssid = "APKA_WIFI_NAME";         ← Apna WiFi
const char* password = "APKA_PASSWORD";       ← Apna password
const char* serverUrl = "http://192.168.1.19:3000/api/transaction";  ← Apna IP

house_b_controller mein bhi same karein!
```

### Step 8: Firmware Upload

```
Arduino IDE mein:

1. house_a_controller.ino kholein

2. Tools → Board: "ESP32 Dev Module"

3. Tools → Port: Select (COM3, COM4, etc.)

4. Upload button dabayein (→)

5. "Done uploading" ka intezaar ✅

6. house_b_controller.ino ke liye repeat karein
```

### Step 9: Web Dashboard Dekhein

```
1. Browser kholein

2. Jayein: http://localhost:3000

3. Live readings dekhein

4. Transactions ka record dekhein
```

---

## Wiring Diagram

### House A Connections:

```
12V Charger → HOPE H201 Inverter → 220V AC Output (~10W)
                           │
                           ├→ PZEM-004T
                           │    ├ VCC → 5V
                           │    ├ GND → GND
                           │    ├ TX → ESP32 GPIO 16
                           │    └ RX → ESP32 GPIO 17
                           │
                           └→ Relay COM

ESP32 House A:
├ GPIO 16 → PZEM RX
├ GPIO 17 → PZEM TX
├ GPIO 27 → Relay IN
└ USB → Power

Relay:
├ VCC → 5V
├ GND → GND
├ IN → GPIO 27
├ COM → AC Source (220V)
├ NO → To House B
└ NC → Not used
```

### House B Connections:

```
Relay NO output → PZEM-004T → House B Load
                      │
                      ├ VCC → 5V
                      ├ GND → GND
                      ├ TX → ESP32 GPIO 16
                      └ RX → ESP32 GPIO 17

ESP32 House B:
├ GPIO 16 → PZEM RX
├ GPIO 17 → PZEM TX
└ USB → Power

House B Load:
└ Bulbs, Fan, etc.
```

---

## System Kaise Chalayen

### Shuru Karna:

#### 1. Ganache Shuru Karein

```
1. Ganache application kholein
2. "Quickstart" click karein
3. RPC Server check karein: http://127.0.0.1:7545 ✅
```

#### 2. Backend Server Shuru Karein

```
1. Command Prompt kholein:
   cd "d:\...\backend"

2. Server chalayen:
   npm start

3. Dekhein:
   ✓ Server running on port 3000
   ✓ Blockchain: Connected
```

#### 3. ESP32s ON Karein

```
1. ESP32 #1 (House A) USB se jorein
2. ESP32 #2 (House B) USB se jorein
3. Serial Monitor kholein (115200 baud)
4. Dekhein:
   ✓ WiFi connected
   ✓ Server connection OK!
```

#### 4. Web Dashboard Kholein

```
1. Browser kholein
2. Jayein: http://localhost:3000
3. Dekhein:
   ✓ Live data (sabz)
   ✓ Contract address
```

### Testing:

#### Test 1: Low Consumption (Relay ON)

```
1. House A mein kuch na chalayen (consumption < 1W)
2. Serial Monitor:
   ✓ "Relay ON - Energy flowing"
3. 30 second baad:
   ✓ "Transaction recorded!"
4. Dashboard:
   ✓ Transaction #1 nazar aaye ga
```

#### Test 2: High Consumption (Relay OFF)

```
1. Bulb ON karein (consumption > 1W)
2. Serial Monitor:
   ✓ "Relay OFF - No energy transfer"
3. Koi transaction nahi
```

---

## Troubleshooting

### Masla 1: WiFi Nahi Jur Raha

```
Alamaat:
- "Connecting to WiFi..." laga rah raha hai

Hal:
✅ WiFi name/password check karein
✅ 2.4GHz WiFi honi chahiye (5GHz nahi)
✅ Router check karein
✅ ESP32 reset karein
```

### Masla 2: PZEM "NaN" Dikha Raha Hai

```
Alamaat:
- Voltage: NaN
- Current: NaN

Hal:
✅ PZEM wiring check (TX, RX)
✅ 5V power check
✅ AC voltage mojood hai?
```

### Masla 3: Relay Kaam Nahi Kar Raha

```
Alamaat:
- Relay click nahi ho raha
- LED nahi jal raha

Hal:
✅ Relay wiring check (VCC, GND, IN)
✅ GPIO 27 connection
✅ Active-LOW check (LOW = ON)
```

### Masla 4: Backend Nahi Jur Raha

```
Alamaat:
- "Server connection failed"

Hal:
✅ Backend running hai?
✅ IP address sahi hai?
   CMD: ipconfig
   IPv4 Address nikalein
✅ Firewall off karein (testing)
```

### Masla 5: Ganache Nahi Jur Raha

```
Alamaat:
- "Error connecting to blockchain"

Hal:
✅ Ganache running hai?
✅ RPC Server: http://127.0.0.1:7545
✅ Ganache restart karein
✅ Contract re-deploy:
   npx hardhat run scripts/deploy.js --network ganache
```

### Masla 6: Dashboard "Error" Dikha Raha

```
Alamaat:
- Surkh "Error" nishan
- Koi transactions nahi

Hal:
✅ Browser refresh (Ctrl+Shift+R)
✅ Backend running?
✅ F12 dabayein → Console check
```

### Masla 7: Transactions Nahi Ho Rahay

```
Alamaat:
- Relay ON hai
- 30 second guzar gaye
- Koi transaction nahi

Hal:
✅ Serial Monitor errors check
✅ WiFi connected?
✅ Backend logs check
```

---

## Aham Malumat (Quick Reference)

### System Timings:

| Kaam | Waqt |
|------|------|
| Sensor reading | Har 0.5 second (500ms) |
| Relay decision | Har 0.5 second |
| Transaction | Har 60 second |
| Dashboard update | Har 0.5 second |
| Dashboard refresh | Har 5 second |

### Units:

| Paimaish | Unit | Misal |
|----------|------|-------|
| Voltage | Volt (V) | 220V |
| Current | Ampere (A) | 1.0A |
| Power | Watt (W) | ~10W |
| Energy | Watt-hour (Wh) | 1.83Wh |
| Price | Rs per kWh | Rs 28 |

### Cost Calculator:

| Waqt | Energy | Qeemat |
|------|--------|--------|
| 30 second | 1.83 Wh | Rs 0.051 |
| 1 ghanta | 10 Wh | Rs 0.28 |
| 1 din (30W avg) | 720 Wh | Rs 20 |
| 1 maheena | 21.6 kWh | Rs 600 |

---

## Ikhtitam (Conclusion)

### Aap Ne Kya Banaya?

✅ **Smart Microgrid** - Khudkar tawanai ka tabadla  
✅ **Blockchain Integration** - Mustaqil, shaffaf record  
✅ **IoT System** - WiFi, sensors, dashboard  
✅ **Web Dashboard** - Real-time monitoring  

### Istemaal Kahan Ho Sakta Hai?

🏘️ **Rehayshi Ilaqe** - Parosiyon mein bijli ka tabadla  
🏭 **Sanaati** - Factories ke liye  
🏢 **Tijarati** - Shopping malls, daftar  
🌾 **Dehati Ilaqe** - Off-grid communities  

### Mustaqbil Ki Behtari:

```
✅ Asli solar panels
✅ Battery backup
✅ Aik se zyada ghar
✅ Mobile app (apni)
✅ AI predictions
✅ Real blockchain (Polygon)
```

---

## Rabta Aur Madad

### Files Ki Location:

```
Project Root/
├── firmware/          → ESP32 code
├── backend/           → Node.js server
├── blockchain/        → Smart contracts
└── docs/              → Tamam documentation
```

### Aham Files:

| File | Maqsad |
|------|--------|
| `house_a_controller.ino` | House A firmware |
| `house_b_controller.ino` | House B firmware |
| `server.js` | Backend server |
| `EnergyTrading.sol` | Smart contract |
| `index.html` | Web dashboard |

---

**Allah Aapko Kamiyabi De!**  
**Best of luck for your FYP! 🎓🚀**

---

**Project Version:** 3.1.0  
**Aakhri Tabdeeli:** 06-February-2026  
**Zubaan:** Roman Urdu + English
