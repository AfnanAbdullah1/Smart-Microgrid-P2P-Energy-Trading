# پورے پراجیکٹ کی مکمل رہنمائی
# Smart Microgrid - Complete Project Guide in Simple Urdu

**BS Electrical Engineering - Final Year Project**  
**آسان اردو میں مکمل گائیڈ**

---

## فہرست (Table of Contents)

1. [پراجیکٹ کیا ہے؟](#پراجیکٹ-کیا-ہے)
2. [ہارڈویئر کی فہرست](#ہارڈویئر-کی-فہرست)
3. [سافٹ ویئر کی فہرست](#سافٹ-ویئر-کی-فہرست)
4. [انسٹالیشن گائیڈ](#انسٹالیشن-گائیڈ)
5. [وائرنگ ڈایاگرام](#وائرنگ-ڈایاگرام)
6. [سسٹم کیسے چلائیں](#سسٹم-کیسے-چلائیں)
7. [ٹربل شوٹنگ](#ٹربل-شوٹنگ)

---

## پراجیکٹ کیا ہے؟

### آسان الفاظ میں:

یہ پراجیکٹ ایک **Smart Microgrid** ہے جس میں:

🏠 **House A** (بیچنے والا):
- سولر پینل سے ~10W بجلی بن رہی ہے
- گھر میں صرف 1W استعمال ہو رہی ہے
- بچی ہوئی 219W بجلی House B کو بیچ سکتے ہیں

🏠 **House B** (خریدنے والا):
- کوئی سولر پینل نہیں
- بجلی چاہیے
- House A سے خریدتے ہیں

⛓️ **Blockchain**:
- تمام لین دین (transaction) کا ریکارڈ
- کوئی مٹا نہیں سکتا
- کوئی بدل نہیں سکتا
- ہمیشہ کے لیے محفوظ

### اصل زندگی میں استعمال:

```
صبح 9 بجے:
- House A: سولر سے ~10W بن رہی ہے
- House A استعمال: صرف 0.7W (کچھ نہیں چل رہا)
- Relay آٹومیٹک ON ہو جاتی ہے
- 219.3W House B کو چلی جاتی ہے
- House B: پنکھا، لائٹس چل رہے ہیں

30 سیکنڈ بعد:
- Transaction blockchain پہ save ہوتا ہے
- Cost: Rs 0.051 (5 پیسے)
- Block #38 میں permanent record

دوپہر 12 بجے:
- House A: بلب چالو کر دیا (استعمال 5W)
- Relay آٹومیٹک OFF ہو جاتی ہے
- House B کو بجلی نہیں ملتی
- House A اپنی بجلی استعمال کرتا ہے
```

---

## ہارڈویئر کی فہرست

### مکمل لسٹ (قیمتوں کے ساتھ):

| نمبر | چیز کا نام | تعداد | تفصیل | قیمت (روپے) |
|------|-----------|--------|--------|-------------|
| 1 | ESP32 (مائیکرو کنٹرولر) | 2 | WiFi والا | 800 |
| 2 | PZEM-004T (سینسر) | 2 | بجلی ناپنے والا | 1,200 |
| 3 | Relay Module | 1 | آن/آف کرنے والا | 150 |
| 4 | 12V 1A Charger | 1 | 12V بجلی | 500 |
| 5 | Inverter (220V) | 1 | DC سے AC | 1,800 |
| 6 | 1602A LCD + I2C | 1 | House A ڈسپلے | 250 |
| 7 | تاریں (Wires) | - | مختلف | 200 |
| 7 | Breadboard | 2 | کنکشن کے لیے | 300 |
| 8 | USB Cables | 2 | ESP32 کے لیے | 200 |

**کل قیمت: تقریباً Rs 7,150**

### ہر چیز کیا کرتی ہے؟

#### 1. ESP32 (دماغ کی طرح)
```
کام:
✅ WiFi سے جڑتا ہے
✅ سینسر سے ڈیٹا پڑھتا ہے  
✅ Relay کو ON/OFF کرتا ہے
✅ Server کو ڈیٹا بھیجتا ہے
✅ Dashboard کو لائیو ڈیٹا بھیجتا ہے

پن استعمال:
- GPIO 18, 19 → PZEM sensor
- GPIO 27 → Relay control
- 3.3V, GND → Power
```

#### 2. PZEM-004T (میٹر کی طرح)
```
کام:
✅ Voltage ناپتا ہے (Volt)
✅ Current ناپتا ہے (Ampere)
✅ Power calculate کرتا ہے (Watt)
✅ Energy جوڑتا ہے (Wh)

ریڈنگ کی فریکوئنسی:
ہر 0.5 سیکنڈ (500ms)
```

#### 3. Relay (سوئچ کی طرح)
```
کام:
✅ House A سے House B کا رابطہ
✅ آٹومیٹک ON/OFF
✅ ESP32 کنٹرول کرتا ہے

Logic:
- GPIO 27 = LOW → Relay ON (بجلی جا رہی ہے)
- GPIO 27 = HIGH → Relay OFF (بجلی رکی ہوئی ہے)
```

#### 4. 12V 1A Charger (سولر کی جگہ)
```
کام:
✅ 12V DC output
✅ ~10W power (12V × 1A = 12W DC, ~10W AC after inverter)
✅ سولر پینل کی جگہ (prototype)
```

#### 5. Inverter (تبدیلی)
```
کام:
✅ 12V DC → 220V AC
✅ PZEM کے لیے AC voltage
```

---

## سافٹ ویئر کی فہرست

### 1. ESP32 Firmware (Arduino C++)

**لوکیشن:** `firmware/house_a_controller/` اور `firmware/house_b_controller/`

**کام:**
```cpp
✅ WiFi سے جڑنا
✅ PZEM سے ڈیٹا پڑھنا (ہر 0.5 سیکنڈ)
✅ Relay کو ON/OFF کرنا (consumption دیکھ کر)
✅ Server کو transaction بھیجنا (ہر 60 سیکنڈ)
✅ Local dashboard کو اپڈیٹ کرنا (ہر 0.5 سیکنڈ)
```

**Libraries:**
- WiFi.h
- HTTPClient.h
- PZEM004Tv30.h
- WiFiClientSecure.h
- ArduinoJson.h

### 2. Backend Server (Node.js)

**لوکیشن:** `backend/server.js`

**کام:**
```javascript
✅ ESP32 سے transaction receive کرنا
✅ Blockchain پہ save کرنا
✅ Web dashboard چلانا
✅ API endpoints فراہم کرنا
```

**Port:** 3000

**API Endpoints:**
```
POST /api/transaction        → نیا transaction
GET  /api/transactions       → سب transactions
GET  /api/blockchain/status  → blockchain کی حالت
GET  /api/stats              → اعداد و شمار
```

### 3. Blockchain (Ganache + Solidity)

**لوکیشن:** `blockchain/contracts/EnergyTrading.sol`

**کام:**
```solidity
✅ Transactions save کرنا (permanent)
✅ Cost calculate کرنا
✅ Data retrieve کرنا
```

**Contract Address:** `0x5f3541DD8401b2A0e1B3Af64a582bf3894d67ADb`

**Ganache:** http://127.0.0.1:7545

### 4. Web Dashboard

**لوکیشن:** `backend/public/index.html`

**کام:**
```
✅ Real-time monitoring
✅ Transaction history
✅ Statistics
✅ Auto-refresh (ہر 5 سیکنڈ)
```

**URL:** http://localhost:3000

### 5. Local Web Dashboard

**کام:**
```
✅ Live power readings
✅ Relay status
✅ Transaction count
✅ Blockchain status
```

**URL:** http://localhost:3000

---

## انسٹالیشن گائیڈ

### Step 1: Arduino IDE Install کریں

```
1. Download کریں:
   https://www.arduino.cc/en/software

2. Install کریں (Next, Next, Finish)

3. ESP32 board add کریں:
   File → Preferences → Additional Board Manager URLs:
   https://dl.espressif.com/dl/package_esp32_index.json

4. ESP32 install کریں:
   Tools → Board → Boards Manager
   Search: "ESP32"
   Install: "esp32 by Espressif Systems"

5. Libraries install کریں:
   Sketch → Include Library → Manage Libraries
   
   Install کریں:
   ✅ PZEM-004T-v30
   ✅ ArduinoJson
```

### Step 2: Node.js Install کریں

```
1. Download:
   https://nodejs.org/

2. Install (Next, Next, Finish)

3. Check کریں:
   Command Prompt (CMD) کھولیں:
   
   > node --version
   v20.11.1  ✅
   
   > npm --version
   10.2.4  ✅
```

### Step 3: Ganache Install کریں

```
1. Download:
   https://trufflesuite.com/ganache/

2. Install کریں

3. Open کریں:
   - Click "Quickstart"
   - RPC Server: http://127.0.0.1:7545
   - 10 accounts دیکھیں گے (ہر ایک میں 100 ETH)
```

### Step 4: Backend Setup

```
1. Folder open کریں:
   cd "d:\Academic\Paid_Projects_Clients\Ruman Appi - Project _ P2P Current transfering\backend"

2. Install dependencies:
   npm install

3. Wait for completion
   node_modules folder بن جائے گا ✅
```

### Step 5: Smart Contract Deploy کریں

```
1. Ganache چالو رکھیں

2. Deploy کریں:
   cd "d:\...\blockchain"
   npm install
   npx hardhat run scripts/deploy.js --network ganache

3. Contract address نوٹ کریں:
   0x5f35...
```

### Step 6: Backend Configure کریں

```
File: backend/server.js

Line 15-16 میں:
const GANACHE_URL = 'http://127.0.0.1:7545';
const CONTRACT_ADDRESS = '0x5f3541DD8401b2A0e1B3Af64a582bf3894d67ADb';

(اپنا contract address ڈالیں)
```

### Step 7: ESP32 Firmware Configure کریں

```
File: firmware/house_a_controller/house_a_controller.ino

Lines 13-17:
const char* ssid = "APKA_WIFI_NAME";         ← اپنا WiFi
const char* password = "APKA_PASSWORD";       ← اپنا password
const char* serverUrl = "http://192.168.1.19:3000/api/transaction";  ← اپنا IP

house_b_controller میں بھی same کریں!
```

### Step 8: Firmware Upload

```
Arduino IDE میں:

1. house_a_controller.ino کھولیں

2. Tools → Board: "ESP32 Dev Module"

3. Tools → Port: Select (COM3, COM4, etc.)

4. Upload button دبائیں (→)

5. "Done uploading" کا انتظار ✅

6. house_b_controller.ino کے لیے repeat کریں
```

### Step 9: Web Dashboard دیکھیں

```
1. Browser کھولیں

2. جائیں: http://localhost:3000

3. Live readings دیکھیں

4. Transactions کا ریکارڈ دیکھیں
```

---

## وائرنگ ڈایاگرام

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

## سسٹم کیسے چلائیں

### شروع کرنا:

#### 1. Ganache شروع کریں

```
1. Ganache application کھولیں
2. "Quickstart" کلک کریں
3. RPC Server check کریں: http://127.0.0.1:7545 ✅
```

#### 2. Backend Server شروع کریں

```
1. Command Prompt کھولیں:
   cd "d:\...\backend"

2. Server چلائیں:
   npm start

3. دیکھیں:
   ✓ Server running on port 3000
   ✓ Blockchain: Connected
```

#### 3. ESP32s ON کریں

```
1. ESP32 #1 (House A) USB سے جوڑیں
2. ESP32 #2 (House B) USB سے جوڑیں
3. Serial Monitor کھولیں (115200 baud)
4. دیکھیں:
   ✓ WiFi connected
   ✓ Server connection OK!
```

#### 4. Web Dashboard کھولیں

```
1. Browser کھولیں
2. جائیں: http://localhost:3000
3. دیکھیں:
   ✓ Live data (سبز)
   ✓ Contract address
```

### ٹیسٹنگ:

#### Test 1: Low Consumption (Relay ON)

```
1. House A میں کچھ نہ چلائیں (consumption < 1W)
2. Serial Monitor:
   ✓ "Relay ON - Energy flowing"
3. 30 سیکنڈ بعد:
   ✓ "Transaction recorded!"
4. Dashboard:
   ✓ Transaction #1 نظر آئے گا
```

#### Test 2: High Consumption (Relay OFF)

```
1. Bulb ON کریں (consumption > 1W)
2. Serial Monitor:
   ✓ "Relay OFF - No energy transfer"
3. کوئی transaction نہیں
```

---

## ٹربل شوٹنگ

### مسئلہ 1: WiFi نہیں جڑ رہا

```
علامات:
- "Connecting to WiFi..." لگا رہ رہا ہے

حل:
✅ WiFi name/password check کریں
✅ 2.4GHz WiFi ہونی چاہیے (5GHz نہیں)
✅ Router چیک کریں
✅ ESP32 reset کریں
```

### مسئلہ 2: PZEM "NaN" دکھا رہا ہے

```
علامات:
- Voltage: NaN
- Current: NaN

حل:
✅ PZEM wiring check (TX, RX)
✅ 5V power check
✅ AC voltage موجود ہے؟
```

### مسئلہ 3: Relay کام نہیں کر رہا

```
علامات:
- Relay click نہیں ہو رہا
- LED نہیں جل رہا

حل:
✅ Relay wiring check (VCC, GND, IN)
✅ GPIO 27 connection
✅ Active-LOW check (LOW = ON)
```

### مسئلہ 4: Backend نہیں جڑ رہا

```
علامات:
- "Server connection failed"

حل:
✅ Backend running ہے؟
✅ IP address صحیح ہے؟
   CMD: ipconfig
   IPv4 Address نکالیں
✅ Firewall off کریں (testing)
```

### مسئلہ 5: Ganache نہیں جڑ رہا

```
علامات:
- "Error connecting to blockchain"

حل:
✅ Ganache running ہے؟
✅ RPC Server: http://127.0.0.1:7545
✅ Ganache restart کریں
✅ Contract re-deploy:
   npx hardhat run scripts/deploy.js --network ganache
```

### مسئلہ 6: Dashboard "Error" دکھا رہا

```
علامات:
- سرخ "Error" نشان
- کوئی transactions نہیں

حل:
✅ Browser refresh (Ctrl+Shift+R)
✅ Backend running؟
✅ F12 دبائیں → Console check
```

### مسئلہ 7: Transactions نہیں ہو رہے

```
علامات:
- Relay ON ہے
- 30 سیکنڈ گزر گئے
- کوئی transaction نہیں

حل:
✅ Serial Monitor errors check
✅ WiFi connected؟
✅ Backend logs check
```

---

## اہم معلومات (Quick Reference)

### System Timings:

| کام | وقت |
|-----|-----|
| Sensor reading | ہر 0.5 سیکنڈ (500ms) |
| Relay decision | ہر 0.5 سیکنڈ |
| Transaction | ہر 60 سیکنڈ |
| Dashboard update | ہر 0.5 سیکنڈ |
| Dashboard refresh | ہر 5 سیکنڈ |

### Units:

| پیمائش | یونٹ | مثال |
|--------|------|------|
| Voltage | Volt (V) | 220V |
| Current | Ampere (A) | 1.0A |
| Power | Watt (W) | ~10W |
| Energy | Watt-hour (Wh) | 1.83Wh |
| Price | Rs per kWh | Rs 28 |

### Cost Calculator:

| وقت | Energy | قیمت |
|-----|--------|------|
| 30 سیکنڈ | 1.83 Wh | Rs 0.051 |
| 1 گھنٹہ | 10 Wh | Rs 0.28 |
| 1 دن (30W avg) | 720 Wh | Rs 20 |
| 1 مہینہ | 21.6 kWh | Rs 600 |

---

## اختتام (Conclusion)

### آپ نے کیا بنایا؟

✅ **Smart Microgrid** - خودکار توانائی کا تبادلہ  
✅ **Blockchain Integration** - مستقل، شفاف ریکارڈ  
✅ **IoT System** - WiFi، sensors، dashboard  
✅ **Web Dashboard** - real-time monitoring  

### استعمال کہاں ہو سکتا ہے؟

🏘️ **رہائشی علاقے** - پڑوسیوں میں بجلی کا تبادلہ  
🏭 **صنعتی** - فیکٹریوں کے لیے  
🏢 **تجارتی** - شاپنگ مالز، دفاتر  
🌾 **دیہی علاقے** - Off-grid communities  

### مستقبل کی بہتری:

```
✅ اصلی سولر پینلز
✅ بیٹری بیک اپ
✅ ایک سے زیادہ گھر
✅ موبائل ایپ (اپنی)
✅ AI predictions
✅ Real blockchain (Polygon)
```

---

## رابطہ اور مدد

### فائلوں کی لوکیشن:

```
Project Root/
├── firmware/          → ESP32 code
├── backend/           → Node.js server
├── blockchain/        → Smart contracts
└── docs/              → تمام documentation
```

### اہم فائلیں:

| فائل | مقصد |
|------|------|
| `house_a_controller.ino` | House A firmware |
| `house_b_controller.ino` | House B firmware |
| `server.js` | Backend server |
| `EnergyTrading.sol` | Smart contract |
| `index.html` | Web dashboard |

---

**اللہ آپ کو کامیابی دے!**  
**Best of luck for your FYP! 🎓🚀**

---

**Project Version:** 3.1.0  
**آخری تبدیلی:** 06-فروری-2026  
**زبان:** اردو/رومن اردو + انگریزی
