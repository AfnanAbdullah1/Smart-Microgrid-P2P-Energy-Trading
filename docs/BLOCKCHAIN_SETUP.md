# Blockchain Setup Guide

This guide walks you through setting up the Ganache blockchain for the Smart Microgrid P2P Energy Trading System.

## Prerequisites

- **Node.js** v16 or higher ([Download](https://nodejs.org/))
- **Ganache** - Local Ethereum blockchain ([Download](https://trufflesuite.com/ganache/))
- **Git** (optional, for cloning)

## Step 1: Install Ganache

### Option A: Ganache GUI (Recommended for Beginners)

1. Download Ganache from: https://trufflesuite.com/ganache/
2. Install and launch Ganache
3. Click **"Quickstart"** to create a new workspace
4. Note these settings:
   - **RPC Server**: `http://127.0.0.1:7545`
   - **Network ID**: `1337`
   - **Accounts**: 10 pre-funded accounts with 100 ETH each

### Option B: Ganache CLI (For Advanced Users)

```powershell
# Install Ganache CLI globally
npm install -g ganache

# Start Ganache
ganache --port 7545 --networkId 1337
```

## Step 2: Install Blockchain Dependencies

```powershell
# Navigate to blockchain directory
cd blockchain

# Install dependencies
npm install

# Compile smart contract
npm run compile
```

You should see:
```
Compiled 1 Solidity file successfully
```

## Step 3: Deploy Smart Contract

### Make sure Ganache is running first!

```powershell
# Deploy to Ganache
npm run deploy:ganache
```

Expected output:
```
============================================================
Smart Microgrid - Blockchain Deployment
============================================================

📡 Deploying to network: ganache
👤 Deployer address: 0x...
💰 Deployer balance: 100 ETH

🚀 Deploying EnergyTrading contract...
✅ Contract deployed successfully!
📍 Contract Address: 0x...

📁 Deployment info saved
📄 Contract ABI copied to backend
⚙️  Backend .env file created

============================================================
✅ Deployment Complete!
============================================================
```

## Step 4: Start Backend Server

```powershell
# Navigate to backend directory
cd ../backend

# Install dependencies
npm install

# Start server
npm start
```

Expected output:
```
==================================================
Smart Microgrid Server v2.0
==================================================
✓ Server running on port 3000
✓ Blockchain: Connected
==================================================
```

## Step 5: Verify Blockchain Connection

### Check Server Status

Open browser: http://localhost:3000/

You should see:
```json
{
  "status": "online",
  "message": "Smart Microgrid Server v2.0 - Blockchain Edition",
  "blockchain": {
    "connected": true,
    "network": "http://127.0.0.1:7545",
    "contractAddress": "0x..."
  }
}
```

### Check Blockchain Status

Open browser: http://localhost:3000/api/blockchain/status

```json
{
  "success": true,
  "connected": true,
  "network": "http://127.0.0.1:7545",
  "currentBlock": "5",
  "account": "0x...",
  "balance": "99.99 ETH",
  "contractStats": {
    "totalTransactions": "0",
    "totalEnergyWh": "0"
  }
}
```

## Step 6: Test Transaction Recording

### Using cURL

```powershell
curl -X POST http://localhost:3000/api/transaction `
  -H "Content-Type: application/json" `
  -d '{
    "from": "House_A",
    "to": "House_B",
    "solarProduction": 500,
    "houseAConsumption": 200,
    "excessPower": 300,
    "power": 300,
    "energy": 25
  }'
```

### Expected Response

```json
{
  "success": true,
  "message": "Transaction recorded",
  "transactionId": 1,
  "blockchainTx": "0x1234...abcd",
  "blockNumber": "6",
  "gasUsed": "125000"
}
```

### Verify in Ganache

1. Open Ganache GUI
2. Click on **"Transactions"** tab
3. You should see the transaction with:
   - Contract creation transaction
   - Your test transaction

## Troubleshooting

### "Blockchain not connected"

1. Make sure Ganache is running
2. Check if port 7545 is correct
3. Verify firewall isn't blocking connections

```powershell
# Test Ganache connection
curl http://127.0.0.1:7545
```

### "Contract ABI not found"

Run deployment again:
```powershell
cd blockchain
npm run deploy:ganache
```

### "Insufficient funds"

This shouldn't happen with Ganache (accounts have 100 ETH each).
If it does, restart Ganache with fresh accounts.

### "Private key error"

Update the private key in `blockchain/hardhat.config.js`:
1. Open Ganache GUI
2. Click the key icon next to any account
3. Copy the private key
4. Paste into `hardhat.config.js`

## Smart Contract Functions

### Recording Transactions

The `EnergyTrading.sol` contract provides:

| Function | Description |
|----------|-------------|
| `recordTransaction()` | Full transaction with all details |
| `recordSimpleTransaction()` | Quick transaction with just power/energy |
| `getTransaction(id)` | Get transaction by ID |
| `getLatestTransaction()` | Get most recent transaction |
| `getStatistics()` | Get total stats |
| `getHouseInfo(address)` | Get house details |

### Events

The contract emits events for real-time monitoring:

- `EnergyTransferred` - When energy is traded
- `TransactionRecorded` - When transaction is on blockchain
- `HouseRegistered` - When new house joins

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                         ESP32                                │
│                    (House A / House B)                       │
└─────────────────────────┬───────────────────────────────────┘
                          │ HTTP POST
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                    Backend Server                            │
│                  (Node.js + Express)                         │
│                                                              │
│  ┌─────────────────┐    ┌─────────────────────────────────┐ │
│  │   REST API      │    │       Web3.js                   │ │
│  │  /api/transaction│───▶│  contract.methods.record...()  │ │
│  └─────────────────┘    └───────────────┬─────────────────┘ │
└─────────────────────────────────────────┼───────────────────┘
                                          │ Ethereum TX
                                          ▼
┌─────────────────────────────────────────────────────────────┐
│                      Ganache                                 │
│               (Local Ethereum Blockchain)                    │
│                                                              │
│  ┌─────────────────────────────────────────────────────────┐│
│  │              EnergyTrading.sol                          ││
│  │  - recordTransaction()                                  ││
│  │  - getStatistics()                                      ││
│  │  - Events: EnergyTransferred, TransactionRecorded       ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

## Next Steps

1. **Upload Firmware**: Flash updated firmware to ESP32s
2. **Open Web Dashboard**: Monitor system at http://localhost:3000
3. **Collect Data**: Run system to gather energy data
4. **Train AI Model**: Train LSTM for load predictions

---

*Smart Microgrid P2P Energy Trading System - Blockchain Edition*
*January 2026*
