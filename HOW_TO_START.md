# 🚀 How To Start — P2P Energy Trading System
### Complete Implementation & Startup Guide

**Project:** Blockchain-Based Peer-to-Peer Energy Trading System  
**Version:** 3.1.0 — Production Ready  
**Last Updated:** April 2026

---

## 📋 Table of Contents

- [System Overview](#-system-overview)
- [Prerequisites](#-prerequisites)
- [First-Time Setup](#-first-time-setup)
- [Daily Startup (Every Time)](#-daily-startup-every-time)
- [Testing & Verification](#-testing--verification)
- [Hardware Wiring Reference](#-hardware-wiring-reference)
- [Network Architecture](#-network-architecture)
- [Troubleshooting](#-troubleshooting)

---

## 🎯 System Overview

This system has **4 layers** that need to run together:

```
┌─────────────────────────────────────────────────────────────────────┐
│                         SYSTEM LAYERS                               │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  LAYER 1: BLOCKCHAIN          Ganache (Local Ethereum testnet)      │
│           Port 7545           Stores energy transactions            │
│                                                                     │
│  LAYER 2: BACKEND SERVER      Node.js + Express                     │
│           Port 3000           Receives data, records to blockchain  │
│                                                                     │
│  LAYER 3: NGROK TUNNEL        Public URL → localhost:3000           │
│           Fixed Domain         Bridges ESP32s to server             │
│                                                                     │
│  LAYER 4: HARDWARE            2x ESP32 + 2x PZEM + Relay + LCD     │
│           WiFi: GoogleTalk    Measures & controls energy flow       │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Data Flow

```
House A ESP32                                           House B ESP32
    │                                                       │
    │  Reads PZEM sensor (every 500ms)                      │  Reads PZEM sensor (every 500ms)
    │  Controls relay (threshold: 1W)                       │  Detects relay state via voltage
    │                                                       │
    └──── HTTPS via Ngrok ────┐         ┌── HTTPS via Ngrok ┘
                              │         │
                        ┌─────▼─────────▼─────┐
                        │   Backend Server     │
                        │   (Your PC)          │
                        │   localhost:3000      │
                        └─────────┬────────────┘
                                  │
                        ┌─────────▼────────────┐
                        │   Ganache Blockchain  │
                        │   localhost:7545      │
                        │   Smart Contract      │
                        └──────────────────────┘
```

---

## 📦 Prerequisites

### Software Required on Your PC

| Software | Version | Download Link | Purpose |
|----------|---------|---------------|---------|
| **Node.js** | v18+ | [nodejs.org](https://nodejs.org) | Backend server runtime |
| **Ganache** | Latest | [trufflesuite.com/ganache](https://trufflesuite.com/ganache/) | Local Ethereum blockchain |
| **Arduino IDE** | 2.x | [arduino.cc](https://www.arduino.cc/en/software) | ESP32 firmware upload |
| **ngrok** | Latest | Already in project folder (`ngrok.exe`) | Public tunnel for ESP32 access |

### Verify Installation

Open PowerShell and run:

```powershell
node --version     # Should show v18.x.x or higher
npm --version      # Should show v9.x.x or higher
```

### Hardware Required

| Component | Qty | Connected To |
|-----------|-----|-------------|
| ESP32 Dev Board (30-pin) | 2 | House A & House B |
| PZEM-004T (AC Power Meter) | 2 | One per house |
| Relay Module (5V, Active-LOW) | 1 | House A ESP32 (GPIO 27) |
| 12V 1A DC Charger | 1 | Power source (simulates solar) |
| HOPE H201 Inverter (12V→220V) | 1 | Converts DC to AC (~10W) |
| 1602A LCD + I2C Backpack | 1 | House A ESP32 (GPIO 21/22) |
| Buck Converter (12V→5V) | 2 | Powers each ESP32 |
| AC Loads (bulbs/fan) | 2 | Testing loads for each house |

---

## 🔧 First-Time Setup

> **Do this section ONCE.** After first-time setup, skip to [Daily Startup](#-daily-startup-every-time).

---

### Step 1: Install Node.js Dependencies

Open PowerShell:

```powershell
# Install blockchain dependencies
cd "d:\Academic\Paid_Projects_Clients\Ruman Appi - Project _ P2P Current transfering\blockchain"
npm install

# Install backend dependencies
cd "..\backend"
npm install
```

---

### Step 2: Setup Ganache Blockchain

1. Open **Ganache** application
2. Click **"New Workspace (Ethereum)"**
3. Configure workspace settings:
   - **Workspace Name:** `Smart Microgrid`
   - **Hostname:** `127.0.0.1`
   - **Port Number:** `7545`
   - **Network ID:** `1337`
4. Click **"Save Workspace"**
5. You will see **10 accounts**, each with **100 ETH**
6. Click the **🔑 key icon** on the **first account** → Copy the private key

---

### Step 3: Deploy Smart Contract

#### 3a. Update Private Key

Open `blockchain/hardhat.config.js` and paste your Ganache private key:

```js
// Line 20-23:
accounts: [
  "0xYOUR_GANACHE_FIRST_ACCOUNT_PRIVATE_KEY"
]
```

#### 3b. Deploy

```powershell
cd "d:\Academic\Paid_Projects_Clients\Ruman Appi - Project _ P2P Current transfering\blockchain"
npx hardhat run scripts/deploy.js --network ganache
```

**Expected output:**
```
✓ Contract deployed successfully!
📍 Contract Address: 0x5f3541DD8401b2A0e1B3Af64a582bf3894d67ADb
```

#### 3c. Verify Backend Configuration

The deploy script auto-creates `backend/.env`. Verify it has the correct values:

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

---

### Step 4: Setup Arduino IDE

#### 4a. Add ESP32 Board Support

1. Open Arduino IDE
2. Go to `File → Preferences`
3. In **"Additional Boards Manager URLs"**, add:
   ```
   https://dl.espressif.com/dl/package_esp32_index.json
   ```
4. Go to `Tools → Board → Boards Manager`
5. Search **"esp32"** → Click **Install**

#### 4b. Install Required Libraries

Go to `Tools → Manage Libraries` and install:

| Library | Author | Used By |
|---------|--------|---------|
| **PZEM-004T-v30** | Jakub Mandula | Both Houses |
| **ArduinoJson** | Benoit Blanchon | Both Houses |
| **LiquidCrystal I2C** | Frank de Brabander | House A only |

---

### Step 5: Configure WiFi & Server (secrets.h)

Both ESP32 boards use the same WiFi and server configuration.

Edit `firmware/house_a_controller/secrets.h` AND `firmware/house_b_controller/secrets.h`:

```cpp
// ==================== WIFI CONFIGURATION ====================
#define WIFI_SSID "GoogleTalk"           // ESP32 WiFi name (always same)
#define WIFI_PASSWORD "abid@1947dnp"     // ESP32 WiFi password

// ==================== SERVER CONFIGURATION ====================
// Local IP (not used when Ngrok is active, but keep as fallback)
#define SERVER_IP "192.168.1.13"
#define SERVER_PORT 3000

// Ngrok Public URL (ACTIVE — handles changing PC WiFi/IP)
#define NGROK_URL "https://proreduction-unevangelical-suzan.ngrok-free.dev"
```

> **Why Ngrok?**  
> Your PC connects to different WiFi networks at different places, so its local IP keeps changing.  
> But the ESP32 boards always connect to the same WiFi (`GoogleTalk`).  
> Ngrok gives a **fixed public URL** that always routes to your backend, no matter what WiFi your PC is on.  
> This way, you **never need to change secrets.h or re-upload firmware** when switching networks.

---

### Step 6: Upload Firmware to ESP32 Boards

#### Upload House A Controller

1. Connect **House A ESP32** to PC via USB
2. Open `firmware/house_a_controller/house_a_controller.ino` in Arduino IDE
3. Set: `Tools → Board → ESP32 Dev Module`
4. Set: `Tools → Port → COM__` (select the correct COM port)
5. Click **Upload ⬆️**
6. Open **Serial Monitor** (baud rate: `115200`)
7. Verify output:
   ```
   HOUSE A - Fixed Generation Model v3.0
   ✓ WiFi connected
     IP: 192.168.1.xxx
   ✓ Server connection OK!
   ✓ PZEM sensor OK
     HOUSE A - READY TO OPERATE
   ```

#### Upload House B Controller

1. Disconnect House A, connect **House B ESP32** via USB
2. Open `firmware/house_b_controller/house_b_controller.ino`
3. Same board/port settings
4. Click **Upload ⬆️**
5. Verify Serial Monitor:
   ```
   Smart Microgrid - House B Controller v2.0
   ✓ WiFi Connected!
     IP Address: 192.168.1.xxx
   ✓ PZEM #2 (House B Consumption) OK
   ✓ Server connected!
   System Ready!
   ```

> **After uploading once, the firmware stays on the ESP32 permanently — even after power-off. You only re-upload if you change the code or WiFi credentials.**

---

## ⚡ Daily Startup (Every Time)

After first-time setup is complete, this is all you need to do **each time** you want to run the system:

```
┌─────────────────────────────────────────────────────────────┐
│                    DAILY STARTUP CHECKLIST                   │
│                                                             │
│  Step 1:  Open Ganache           (load your workspace)      │
│  Step 2:  Start Backend          (npm start)                │
│  Step 3:  Start Ngrok            (with fixed domain)        │
│  Step 4:  Power ON ESP32 boards  (auto-connect)             │
│  Step 5:  Open Dashboard         (localhost:3000)            │
│                                                             │
│  Total time: ~1 minute                                      │
└─────────────────────────────────────────────────────────────┘
```

### Step 1: Start Ganache

1. Open **Ganache** application
2. Click on your **"Smart Microgrid"** workspace
3. Wait for accounts to load

### Step 2: Start Backend Server

Open **PowerShell Terminal #1** (keep it open):

```powershell
cd "d:\Academic\Paid_Projects_Clients\Ruman Appi - Project _ P2P Current transfering\backend"
npm start
```

**You should see:**
```
✓ Connected to Ganache (Block #XX)
✓ Contract loaded successfully
==================================================
Smart Microgrid Server v2.0
==================================================
✓ Server running on port 3000
✓ Blockchain: Connected
==================================================
```

> ⚠️ **Keep this terminal open!** Closing it stops the server.

### Step 3: Start Ngrok Tunnel

Open **PowerShell Terminal #2** (keep it open):

```powershell
cd "d:\Academic\Paid_Projects_Clients\Ruman Appi - Project _ P2P Current transfering"
.\ngrok.exe http --domain=proreduction-unevangelical-suzan.ngrok-free.dev 3000
```

**You should see:**
```
Session Status    online
Account           your-email@example.com
Forwarding        https://proreduction-unevangelical-suzan.ngrok-free.dev → http://localhost:3000
```

> ⚠️ **Keep this terminal open too!** This is the bridge between ESP32s and your server.
>
> **Important:** Use `--domain=` flag to use your fixed domain. Without it, ngrok generates a random URL that won't match your firmware.

### Step 4: Power ON ESP32 Boards

1. Power ON **House A ESP32** (USB or 5V buck converter from 12V supply)
2. Power ON **House B ESP32** (USB or 5V supply)
3. Wait ~5 seconds — they auto-connect to WiFi and start sending data

### Step 5: Open Dashboard

Open browser → **http://localhost:3000**

You should see live data from both houses updating every 500ms.

---

## ✅ Testing & Verification

### Test 1: Relay Turns ON (Energy Sharing Starts)

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Remove all loads from House A inverter output | Consumption drops to < 1W |
| 2 | Wait 2-5 seconds | Relay clicks ON |
| 3 | Check House A Serial Monitor | `"LOW CONSUMPTION DETECTED → Relay ON"` |
| 4 | Check House A LCD | Shows `R:ON` |
| 5 | Check Dashboard | Relay status = ON, House B starts receiving |
| 6 | Wait 60 seconds | Transaction appears with blockchain TX hash |
| 7 | Check Ganache | New block mined with transaction |

### Test 2: Relay Turns OFF (Energy Sharing Stops)

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Plug a 5W+ load into House A output | Consumption rises above 1W |
| 2 | Wait 2-5 seconds | Relay clicks OFF |
| 3 | Check Serial Monitor | `"HIGH CONSUMPTION DETECTED → Relay OFF"` |
| 4 | Check LCD | Shows `R:OF` |
| 5 | Check Dashboard | Relay status = OFF, sharing stops |

### Test 3: Verify Blockchain Recording

1. Open **Ganache** → **Transactions** tab
2. You should see `recordTransaction` calls
3. Click on a transaction to see details (gas used, block number)
4. On the dashboard, transactions show blockchain TX hashes

### Test 4: Verify API Endpoints

Open these URLs in your browser:

| URL | Shows |
|-----|-------|
| `http://localhost:3000` | Web Dashboard |
| `http://localhost:3000/api/blockchain/status` | Blockchain connection info |
| `http://localhost:3000/api/transactions` | All recorded transactions |
| `http://localhost:3000/api/stats` | Energy trading statistics |
| `http://localhost:3000/api/live` | Real-time sensor data |

---

## 🔌 Hardware Wiring Reference

### House A — Complete Wiring

```
12V 1A DC Charger
    │
    ├──→ Buck Converter (12V → 5V) ──→ ESP32 VIN
    │
    └──→ HOPE H201 Inverter (12V → 220V AC, ~10W)
              │
              ├──→ PZEM-004T #1 ──→ House A Load (bulb/fan)
              │     │
              │     ├── PZEM TX → ESP32 GPIO 18
              │     └── PZEM RX → ESP32 GPIO 19
              │
              └──→ Relay Module (NO terminal)
                    │
                    ├── Relay IN → ESP32 GPIO 27
                    ├── Relay VCC → 5V
                    ├── Relay GND → GND
                    │
                    └── NO output → Wire to House B PZEM input

ESP32 I2C LCD Connections:
    ├── SDA → GPIO 21
    └── SCL → GPIO 22
    LCD I2C Address: 0x27
```

### House B — Complete Wiring

```
Power from House A Relay (when ON)
    │
    └──→ PZEM-004T #2 ──→ House B Load (bulb/fan)
          │
          ├── PZEM TX → ESP32 GPIO 16
          └── PZEM RX → ESP32 GPIO 17

ESP32 powered by separate Buck Converter (12V → 5V)
Built-in LED on GPIO 2 (blinks faster when receiving energy)
```

### Pin Summary Table

| ESP32 Pin | House A Function | House B Function |
|-----------|-----------------|-----------------|
| GPIO 18 | PZEM TX (receive) | — |
| GPIO 19 | PZEM RX (send) | — |
| GPIO 16 | — | PZEM TX (receive) |
| GPIO 17 | — | PZEM RX (send) |
| GPIO 27 | Relay IN (control) | — |
| GPIO 21 | LCD SDA | — |
| GPIO 22 | LCD SCL | — |
| GPIO 2 | — | Built-in LED |
| VIN | 5V from buck converter | 5V from buck converter |
| GND | Common ground | Common ground |

> ⚠️ **Safety Note:** This prototype uses 12V DC with inverter for safe testing. For production with real solar panels and AC mains, use proper electrical isolation, fuses, and certified components.

---

## 🌐 Network Architecture

### Why Ngrok?

```
THE PROBLEM:
┌──────────┐     ┌──────────┐     ┌──────────┐
│ ESP32s   │     │ Your PC  │     │ ESP32s   │
│ WiFi:    │     │ WiFi:    │     │ WiFi:    │
│GoogleTalk│     │ Home     │     │GoogleTalk│
│          │  ✗  │ IP: ???  │  ✗  │          │
│ Always   │─────│ Changes  │─────│ Always   │
│ same IP  │     │ every    │     │ same IP  │
│          │     │ location │     │          │
└──────────┘     └──────────┘     └──────────┘

THE SOLUTION (Ngrok):
┌──────────┐     ┌──────────┐     ┌──────────┐
│ ESP32s   │     │  Ngrok   │     │ Your PC  │
│          │────▶│  Cloud   │────▶│          │
│ Fixed    │HTTPS│ Fixed URL│Tunnel│ Any WiFi │
│ WiFi     │     │ proredu- │     │ Any IP   │
│          │     │ ction... │     │          │
│          │     │ .dev     │     │ Port 3000│
└──────────┘     └──────────┘     └──────────┘
```

### Current Network Configuration

| Component | Network | Address |
|-----------|---------|---------|
| **Ganache** | localhost | `http://127.0.0.1:7545` |
| **Backend Server** | localhost | `http://localhost:3000` |
| **Ngrok Tunnel** | Internet | `https://proreduction-unevangelical-suzan.ngrok-free.dev` |
| **ESP32 House A** | WiFi: GoogleTalk | DHCP (auto) |
| **ESP32 House B** | WiFi: GoogleTalk | DHCP (auto) |
| **Web Dashboard** | Your browser | `http://localhost:3000` |

### Data Flow with Ngrok

```
ESP32 (House A)
    │
    │ POST /api/live (every 500ms)
    │ POST /api/transaction (every 60s when relay ON)
    │
    ▼
Ngrok Cloud (proreduction-unevangelical-suzan.ngrok-free.dev)
    │
    │ Tunnels to localhost:3000
    │
    ▼
Backend Server (your PC, port 3000)
    │
    │ Web3.js call
    │
    ▼
Ganache Blockchain (your PC, port 7545)
    │
    │ Smart contract records transaction
    │
    ▼
Immutable transaction stored on blockchain ✓
```

---

## ❌ Troubleshooting

### Startup Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| `npm start` fails with "Cannot find module" | Dependencies not installed | Run `npm install` in the `backend` folder |
| "Blockchain not connected" | Ganache not running | Open Ganache and load workspace first |
| "Contract not found" or ABI error | New Ganache workspace created | Redeploy: `cd blockchain && npx hardhat run scripts/deploy.js --network ganache` |
| Ngrok shows "ERR_NGROK_8012" | Another ngrok session active | Close the other ngrok session or use the dashboard at `dashboard.ngrok.com` |

### ESP32 Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| WiFi won't connect | Wrong credentials or 5GHz WiFi | Check `secrets.h` — must be 2.4GHz WiFi |
| "Server connection failed" | Ngrok not running | Start ngrok with the fixed domain command |
| PZEM reads 0 or NaN | Wiring issue or no AC load | Check TX/RX wires. PZEM needs AC power flowing through it |
| Relay won't switch | Wrong GPIO or wiring | Must be GPIO 27. Test: `digitalWrite(27, LOW);` — LOW = ON |

### Dashboard Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| Dashboard shows stale data | ESP32s not connected | Check ESP32 Serial Monitor for WiFi/server status |
| No transactions showing | Relay hasn't been ON for 60s | Wait 60 seconds with relay ON for first transaction |
| Blockchain TX shows "null" | Ganache/contract issue | Check Ganache is running and contract is deployed |

### Ngrok Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| Random URL instead of fixed domain | Missing `--domain` flag | Use: `.\ngrok.exe http --domain=proreduction-unevangelical-suzan.ngrok-free.dev 3000` |
| "Command not found" | Not in right directory | Navigate to project root where `ngrok.exe` is located |
| ESP32 gets HTTPS errors | Certificate issue | Firmware already uses `client.setInsecure()` — this is handled |

---

## 📂 Key Files Reference

```
Project Root/
│
├── backend/
│   ├── .env                    ← Blockchain config (contract address, private key)
│   ├── server.js               ← Main backend server (588 lines)
│   ├── package.json            ← Node.js dependencies
│   ├── public/index.html       ← Web dashboard UI
│   ├── contracts/
│   │   └── EnergyTrading.json  ← Smart contract ABI (auto-generated)
│   └── data/
│       ├── transactions.json   ← Local transaction backup
│       └── consumption.json    ← House B consumption data (for ML)
│
├── blockchain/
│   ├── contracts/
│   │   └── EnergyTrading.sol   ← Solidity smart contract
│   ├── scripts/
│   │   └── deploy.js           ← Deployment script
│   ├── hardhat.config.js       ← Ganache private key goes here
│   └── deployments/
│       └── ganache.json        ← Deployment record
│
├── firmware/
│   ├── house_a_controller/
│   │   ├── house_a_controller.ino  ← House A firmware (561 lines)
│   │   └── secrets.h               ← WiFi + Ngrok URL config
│   ├── house_b_controller/
│   │   ├── house_b_controller.ino  ← House B firmware (393 lines)
│   │   └── secrets.h               ← WiFi + Ngrok URL config
│   └── secrets.h.template          ← Template for new setups
│
├── ml-model/
│   ├── train_model.py          ← LSTM prediction model
│   └── requirements.txt       ← Python dependencies
│
├── ngrok.exe                   ← Ngrok tunnel executable
└── HOW_TO_START.md             ← THIS FILE
```

---

## 🔑 Credentials Quick Reference

| Credential | Value | Where Used |
|-----------|-------|------------|
| **WiFi SSID** | `GoogleTalk` | `firmware/*/secrets.h` |
| **WiFi Password** | `abid@1947dnp` | `firmware/*/secrets.h` |
| **Ngrok Domain** | `proreduction-unevangelical-suzan.ngrok-free.dev` | `firmware/*/secrets.h` |
| **Backend Port** | `3000` | `backend/.env` |
| **Ganache Port** | `7545` | `backend/.env`, `blockchain/hardhat.config.js` |
| **Contract Address** | `0x5f3541DD8401b2A0e1B3Af64a582bf3894d67ADb` | `backend/.env` |
| **Ganache Private Key** | `0x82564c...66fa93` | `backend/.env`, `blockchain/hardhat.config.js` |

> ⚠️ **Never share secrets.h or .env files publicly. They contain private keys and passwords.**

---

## 📊 System Specifications

| Parameter | Value |
|-----------|-------|
| Fixed Generation | 10W (~0.045A at 220V) |
| Relay Threshold | < 1W consumption → Relay ON |
| Sensor Read Interval | 500ms |
| Live Telemetry Interval | 500ms |
| Transaction Interval | 60 seconds |
| House B Server Send | 30 seconds |
| Blockchain Confirmation | 2-5 seconds (Ganache) |
| Smart Contract | Solidity 0.8.20 |
| Relay Logic | Active-LOW (LOW = ON, HIGH = OFF) |
| Relay Pin | GPIO 27 (boot-safe) |
| LCD Address | 0x27 (I2C) |

---

*Smart Microgrid P2P Energy Trading System — BS Electrical Engineering FYP*  
*Built with ❤️ for sustainable energy future*
