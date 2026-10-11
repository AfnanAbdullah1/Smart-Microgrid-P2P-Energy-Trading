# Smart Microgrid P2P Energy Trading System
## Complete Project Overview & Flow Documentation

---

## 1. System Overview

The Smart Microgrid P2P Energy Trading System enables **peer-to-peer energy trading** between a solar producer (House A) and a consumer (House B), with all transactions recorded on an **immutable blockchain ledger**.

```mermaid
graph TB
    subgraph "House A - Fixed Generation ~10W"
        SP[☀️ 12V Charger]
        INV[DC-AC Inverter]
        PZEM1[PZEM<br/>Consumption]
        ESP1[ESP32 #1]
        RELAY[Relay Module]
        LOAD_A[House A Loads]
    end
    
    subgraph "House B - Consumer"
        PZEM3[PZEM<br/>Consumption]
        ESP2[ESP32 #2]
        LOAD_B[House B Loads]
    end
    
    subgraph "Backend Services"
        DASH[📱 Web Dashboard]
        SERVER[🖥️ Backend Server]
        BC[⛓️ Blockchain<br/>Ganache]
    end
    
    SP --> INV --> LOAD_A
    LOAD_A --> PZEM1
    PZEM1 --> ESP1
    ESP1 --> RELAY
    RELAY -->|Excess Energy| LOAD_B
    
    ESP1 -.->|WiFi| SERVER
    ESP2 -.->|WiFi| SERVER
    SERVER --> BC
    SERVER --> DASH
    
    LOAD_B --> PZEM3
    PZEM3 --> ESP2
```

---

## 2. Energy Flow Diagram

Shows how electricity flows between houses based on solar production.

```mermaid
flowchart LR
    subgraph SOLAR["☀️ Fixed Generation ~10W"]
        S1[12V Charger + HOPE H201 Inverter]
    end
    
    subgraph HOUSE_A["🏠 House A"]
        A1[House A Loads]
        P1[PZEM Sensor]
    end
    
    subgraph DECISION["⚡ Consumption Check"]
        D1{"Consumption < 1W?"}
    end
    
    subgraph HOUSE_B["🏠 House B"]
        B1[House B Loads]
    end
    
    S1 -->|220V AC| A1
    A1 --> P1
    P1 -->|Measure| D1
    D1 -->|YES: Relay ON| B1
    D1 -->|NO: Relay OFF| A1
```

---

## 3. Data Flow Architecture

```mermaid
flowchart TB
    subgraph ESP["ESP32 Microcontrollers"]
        E1[House A ESP32]
        E2[House B ESP32]
    end
    
    subgraph SENSORS["PZEM-004T Sensors"]
        P1[House A Consumption]
        P2[House B Consumption]
    end
    
    subgraph BACKEND["Backend Server"]
        API[REST API<br/>Express.js]
        W3[Web3.js]
        JSON[JSON Backup]
        DASH[Web Dashboard]
    end
    
    subgraph BLOCKCHAIN["Ganache Blockchain"]
        SC[Smart Contract<br/>EnergyTrading.sol]
        TX[Transaction Log]
    end
    
    subgraph ML["Machine Learning"]
        LSTM[LSTM Model<br/>Load Prediction]
    end
    
    P1 --> E1
    P2 --> E2
    
    E1 -->|HTTP POST| API
    E2 -->|HTTP POST| API
    
    API --> W3
    API --> JSON
    API --> DASH
    W3 --> SC
    SC --> TX
    
    JSON --> LSTM
```

---

## 4. Transaction Recording Flow

Step-by-step flow of how energy transactions are recorded on blockchain.

```mermaid
sequenceDiagram
    participant ESP as ESP32 (House A)
    participant API as Backend Server
    participant W3 as Web3.js
    participant BC as Blockchain
    participant SC as Smart Contract
    
    ESP->>ESP: Read PZEM Sensor
    ESP->>ESP: Check Consumption vs 1W Threshold
    
    alt Consumption < 1W
        ESP->>ESP: Activate Relay (LOW)
        ESP->>API: POST /api/transaction (every 60s)
        Note over ESP,API: {power: 220, energy: 25, from: "House_A"}
        
        API->>W3: Connect to Ganache
        W3->>SC: recordTransaction()
        SC->>SC: Update Statistics
        SC->>BC: Emit EnergyTransferred Event
        BC-->>W3: Transaction Hash
        W3-->>API: TX Receipt
        API-->>ESP: {blockchainTx: "0x..."}
        
        ESP->>ESP: Update Serial Monitor
    else Consumption >= 1W
        ESP->>ESP: Deactivate Relay (HIGH)
    end
```

---

## 5. Smart Contract Functions

```mermaid
classDiagram
    class EnergyTrading {
        +address owner
        +uint256 transactionCounter
        +uint256 defaultPricePerKwh
        +Transaction[] transactions
        +mapping houses
        
        +recordTransaction(seller, buyer, power, energy)
        +recordSimpleTransaction(power, energy)
        +getTransaction(id)
        +getLatestTransaction()
        +getStatistics()
        +getHouseInfo(address)
        +registerHouse(name, isSeller)
        +setPricePerKwh(price)
    }
    
    class Transaction {
        +uint256 id
        +address seller
        +address buyer
        +uint256 powerWatts
        +uint256 energyWh
        +uint256 pricePerKwh
        +uint256 totalCost
        +uint256 timestamp
    }
    
    class House {
        +string name
        +address wallet
        +bool isRegistered
        +bool isSeller
        +uint256 totalEnergyTraded
        +uint256 transactionCount
    }
    
    EnergyTrading --> Transaction : contains
    EnergyTrading --> House : manages
```

---

## 6. Relay Control Logic

```mermaid
stateDiagram-v2
    [*] --> STANDBY: System Start
    
    STANDBY --> TRANSFERRING: Consumption < 1W
    TRANSFERRING --> STANDBY: Consumption >= 1W
    
    state STANDBY {
        [*] --> Monitoring
        Monitoring --> Monitoring: Read Sensors (500ms)
    }
    
    state TRANSFERRING {
        [*] --> RelayON
        RelayON --> SendingData: Every 60s
        SendingData --> RelayON: Continue
    }
    
    note right of STANDBY: Relay OFF (HIGH)\nNo energy transfer
    note right of TRANSFERRING: Relay ON (LOW)\nEnergy flowing to House B
```

---

## 7. API Endpoints

```mermaid
graph LR
    subgraph CLIENT["Clients"]
        E1[ESP32 House A]
        E2[ESP32 House B]
        WEB[Web Dashboard]
    end
    
    subgraph API["REST API - Port 3000"]
        R1["GET /"]
        R2["GET /api/blockchain/status"]
        R3["POST /api/transaction"]
        R4["POST /api/consumption"]
        R5["GET /api/transactions"]
        R6["GET /api/stats"]
        R7["GET /api/export/ml"]
    end
    
    E1 --> R3
    E2 --> R4
    WEB --> R1
    WEB --> R2
    WEB --> R5
    WEB --> R6
    WEB --> R7
```

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Server health & blockchain status |
| `/api/blockchain/status` | GET | Detailed blockchain info |
| `/api/transaction` | POST | Record energy transaction |
| `/api/consumption` | POST | Record House B data |
| `/api/transactions` | GET | List all transactions |
| `/api/stats` | GET | Energy statistics |
| `/api/export/ml` | GET | Export for ML training |

---

## 8. Local Web Dashboard

**URL:** `http://localhost:3000`

The web dashboard is served by the backend server and displays:
- Live voltage, current, power for House A and House B
- Relay status (ON/OFF)
- Transaction count and blockchain status
- Recent transaction history with blockchain hashes

Data is streamed via the `/api/live` endpoint every 500ms from both ESP32s.

---

## 9. ML Prediction Pipeline

```mermaid
flowchart LR
    subgraph DATA["Data Collection"]
        D1[Transaction JSON]
        D2[Consumption JSON]
    end
    
    subgraph PREPROCESS["Preprocessing"]
        P1[Load Data]
        P2[Feature Extraction]
        P3[Normalization]
        P4[Sequence Creation]
    end
    
    subgraph MODEL["LSTM Model"]
        M1[LSTM Layer 1<br/>128 units]
        M2[Dropout 20%]
        M3[LSTM Layer 2<br/>64 units]
        M4[Dense Layers]
        M5[Output: 24h Prediction]
    end
    
    subgraph OUTPUT["Predictions"]
        O1[Solar Generation<br/>Next 24 Hours]
        O2[Consumption<br/>Next 24 Hours]
    end
    
    D1 --> P1
    D2 --> P1
    P1 --> P2 --> P3 --> P4
    P4 --> M1 --> M2 --> M3 --> M4 --> M5
    M5 --> O1
    M5 --> O2
```

---

## 10. Hardware Wiring Summary

```mermaid
graph TB
    subgraph ESP32_A["ESP32 #1 - House A"]
        GPIO18_A[GPIO18 RX]
        GPIO19_A[GPIO19 TX]
        GPIO27_A[GPIO27]
    end
    
    subgraph SENSORS_A["Sensor"]
        PZEM1[PZEM - House A Consumption]
    end
    
    subgraph RELAY["Relay Module"]
        R1[IN]
        R2[COM/NO]
    end
    
    PZEM1 -->|TX| GPIO18_A
    PZEM1 -->|RX| GPIO19_A
    GPIO27_A --> R1
    
    subgraph ESP32_B["ESP32 #2 - House B"]
        GPIO16_B[GPIO16 RX]
        GPIO17_B[GPIO17 TX]
    end
    
    subgraph PZEM_B["PZEM - House B Consumption"]
        P3[PZEM]
    end
    
    P3 -->|TX| GPIO16_B
    P3 -->|RX| GPIO17_B
```

---

## 11. Project File Structure

```
📁 Smart-Microgrid-P2P/
├── 📁 blockchain/           ← Ethereum Smart Contract
│   ├── 📁 contracts/
│   │   └── 📄 EnergyTrading.sol
│   ├── 📁 scripts/
│   │   └── 📄 deploy.js
│   ├── 📁 test/
│   │   └── 📄 EnergyTrading.test.js
│   ├── 📄 hardhat.config.js
│   └── 📄 package.json
│
├── 📁 backend/              ← Node.js API Server
│   ├── 📄 server.js         (Web3.js + Express)
│   ├── 📄 package.json
│   ├── 📄 .env.example
│   └── 📄 test-api.js
│
├── 📁 firmware/             ← ESP32 Arduino Code
│   ├── 📄 secrets.h.template
│   ├── 📁 house_a_controller/
│   │   └── 📄 house_a_controller.ino
│   └── 📁 house_b_controller/
│       └── 📄 house_b_controller.ino
│
├── 📁 ml-model/             ← AI Load Prediction
│   ├── 📄 train_model.py
│   └── 📄 requirements.txt
│
├── 📁 hardware/             ← Wiring Diagrams
│   └── 📄 COMPLETE_WIRING_DIAGRAM.md
│
├── 📁 docs/                 ← Documentation
│   ├── 📄 BLOCKCHAIN_SETUP.md
│   ├── 📄 NGROK_SETUP_GUIDE.md
│   └── 📄 PROJECT_FLOW.md  (This file)
│
├── 📄 README.md
└── 📄 QUICK_START.md
```

---

## 12. Technology Stack

| Layer | Technology | Purpose |
|-------|------------|---------|
| **Hardware** | ESP32, PZEM-004T | Sensing & Control |
| **Firmware** | Arduino C++ | Embedded Logic |
| **Communication** | WiFi, HTTP | Data Transfer |
| **Backend** | Node.js, Express | API Server |
| **Blockchain** | Ganache, Solidity | Transaction Ledger |
| **Blockchain API** | Web3.js | Smart Contract Calls |
| **Dashboard** | Local Web (HTML/JS) | Real-time Monitoring |
| **Machine Learning** | TensorFlow, LSTM | Load Prediction |

---

## 13. Quick Reference Commands

```powershell
# Start Blockchain
cd blockchain && npx hardhat run scripts/deploy.js --network ganache

# Start Server
cd backend && npm start

# Run Tests
cd blockchain && npm test

# Test API
cd backend && node test-api.js
```

---

*Smart Microgrid P2P Energy Trading System - January 2026*
