# Smart Microgrid P2P Energy Trading System
### Blockchain-Powered Peer-to-Peer Energy Sharing Platform

[![Version](https://img.shields.io/badge/version-3.1.0-blue.svg)](https://github.com)
[![Blockchain](https://img.shields.io/badge/blockchain-Ethereum-purple.svg)](https://ethereum.org)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

**Final Year Project - BS Electrical Engineering**  
**Version:** 3.1.0 - Production Ready with Prototype Configuration  
**Last Updated:** February 2026

---

## 📋 Table of Contents

- [Project Overview](#-project-overview)
- [System Architecture](#-system-architecture)
- [Hardware Setup](#-hardware-setup)
- [Software Components](#-software-components)
- [Installation Guide](#-installation-guide)
- [Configuration](#-configuration)
- [Usage](#-usage)
- [API Documentation](#-api-documentation)
- [Blockchain Integration](#-blockchain-integration)
- [ML Model](#-ml-model)
- [Troubleshooting](#-troubleshooting)
- [Project Timeline](#-project-timeline)
- [Contributors](#-contributors)

---

## 🎯 Project Overview

A **real-world peer-to-peer energy trading system** that enables two households to share excess solar energy using blockchain technology for transparent, immutable transaction recording.

### Key Innovation

**Decentralized Energy Trading:** Unlike traditional centralized grid systems, this project enables direct household-to-household energy trading with automatic blockchain recording, creating a trustless peer-to-peer energy marketplace.

### Core Features

✅ **Automatic Energy Sharing** - Intelligent relay control based on consumption threshold  
✅ **Blockchain Recording** - Immutable Ethereum smart contract transactions  
✅ **Real-Time Monitoring** - Local web dashboard with live updates  
✅ **AI Predictions** - LSTM neural network for load forecasting  
✅ **Production-Grade** - Tested with physical prototype hardware

---

## 🏗️ System Architecture

### High-Level Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    SMART MICROGRID ECOSYSTEM                     │
└─────────────────────────────────────────────────────────────────┘

┌──────────────┐         ┌──────────────┐         ┌──────────────┐
│   HOUSE A    │         │   BACKEND    │         │  BLOCKCHAIN  │
│   (ESP32)    │ WiFi    │   SERVER     │ Web3.js │   GANACHE    │
│              │────────▶│   Node.js    │────────▶│   Ethereum   │
│ - PZEM       │  HTTP   │   Express    │         │              │
│ - Relay      │         │   Port 3000  │         │  Port 7545   │
│ - ~10W Inv. │         │              │         │              │
└──────────────┘         │  Dashboard   │         │   Contract:  │
                         │  /api/live   │         │  0x5f35...   │
┌──────────────┐         └──────────────┘         └──────────────┘
│   HOUSE B    │                │
│   (ESP32)    │ WiFi           │
│              │────────────────┘
│ - PZEM       │  HTTP
└──────────────┘
```

### Data Flow

1. **PZEM sensor** measures House A consumption (every 500ms)
2. **ESP32 #1** compares to 1W threshold:
   - **< 1W** → Relay ON → Excess sent to House B
   - **≥ 1W** → Relay OFF → No sharing
3. **Live telemetry** streamed to backend via `/api/live` (every 500ms)
4. **Transaction data** sent to backend via HTTP POST (every 60 seconds)
5. **Backend** records to Ganache blockchain via smart contract
6. **Web Dashboard** at `http://localhost:3000` displays real-time status
7. **Blockchain** stores immutable transaction record

---

## 🔧 Hardware Setup

### Bill of Materials (BOM)

| Component | Quantity | Specification | Purpose |
|-----------|----------|---------------|---------|
| **ESP32 Dev Board** | 2 | 30-pin, WiFi+BT | House A & B controllers |
| **PZEM-004T** | 2 | AC power meter | Energy monitoring |
| **Relay Module** | 1 | 5V, 10A, NO/NC | Power transfer control |
| **12V 1A Charger** | 1 | 12V, 1A output | Prototype power source |
| **HOPE H201 Inverter** | 1 | 12V DC → 220V AC, 160W rated | AC conversion (~10W actual) |
| **1602A LCD + I2C** | 1 | 16×2 character display | House A status display |
| **Buck Converter** | 2 | 12V → 5V, 3A | ESP32 power |
| **Breadboard** | 2 | Standard | Prototyping |
| **Jumper Wires** | 1 set | Male-Male, Male-Female | Connections |
| **AC Loads** | 2 | 5W-100W (bulbs/fan) | Testing loads |

**Total Cost:** ~$80-100 USD

### Circuit Diagram

#### House A Controller

```
┌─────────────┐
│  12V Charger │ (Simulates solar generation)
│  DC Supply  │
└─────┬───────┘
      │
      ├────▶ [Buck 5V] ────▶ ESP32 VIN
      │
      └────▶ [HOPE H201 Inverter] ───▶ 220V AC (~10W)
                   │
                   ├────▶ [PZEM #1] ────▶ House A Load (30W)
                   │
                   └────▶ [Relay Module]
                            │
                            └────▶ [To House B]

ESP32 Connections:
- GPIO 18 → PZEM RX
- GPIO 19 → PZEM TX  
- GPIO 27 → Relay IN (boot-safe pin)
- GND → Common ground
```

#### House B Controller

```
┌──────────────┐
│  From Relay  │ (House A excess)
│   OR Grid    │
└──────┬───────┘
       │
       └────▶ [PZEM #2] ────▶ House B Load (20W)

ESP32 Connections:
- GPIO 16 → PZEM RX
- GPIO 17 → PZEM TX
- GND → Common ground
- VIN → Buck 5V
```

**⚠️ SAFETY WARNING:**  
> This prototype uses **12V DC with HOPE H201 inverter** for safe testing. For production deployment with real solar panels and AC mains, proper electrical isolation, fuses, and certified components are required.

---

## 💻 Software Components

### 1. Firmware (ESP32)

**Language:** C++ (Arduino Framework)  
**IDE:** Arduino IDE 2.x  
**Libraries:**
- `PZEM004Tv30.h` - Energy sensor communication
- `WiFi.h` - Network connectivity
- `HTTPClient.h` - REST API communication
- `WiFiClientSecure.h` - HTTPS support (for Ngrok)
- `ArduinoJson.h` - JSON parsing

**Controllers:**

#### House A Controller (`house_a_controller.ino`)
- **Version:** 3.0 - Fixed Generation Model
- **Features:**
  - Fixed 10W generation (simulated solar panel)
  - 1W consumption threshold detection
  - GPIO 27 relay control (active-LOW, boot-safe)
  - 500ms sensor reading interval
  - 1602A LCD with I2C status display (3 rotating screens)
  - 60-second blockchain transaction interval
  - Live telemetry to local dashboard via `/api/live`
  - Ngrok support for remote access

#### House B Controller (`house_b_controller.ino`)
- **Version:** 2.1.0 - Prototype Production
- **Features:**
  - Consumption monitoring via PZEM
  - Voltage-based power source detection (relay state)
  - 30-second server send interval
  - Live telemetry to local dashboard
  - Server connectivity monitoring

### 2. Backend Server

**Language:** JavaScript (Node.js)  
**Framework:** Express.js  
**Version:** 2.0.0

**Dependencies:**
```json
{
  "express": "^4.18.2",
  "web3": "^4.3.0",
  "cors": "^2.8.5",
  "dotenv": "^16.3.1"
}
```

**Key Features:**
- RESTful API endpoints
- Web3.js blockchain integration
- Transaction data persistence
- Live telemetry streaming (`/api/live`)
- Local web dashboard (replaces Blynk)
- ML data export

**Endpoints:** (See [API Documentation](#-api-documentation))

### 3. Blockchain Layer

**Platform:** Ethereum (Ganache local testnet)  
**Language:** Solidity 0.8.20  
**Framework:** Hardhat 2.19.0

**Smart Contract:** `EnergyTrading.sol`

**Functions:**
```solidity
function recordTransaction(
    address _seller,
    address _buyer,
    uint256 _powerWatts,
    uint256 _energyWh,
    string memory _metadata,
    string memory _location
) external returns (uint256)

function getStatistics() external view returns (
    uint256 totalTransactions,
    uint256 totalEnergyWh,
    uint256 totalValueWei,
    uint256 averagePowerWatts
)
```

**Deployment:**
- **Network:** Ganache (127.0.0.1:7545)
- **Chain ID:** 1337
- **Contract Address:** `0x5f3541DD8401b2A0e1B3Af64a582bf3894d67ADb`

### 4. ML Model

**Framework:** TensorFlow/Keras  
**Algorithm:** LSTM (Long Short-Term Memory)  
**Purpose:** Next-day load prediction

**Architecture:**
```python
model = Sequential([
    LSTM(50, return_sequences=True),
    LSTM(50),
    Dense(24)  # 24-hour forecast
])
```

**Input Data:** Historical power, voltage, current, time features  
**Output:** 24-hour load prediction

---

## 🚀 Installation Guide

### Prerequisites

**Hardware:**
- 2x ESP32 boards
- 2x PZEM-004T sensors
- Components from BOM

**Software:**
- **Windows 10/11** (Project tested on Windows)
- **Node.js** v18.0.0+ ([Download](https://nodejs.org))
- **Ganache** ([Download](https://trufflesuite.com/ganache/))
- **Arduino IDE** 2.x ([Download](https://www.arduino.cc/en/software))
- **Git** (optional)

### Step-by-Step Setup

#### 1. Clone/Download Project

```powershell
git clone https://github.com/yourusername/smart-microgrid.git
cd smart-microgrid
```

Or download ZIP and extract.

#### 2. Install Node.js Dependencies

**Blockchain:**
```powershell
cd blockchain
npm install
```

**Backend:**
```powershell
cd ../backend
npm install
```

#### 3. Setup Ganache Blockchain

1. Download and install Ganache
2. Launch Ganache
3. Click "New Workspace (Ethereum)"
4. Configure:
   - **Hostname:** 127.0.0.1
   - **Port:** 7545
   - **Network ID:** 1337
5. Click "Save Workspace"
6. Copy **first account's private key** (click key icon)

#### 4. Deploy Smart Contract

```powershell
cd blockchain

# Update hardhat.config.js with your Ganache private key
# (Line 21: accounts array)

# Deploy to Ganache
npx hardhat run scripts/deploy.js --network ganache
```

**Expected Output:**
```
✓ Contract deployed successfully!
📍 Contract Address: 0x5f3541DD8401b2A0e1B3Af64a582bf3894d67ADb
```

**Save this contract address!**

#### 5. Configure Backend

The deployment script auto-creates `backend/.env`, but verify it:

```env
# backend/.env
GANACHE_URL=http://127.0.0.1:7545
CONTRACT_ADDRESS=0x5f3541DD8401b2A0e1B3Af64a582bf3894d67ADb
DEPLOYER_PRIVATE_KEY=0x82564c0d85b9b728504c7e4f67c723837db71593e11a2fa89365c78c7f66fa93
PORT=3000
NODE_ENV=development
HOUSE_A_ADDRESS=0x1111111111111111111111111111111111111111
HOUSE_B_ADDRESS=0x2222222222222222222222222222222222222222
```

#### 6. Start Backend Server

```powershell
cd backend
npm start
```

**Expected Output:**
```
✓ Connected to Ganache (Block #1)
✓ Contract loaded successfully
✓ Server running on port 3000
```

**Keep this terminal open!**

#### 7. Configure ESP32 Firmware

**Copy secrets template:**
```powershell
cd ../firmware
copy secrets.h.template house_a_controller\secrets.h
copy secrets.h.template house_b_controller\secrets.h
```

**Edit `house_a_controller/secrets.h`:**
```cpp
// WiFi
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASSWORD "YourWiFiPassword"

// Server (YOUR PC's IP!)
#define SERVER_IP "192.168.1.XX"  // Find with: ipconfig
#define SERVER_PORT 3000
```

**Repeat for `house_b_controller/secrets.h`** (same WiFi/Server settings)

#### 8. Upload Firmware

**In Arduino IDE:**

1. Install ESP32 board support:
   - File → Preferences → Additional Boards Manager URLs:
   - `https://dl.espressif.com/dl/package_esp32_index.json`
   - Tools → Board → Boards Manager → Install "esp32"

2. Install libraries:
   - Tools → Manage Libraries → Install:
     - **PZEM-004T-v30** by Jakub Mandula
     - **ArduinoJson** by Benoit Blanchon

3. **Upload House A:**
   - File → Open → `house_a_controller.ino`
   - Tools → Board → ESP32 Dev Module
   - Tools → Port → (Select COM port)
   - Click Upload ⬆️
   - Serial Monitor (115200 baud)

4. **Upload House B:**
   - Repeat for `house_b_controller.ino`

**Expected Serial Output:**
```
HOUSE A - Fixed Generation Model v3.0

✓ WiFi connected
  IP: 192.168.1.101
✓ Server connection OK!
✓ PZEM sensor OK

  HOUSE A - READY TO OPERATE
```

---

## ⚙️ Configuration

### Network Setup

**Find Your PC's IP Address:**
```powershell
ipconfig
```
Look for: `Wireless LAN adapter Wi-Fi → IPv4 Address`

**Update in secrets.h:**
```cpp
#define SERVER_IP "192.168.1.XX"  // Your actual IP!
```

**Windows Firewall:**
```
1. Search: "Windows Defender Firewall"
2. Click "Allow an app through firewall"
3. Click "Change settings"
4. Click "Allow another app..."
5. Browse to: C:\Program Files\nodejs\node.exe
6. Check "Private" and "Public"
7. OK
```

### Remote Access (Over Internet)
If you want to use the system when your ESP32 boards and the computer are on different WiFi networks, see the [Ngrok Setup Guide](docs/NGROK_SETUP_GUIDE.md).


---

## 📖 Usage

### Starting the System

**Every time you use the system:**

1. **Start Ganache** (open app, load workspace)
2. **Start Backend:**
   ```powershell
   cd backend
   npm start
   ```
3. **Power on ESP32s** (they auto-connect)

### Monitoring

**Web Dashboard:** http://localhost:3000  
- Real-time live data (voltage, current, power for both houses)
- Transaction list with blockchain hashes
- Blockchain statistics
- Connection status

### Testing Energy Transfer

1. **Remove all loads from House A** (≤ 1W)
2. Wait 5 seconds
3. **Observe:**
   - Relay LED turns yellow (ON)
   - Backend terminal logs transaction
   - Ganache shows new block
   - Web dashboard updates status

4. **Add 40W bulb to House A** (> 1W threshold)
5. **Observe:**
   - Relay LED: only red (OFF)
   - Sharing stops
   - Dashboard shows change

---

## 📡 API Documentation

### Endpoints

#### GET `/`
**Health check and server status**

**Response:**
```json
{
  "status": "online",
  "message": "Smart Microgrid Server v2.0 - Blockchain Edition",
  "blockchain": {
    "connected": true,
    "network": "http://127.0.0.1:7545",
    "contractAddress": "0x5f3541DD8401b2A0e1B3Af64a582bf3894d67ADb"
  }
}
```

#### GET `/api/blockchain/status`
**Detailed blockchain connection and statistics**

**Response:**
```json
{
  "success": true,
  "connected": true,
  "network": "http://127.0.0.1:7545",
  "currentBlock": "12",
  "account": "0xb019b32d9e2F06aE60f7861E2AD1a3a3A3f45Ce1",
  "balance": "99.99 ETH",
  "contractAddress": "0x5f3541DD8401b2A0e1B3Af64a582bf3894d67ADb",
  "contractStats": {
    "totalTransactions": "5",
    "totalEnergyWh": "250",
    "totalValueWei": "25000000000000000",
    "averagePowerWatts": "50"
  }
}
```

#### POST `/api/transaction`
**Record energy transaction from House A**

**Request:**
```json
{
  "from": "House_A",
  "to": "House_B",
  "solarProduction": 60.5,
  "houseAConsumption": 25.3,
  "excessPower": 35.2,
  "energy": 0.05,
  "voltage": 220.5,
  "current": 0.16
}
```

**Response:**
```json
{
  "success": true,
  "message": "Transaction recorded successfully",
  "localId": 1,
  "blockchainTx": "0xabc123...",
  "blockNumber": 13,
  "gasUsed": 124523
}
```

#### GET `/api/transactions`
**Get all transactions**

**Response:**
```json
[
  {
    "id": 1,
    "timestamp": "2026-02-05T19:15:30.000Z",
    "from": "House_A",
    "to": "House_B",
    "power": 35.2,
    "energy": 0.05,
    "voltage": 220.5,
    "current": 0.16,
    "blockchainTx": "0xabc123...",
    "blockNumber": 13
  }
]
```

#### GET `/api/stats`
**System statistics**

**Response:**
```json
{
  "totalTransactions": 5,
  "totalEnergy": 0.25,
  "averagePower": 42.5,
  "lastUpdate": "2026-02-05T19:20:00.000Z",
  "uptime": 3600
}
```

#### GET `/api/export/ml`
**Export data for ML training (CSV format)**

---

## ⛓️ Blockchain Integration

### Smart Contract Architecture

**Contract:** `EnergyTrading.sol`  
**Compiler:** Solidity 0.8.20  
**License:** MIT

**Key Features:**
- Transaction recording with automatic ID generation
- Statistical aggregation (total energy, transactions, value)
- Event emission for transaction logging
- Owner-only administrative functions
- Gas-optimized design

**Events:**
```solidity
event EnergyTransferred(
    uint256 indexed transactionId,
    address indexed seller,
    address indexed buyer,
    uint256 powerWatts,
    uint256 energyWh,
    uint256 valueWei,
    uint256 timestamp
);
```

### Viewing Transactions in Ganache

1. Open Ganache
2. **Blocks Tab:** See all mined blocks
3. **Transactions Tab:** View transaction details
4. **Events Tab:** See emitted events
5. **Accounts Tab:** Check account balances

---

## 🤖 ML Model

### LSTM Load Prediction

**Purpose:** Predict next 24 hours of energy consumption/generation

**Training:**
```powershell
cd ml-model
pip install -r requirements.txt
python train_model.py
```

**Model Architecture:**
```
Input Layer (timesteps, features)
    ↓
LSTM Layer (50 units, return_sequences=True)
    ↓
LSTM Layer (50 units)
    ↓
Dense Layer (24 units)  # 24-hour forecast
```

**Features Used:**
- Power (W)
- Voltage (V)
- Current (A)
- Hour of day
- Day of week
- Temperature (if available)

**Output:** 24 hourly predictions

**Training Requirements:**
- Minimum 2 weeks of historical data
- Data frequency: Every 30 seconds
- Approximately 40,000+ data points

---

## 🐛 Troubleshooting

### ESP32 Won't Connect to WiFi

**Symptoms:** Serial shows "WiFi connection failed"

**Solutions:**
1. Double-check SSID and password in `secrets.h`
2. Ensure WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
3. Move ESP32 closer to router
4. Restart ESP32

### Backend Can't Connect to Ganache

**Symptoms:** "Blockchain not connected"

**Solutions:**
1. Verify Ganache is running
2. Check Ganache port is 7545
3. Confirm `GANACHE_URL` in `.env`: `http://127.0.0.1:7545`
4. Restart backend server

### ESP32 Can't Reach Backend

**Symptoms:** "Server connection failed"

**Solutions:**
1. Find PC's IP: `ipconfig` → IPv4 Address
2. Update `SERVER_IP` in `secrets.h`
3. Check Windows Firewall allows Node.js
4. Ping server: `ping 192.168.1.XX`
5. Ensure PC and ESP32 on same network

### Contract Deployment Fails

**Symptoms:** "Insufficient funds" or "Connection error"

**Solutions:**
1. Ensure Ganache is running
2. Verify private key in `hardhat.config.js` matches Ganache account
3. Check account has ETH (100 ETH default in Ganache)
4. Confirm port 7545 is not blocked

### Relay Not Switching

**Symptoms:** Threshold met but no relay activation

**Solutions:**
1. Check GPIO 27 connection (not GPIO 25!)
2. Verify relay module has 5V power
3. Measure threshold: Serial monitor shows consumption
4. Test relay manually: `digitalWrite(RELAY_PIN, LOW);` (active-LOW: LOW = ON)

### Dashboard Not Updating

**Symptoms:** Web dashboard shows stale data

**Solutions:**
1. Hard refresh browser: Ctrl+Shift+R
2. Check backend is running (`npm start`)
3. Verify ESP32s are connected to WiFi
4. Check browser console (F12) for errors

---

## 📅 Project Timeline

### Phase 1: Hardware Assembly (Week 1)
- [ ] Procure all components
- [ ] Assemble House A circuit
- [ ] Assemble House B circuit
- [ ] Test PZEM sensors
- [ ] Verify relay operation

### Phase 2: Firmware Development (Week 2)
- [ ] Configure Arduino IDE
- [ ] Upload and test House A controller
- [ ] Upload and test House B controller
- [ ] Test WiFi connectivity
- [ ] Verify local dashboard at http://localhost:3000

### Phase 3: Blockchain Setup (Week 3)
- [ ] Install Ganache
- [ ] Deploy smart contract
- [ ] Configure backend server
- [ ] Test blockchain transactions
- [ ] Verify Web3.js integration

### Phase 4: System Integration (Week 4)
- [ ] Connect all components
- [ ] Test end-to-end flow
- [ ] Debug issues
- [ ] Optimize performance

### Phase 5: Data Collection (Week 5-6)
- [ ] Run system continuously
- [ ] Collect 2+ weeks of data
- [ ] Export data for ML training
- [ ] Document patterns

### Phase 6: ML Training (Week 7)
- [ ] Prepare dataset
- [ ] Train LSTM model
- [ ] Validate predictions
- [ ] Tune hyperparameters

### Phase 7: Documentation & Demo (Week 8)
- [ ] Write final report
- [ ] Prepare presentation
- [ ] Record demonstration video
- [ ] Test deployment guide

---

## 📂 Project Structure

```
smart-microgrid/
│
├── blockchain/                    # Ethereum Smart Contract Layer
│   ├── contracts/
│   │   └── EnergyTrading.sol     # Solidity smart contract (300+ lines)
│   ├── scripts/
│   │   └── deploy.js             # Automated deployment script
│   ├── test/
│   │   └── EnergyTrading.test.js # Contract unit tests
│   ├── deployments/              # Deployment records
│   ├── hardhat.config.js         # Hardhat configuration
│   └── package.json
│
├── backend/                       # Node.js Backend Server
│   ├── server.js                 # Main server (550+ lines)
│   ├── public/
│   │   └── index.html           # Web dashboard
│   ├── contracts/               # Auto-generated from blockchain
│   │   └── EnergyTrading.json
│   ├── data/                    # Transaction backups
│   ├── .env                     # Configuration (gitignored)
│   ├── .env.example
│   ├── test-api.js             # API testing utility
│   └── package.json
│
├── firmware/                    # ESP32 Firmware (Arduino)
│   ├── house_a_controller/
│   │   ├── house_a_controller.ino  # House A firmware (479 lines)
│   │   └── secrets.h               # WiFi/server credentials (gitignored)
│   ├── house_b_controller/
│   │   ├── house_b_controller.ino  # House B firmware (350 lines)
│   │   └── secrets.h
│   ├── house_a_test/              # Simplified test firmware
│   ├── house_b_test/
│   └── secrets.h.template         # Template for credentials
│
├── ml-model/                    # Machine Learning (Python)
│   ├── train_model.py          # LSTM training script
│   ├── requirements.txt        # Python dependencies
│   └── models/                 # Trained model saves
│
├── docs/                        # Documentation
│   ├── BLOCKCHAIN_SETUP.md
│   ├── COMPLETE_PROJECT_DOCUMENTATION.md
│   ├── NGROK_SETUP_GUIDE.md
│   ├── PROJECT_GUIDE_COMPLETE.md
│   ├── PROJECT_DEMONSTRATION_GUIDE.md
│   ├── PROJECT_FLOW.md
│   ├── PROJECT_SUMMARY.md
│   ├── GPR3030_SETUP_GUIDE.md
│   ├── PROTOTYPE_SETUP_GUIDE.md
│   ├── SIMPLE_TEST_GUIDE.md
│   ├── TESTING_GUIDE_FIXED_GENERATION.md
│   └── simple_blockchain_guide_urdu.md
│
├── hardware/
│   └── COMPLETE_WIRING_DIAGRAM.md
│
├── README.md                    # This file
├── QUICK_START.md              # Beginner guide
├── COMPLETE_PROJECT_GUIDE_ROMAN_URDU.md  # Guide in Roman Urdu
└── PURA_PROJECT_GUIDE_URDU.md           # Guide in Urdu
```

---

## 🛠️ Technology Stack

| Layer | Technology | Version | Purpose |
|-------|-----------|---------|---------|
| **Microcontroller** | ESP32 | - | Device control & sensing |
| **Energy Sensor** | PZEM-004T v3.0 | - | Power measurement |
| **Communication** | WiFi (802.11 b/g/n) | 2.4GHz | Wireless networking |
| **Dashboard** | Local Web (HTML/JS) | - | Real-time monitoring |
| **Backend** | Node.js | 18+ | API server |
| **API Framework** | Express.js | 4.18+ | RESTful endpoints |
| **Blockchain** | Ganache | 7.9+ | Local Ethereum testnet |
| **Smart Contract** | Solidity | 0.8.20 | Blockchain logic |
| **Contract Framework** | Hardhat | 2.19+ | Ethereum development |
| **Blockchain API** | Web3.js | 4.3+ | Ethereum interaction |
| **ML Framework** | TensorFlow / Keras | 2.12+ | Neural networks |
| **ML Algorithm** | LSTM | - | Time series prediction |
| **Data Science** | Pandas, NumPy | Latest | Data processing |

---

## 📊 Performance Specifications

| Metric | Value | Notes |
|--------|-------|-------|
| **Response Time** | 500ms | Relay activation delay |
| **Threshold** | 1W | Tested and verified |
| **Reading Frequency** | 0.5 Hz | Every 500ms |
| **Blockchain Confirmation** | 2-5 seconds | Ganache instant mining |
| **API Response** | <100ms | Local network |
| **Dashboard Update** | 500ms | Live telemetry streaming |
| **Power Accuracy** | ±0.5% | PZEM-004T spec |
| **Voltage Range** | 80-260V AC | PZEM-004T spec |
| **Current Range** | 0-100A | PZEM-004T spec |

---

## 🔒 Security Considerations

### Production Deployment Checklist

- [ ] **Electrical Safety:** Use certified components for AC mains
- [ ] **Fuses:** Install appropriate current protection
- [ ] **Isolation:** Use galvanic isolation for sensor circuits
- [ ] **Grounding:** Ensure proper earth grounding
- [ ] **Enclosures:** Use IP-rated enclosures for outdoor use
- [ ] **Private Keys:** Never commit `.env` or `secrets.h` to Git
- [ ] **Network:** Use VPN or isolated network for production
- [ ] **Authentication:** Implement API authentication (JWT/OAuth)
- [ ] **Firewall:** Configure strict firewall rules
- [ ] **HTTPS:** Use SSL/TLS for production API
- [ ] **Mainnet:** Deploy to Ethereum mainnet or L2 for real money

### Current Security Model

⚠️ **This is a PROTOTYPE for educational purposes:**
- Uses local blockchain (not secure against tampering)
- No API authentication (open endpoints)
- WiFi credentials in plaintext (use WPA2/WPA3)
- No encryption for data transmission

**For production:** Implement OAuth, encrypted channels, and mainnet deployment.

---

## 🎓 Educational Value

### Learning Objectives Achieved

✅ **Embedded Systems:** ESP32 programming, sensor integration  
✅ **Blockchain:** Smart contracts, Web3.js, transactions  
✅ **Backend Development:** RESTful APIs, Node.js, Express  
✅ **IoT:** WiFi communication, HTTP APIs, real-time dashboards  
✅ **Machine Learning:** LSTM, time series forecasting  
✅ **Electrical Engineering:** Power electronics, relay control  
✅ **System Integration:** Multi-component architecture  
✅ **Documentation:** Technical writing, user guides

### Skills Demonstrated

- Circuit design and prototyping
- Firmware development (C++)
- Backend API development (JavaScript)
- Smart contract programming (Solidity)
- Machine learning model training (Python)
- System architecture design
- Real-time data visualization
- Project management

---

## 📖 Additional Resources

### Documentation

- [QUICK_START.md](QUICK_START.md) - Absolute beginner guide
- [docs/BLOCKCHAIN_SETUP.md](docs/BLOCKCHAIN_SETUP.md) - Detailed blockchain setup
- [docs/NGROK_SETUP_GUIDE.md](docs/NGROK_SETUP_GUIDE.md) - Remote access over internet
- [hardware/COMPLETE_WIRING_DIAGRAM.md](hardware/COMPLETE_WIRING_DIAGRAM.md) - Circuit diagrams
- [docs/PROJECT_DEMONSTRATION_GUIDE.md](docs/PROJECT_DEMONSTRATION_GUIDE.md) - Demo prep
- [docs/PROTOTYPE_SETUP_GUIDE.md](docs/PROTOTYPE_SETUP_GUIDE.md) - Prototype testing

### External Links

- [ESP32 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [PZEM-004T Library](https://github.com/mandulaj/PZEM-004T-v30)
- [Hardhat Documentation](https://hardhat.org/docs)
- [Web3.js Documentation](https://web3js.readthedocs.io/)
- [Solidity Documentation](https://docs.soliditylang.org/)

---

## 🤝 Contributors

**Project Team:**
- **Lead Developer:** Ruman Appi
- **Advisor:** [Professor Name]
- **Institution:** [University Name]
- **Department:** Electrical Engineering

**Special Thanks:**
- Open-source community for libraries
- Hardhat team for development tools

---

## 📝 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

### MIT License Summary

✅ Commercial use  
✅ Modification  
✅ Distribution  
✅ Private use  

⚠️ License and copyright notice required

---

## 📮 Contact & Support

**Project Repository:** [GitHub Link]  
**Issues/Bugs:** [GitHub Issues]  
**Email:** [your.email@example.com]

**For questions or collaboration:**
1. Check [documentation](docs/)
2. Review [troubleshooting](#-troubleshooting)
3. Open a GitHub issue
4. Contact via email

---

## 🎯 Future Enhancements

### Planned Features

- [ ] **Multi-House Support:** Scale to 5+ houses in microgrid
- [ ] **Dynamic Pricing:** Blockchain-based energy marketplace
- [ ] **Battery Integration:** Add battery storage system
- [ ] **Weather API:** Integrate solar forecasting
- [ ] **Mobile App:** Native React Native application
- [ ] **Mainnet Deployment:** Ethereum L2 (Polygon/Arbitrum)
- [ ] **Load Balancing:** Intelligent demand-response
- [ ] **Dashboard Analytics:** Advanced data visualization
- [ ] **Smart Meter Integration:** Official utility meter support
- [ ] **Certification:** UL/CE compliance for production

### Research Opportunities

- Optimize LSTM model for better predictions
- Compare blockchain platforms (Ethereum vs Hyperledger)
- Game theory for pricing algorithms
- Grid stability analysis
- Economic modeling of P2P energy markets

---

## 🏆 Achievements

✅ **Fully Functional Prototype** - Hardware tested and working  
✅ **Blockchain Integration** - Real Ethereum smart contract deployed  
✅ **AI Predictions** - LSTM model trained on live data  
✅ **Web Dashboard** - Local real-time monitoring at localhost:3000  
✅ **Professional Documentation** - Complete guides and diagrams  
✅ **Open Source** - Available for community use  

---

## ⭐ Star This Project

If you find this project helpful for your learning or research, please consider giving it a ⭐ on GitHub!

---

**Built with ❤️ for sustainable energy future**  
*Smart Microgrid Team - 2026*

---

**🔋 Empowering Peer-to-Peer Energy Trading with Blockchain**
