/*
 * Smart Microgrid - Backend Server v2.0
 * 
 * This Node.js server:
 * 1. Connects to Ganache blockchain via Web3.js
 * 2. Receives energy transaction data from ESP32
 * 3. Records transactions to smart contract on blockchain
 * 4. Stores backup data in JSON file
 * 5. Provides API for dashboard and ML training
 * 
 * Author: Smart Microgrid Project
 * Version: 2.0.0 - Blockchain Integration
 */

require('dotenv').config();
const express = require('express');
const cors = require('cors');
const fs = require('fs');
const path = require('path');
const { Web3 } = require('web3');

// ==================== CONFIGURATION ====================

const app = express();
const PORT = process.env.PORT || 3000;

// Blockchain configuration
const GANACHE_URL = process.env.GANACHE_URL || 'http://127.0.0.1:7545';
const CONTRACT_ADDRESS = process.env.CONTRACT_ADDRESS || '';
const PRIVATE_KEY = process.env.DEPLOYER_PRIVATE_KEY || '';

// House addresses (predefined in smart contract)
const HOUSE_A_ADDRESS = process.env.HOUSE_A_ADDRESS || '0x1111111111111111111111111111111111111111';
const HOUSE_B_ADDRESS = process.env.HOUSE_B_ADDRESS || '0x2222222222222222222222222222222222222222';

// Blockchain scaling factors — Solidity uses uint256 (integers only).
// We multiply before storing and divide after reading to preserve decimals.
const POWER_SCALE  = 100;   // store centi-watts  (10.0 W  → 1000)
const ENERGY_SCALE = 1000;  // store milli-watt-hours (0.167 Wh → 167)

// Transactions with id <= this value were stored WITHOUT scaling.
// Read the current count once at startup and freeze it.
let SCALING_CUTOFF_ID = 0;  // set after blockchain init

// Address-to-name mapping for human-readable display
const ADDRESS_NAMES = {
  [HOUSE_A_ADDRESS.toLowerCase()]: 'House A',
  [HOUSE_B_ADDRESS.toLowerCase()]: 'House B',
};

// ==================== MIDDLEWARE ====================

app.use(cors());
app.use(express.json());
app.use(express.static('public'));

// ==================== DATA STORAGE (BACKUP) ====================

let transactions = [];
const DATA_DIR = path.join(__dirname, 'data');
const DATA_FILE = path.join(DATA_DIR, 'transactions.json');
const SENSOR_LOG_FILE = path.join(DATA_DIR, 'sensor_log.csv');  // ← ML training data

// Ensure data directory exists
if (!fs.existsSync(DATA_DIR)) {
  fs.mkdirSync(DATA_DIR, { recursive: true });
}

// Create CSV header if file doesn't exist yet
if (!fs.existsSync(SENSOR_LOG_FILE)) {
  const csvHeader = 'timestamp,hour,day_of_week,' +
    'houseA_voltage,houseA_current,houseA_power,houseA_energy,houseA_frequency,houseA_pf,' +
    'houseB_voltage,houseB_current,houseB_power,houseB_energy,' +
    'relay_state,receiving_from_A,solar_generation\n';
  fs.writeFileSync(SENSOR_LOG_FILE, csvHeader);
  console.log('✓ Created sensor_log.csv for ML data collection');
}

// Load existing transactions
if (fs.existsSync(DATA_FILE)) {
  try {
    const data = fs.readFileSync(DATA_FILE, 'utf8');
    transactions = JSON.parse(data);
    console.log(`✓ Loaded ${transactions.length} existing transactions from backup`);
  } catch (error) {
    console.log('⚠️  Could not load existing transactions');
    transactions = [];
  }
}

// Live tracking data for instantaneous dashboard
let liveData = {
  houseA: {
    voltage: 0, current: 0, power: 0, energy: 0,
    solarProduction: 10, // ~10W from 12V 1A charger + HOPE H201 inverter
    relayState: false, manualMode: false
  },
  houseB: {
    voltage: 0, current: 0, power: 0, energy: 0,
    receivingFromHouseA: false
  },
  lastUpdate: Date.now()
};

// ==================== BLOCKCHAIN CONNECTION ====================

let web3;
let contract;
let account;
let blockchainConnected = false;

async function initBlockchain() {
  console.log('\n🔗 Connecting to blockchain...');
  console.log(`   Ganache URL: ${GANACHE_URL}`);

  try {
    // Initialize Web3
    web3 = new Web3(GANACHE_URL);

    // Test connection
    const blockNumber = await web3.eth.getBlockNumber();
    console.log(`✓ Connected to Ganache (Block #${blockNumber})`);

    // Get accounts
    const accounts = await web3.eth.getAccounts();
    if (accounts.length === 0) {
      throw new Error('No accounts found in Ganache');
    }
    account = accounts[0];
    console.log(`✓ Using account: ${account}`);

    // Get balance
    const balance = await web3.eth.getBalance(account);
    console.log(`✓ Account balance: ${web3.utils.fromWei(balance, 'ether')} ETH`);

    // Load contract
    if (CONTRACT_ADDRESS) {
      await loadContract();
    } else {
      console.log('⚠️  No contract address configured');
      console.log('   Run: cd ../blockchain && npm run deploy:ganache');
    }

    blockchainConnected = true;

  } catch (error) {
    console.log(`⚠️  Blockchain connection failed: ${error.message}`);
    console.log('   Make sure Ganache is running on port 7545');
    blockchainConnected = false;
  }
}

async function loadContract() {
  console.log(`\n📄 Loading contract at: ${CONTRACT_ADDRESS}`);

  // Try to load ABI from contracts folder
  const abiPath = path.join(__dirname, 'contracts', 'EnergyTrading.json');

  if (!fs.existsSync(abiPath)) {
    console.log('⚠️  Contract ABI not found');
    console.log('   Run deployment script first: cd ../blockchain && npm run deploy:ganache');
    return;
  }

  const contractData = JSON.parse(fs.readFileSync(abiPath, 'utf8'));
  contract = new web3.eth.Contract(contractData.abi, CONTRACT_ADDRESS);

  // Test contract connection
  try {
    const txCount = await contract.methods.getTransactionCount().call();
    SCALING_CUTOFF_ID = Number(txCount);  // everything up to here is old (unscaled)
    console.log(`✓ Contract loaded successfully`);
    console.log(`✓ Existing blockchain transactions: ${txCount}`);
    console.log(`✓ Scaling cutoff set at TX #${SCALING_CUTOFF_ID}`);
  } catch (error) {
    console.log(`⚠️  Contract call failed: ${error.message}`);
  }
}

// ==================== API ENDPOINTS ====================

// Health check
app.get('/', (req, res) => {
  res.json({
    status: 'online',
    message: 'Smart Microgrid Server v2.0 - Blockchain Edition',
    blockchain: {
      connected: blockchainConnected,
      network: GANACHE_URL,
      contractAddress: CONTRACT_ADDRESS || 'Not deployed'
    },
    statistics: {
      localTransactions: transactions.length,
      uptime: process.uptime()
    }
  });
});

// Blockchain status
app.get('/api/blockchain/status', async (req, res) => {
  if (!blockchainConnected) {
    return res.json({
      success: false,
      connected: false,
      message: 'Blockchain not connected'
    });
  }

  try {
    const blockNumber = await web3.eth.getBlockNumber();
    const balance = await web3.eth.getBalance(account);

    let contractStats = null;
    if (contract) {
      const stats = await contract.methods.getStatistics().call();
      contractStats = {
        totalTransactions: stats._totalTransactions.toString(),
        totalEnergyWh: stats._totalEnergyWh.toString(),
        totalValueWei: stats._totalValueWei.toString(),
        averagePowerWatts: stats._averagePowerWatts.toString()
      };
    }

    res.json({
      success: true,
      connected: true,
      network: GANACHE_URL,
      currentBlock: blockNumber.toString(),
      account: account,
      balance: web3.utils.fromWei(balance, 'ether') + ' ETH',
      contractAddress: CONTRACT_ADDRESS,
      contractStats: contractStats
    });
  } catch (error) {
    res.status(500).json({
      success: false,
      error: error.message
    });
  }
});

// Receive transaction from ESP32 (House A)
app.post('/api/transaction', async (req, res) => {
  try {
    const {
      from = 'House_A',
      to = 'House_B',
      solarProduction = 0,
      houseAConsumption = 0,
      excessPower = 0,
      power = excessPower,
      energy = 0,
      voltage = 0,
      current = 0
    } = req.body;

    // Calculate energy in Wh (if not provided)
    const energyWh = energy || Math.round(power * 30 / 3600); // 30 second interval

    // Create local transaction record
    const localTransaction = {
      id: transactions.length + 1,
      timestamp: new Date().toISOString(),
      from,
      to,
      power: parseFloat(power) || 0,
      energy: parseFloat(energyWh) || 0,
      voltage: parseFloat(voltage) || 0,
      current: parseFloat(current) || 0,
      solarProduction: parseFloat(solarProduction) || 0,
      houseAConsumption: parseFloat(houseAConsumption) || 0,
      blockchainTx: null,
      blockNumber: null,
      gasUsed: null
    };

    console.log(`\n📊 New Transaction from ${from}`);
    console.log(`   Power: ${power} W, Energy: ${energyWh} Wh`);

    // Record to blockchain if connected
    if (blockchainConnected && contract) {
      try {
        console.log('   ⛓️  Recording to blockchain...');

        // Scale up to integers — Solidity only stores uint256
        const powerScaled  = Math.round(parseFloat(power)    * POWER_SCALE);
        const energyScaled = Math.round(parseFloat(energyWh) * ENERGY_SCALE);
        console.log(`   Scaled for blockchain: power=${powerScaled} cW, energy=${energyScaled} mWh`);

        const tx = await contract.methods.recordTransaction(
          HOUSE_A_ADDRESS,
          HOUSE_B_ADDRESS,
          powerScaled,
          energyScaled,
          `Solar: ${solarProduction}W`,
          `Consumption: ${houseAConsumption}W`
        ).send({
          from: account,
          gas: 3000000
        });

        localTransaction.blockchainTx = tx.transactionHash;
        localTransaction.blockNumber = tx.blockNumber.toString();
        localTransaction.gasUsed = tx.gasUsed.toString();

        console.log(`   ✓ Blockchain TX: ${tx.transactionHash}`);
        console.log(`   ✓ Block Number: ${tx.blockNumber}`);
        console.log(`   ✓ Gas Used: ${tx.gasUsed}`);

      } catch (blockchainError) {
        console.log(`   ⚠️  Blockchain error: ${blockchainError.message}`);
        localTransaction.blockchainError = blockchainError.message;
      }
    } else {
      console.log('   ⚠️  Blockchain not connected - saving locally only');
    }

    // Save to local backup
    transactions.push(localTransaction);
    saveTransactions();

    res.json({
      success: true,
      message: 'Transaction recorded',
      transactionId: localTransaction.id,
      blockchainTx: localTransaction.blockchainTx,
      blockNumber: localTransaction.blockNumber,
      gasUsed: localTransaction.gasUsed
    });

  } catch (error) {
    console.error('Error processing transaction:', error);
    res.status(500).json({
      success: false,
      message: 'Error recording transaction',
      error: error.message
    });
  }
});

// Receive consumption data from House B
app.post('/api/consumption', async (req, res) => {
  try {
    const {
      house = 'House_B',
      power = 0,
      energy = 0,
      voltage = 0,
      current = 0,
      receivingFromHouseA = false,
      totalReceived = 0
    } = req.body;

    console.log(`\n📊 Consumption Data from ${house}`);
    console.log(`   Power: ${power} W, Receiving from House A: ${receivingFromHouseA}`);

    // Store consumption data (for ML training)
    const consumptionRecord = {
      id: `consumption_${Date.now()}`,
      timestamp: new Date().toISOString(),
      house,
      power: parseFloat(power) || 0,
      energy: parseFloat(energy) || 0,
      voltage: parseFloat(voltage) || 0,
      current: parseFloat(current) || 0,
      receivingFromHouseA,
      totalReceived: parseFloat(totalReceived) || 0
    };

    // Save to separate file for ML training
    saveConsumptionData(consumptionRecord);

    res.json({
      success: true,
      message: 'Consumption data recorded',
      recordId: consumptionRecord.id
    });

  } catch (error) {
    console.error('Error processing consumption data:', error);
    res.status(500).json({
      success: false,
      message: 'Error recording consumption',
      error: error.message
    });
  }
});

// High-speed live fast-streaming endpoint
app.post('/api/live', (req, res) => {
  const data = req.body;

  if (data.source === 'House_A') {
    liveData.houseA = { ...liveData.houseA, ...data };
  } else if (data.source === 'House_B') {
    liveData.houseB = { ...liveData.houseB, ...data };
  }

  liveData.lastUpdate = Date.now();

  // ── Append to sensor_log.csv for ML training ──────────────────
  // Write one row every time we receive data from EITHER house.
  // This gives continuous readings regardless of relay state.
  try {
    const now = new Date();
    const A = liveData.houseA;
    const B = liveData.houseB;
    const row = [
      now.toISOString(),
      now.getHours(),
      now.getDay(),
      (A.voltage   || 0).toFixed(2),
      (A.current   || 0).toFixed(4),
      (A.power     || 0).toFixed(2),
      (A.energy    || 0).toFixed(4),
      (A.frequency || 0).toFixed(1),
      (A.pf        || 0).toFixed(3),
      (B.voltage   || 0).toFixed(2),
      (B.current   || 0).toFixed(4),
      (B.power     || 0).toFixed(2),
      (B.energy    || 0).toFixed(4),
      A.relayState ? 1 : 0,
      B.receivingFromHouseA ? 1 : 0,
      10  // fixed solar generation (W)
    ].join(',') + '\n';

    fs.appendFileSync(SENSOR_LOG_FILE, row);
  } catch (csvErr) {
    // Non-fatal — don't let CSV error break the live endpoint
    console.error('CSV log error:', csvErr.message);
  }
  // ─────────────────────────────────────────────────────────────

  res.json({ success: true });
});

app.get('/api/live', (req, res) => {
  res.json({
    success: true,
    data: liveData
  });
});

// Get all transactions
app.get('/api/transactions', async (req, res) => {
  try {
    // Get blockchain transactions if connected
    let blockchainTransactions = [];

    if (blockchainConnected && contract) {
      console.log('Fetching transaction count from blockchain...');
      const count = await contract.methods.getTransactionCount().call();
      console.log(`Total transactions in contract: ${count}`);

      for (let i = 1; i <= Number(count); i++) {
        const tx = await contract.methods.getTransaction(i).call();
        const txId = Number(tx.id);
        const isOld = txId <= SCALING_CUTOFF_ID;  // old = stored without scaling

        const rawPower  = Number(tx.powerWatts);
        const rawEnergy = Number(tx.energyWh);

        // Convert back to real units
        const powerW  = isOld ? rawPower                : rawPower  / POWER_SCALE;
        const energyWh = isOld ? rawEnergy              : rawEnergy / ENERGY_SCALE;

        // Map addresses to human-readable names
        const sellerName = ADDRESS_NAMES[tx.seller.toLowerCase()] || tx.seller;
        const buyerName  = ADDRESS_NAMES[tx.buyer.toLowerCase()]  || tx.buyer;

        blockchainTransactions.push({
          id: txId,
          from: sellerName,
          to: buyerName,
          power: parseFloat(powerW.toFixed(2)),
          energy: parseFloat(energyWh.toFixed(4)),
          timestamp: new Date(Number(tx.timestamp) * 1000).toISOString(),
          totalCost: tx.totalCost.toString(),
          blockNumber: null  // not returned by getTransaction, but kept for shape
        });
      }
    }

    res.json({
      success: true,
      source: blockchainConnected ? 'blockchain' : 'local',
      count: blockchainConnected ? blockchainTransactions.length : transactions.length,
      transactions: blockchainConnected ? blockchainTransactions : transactions
    });
  } catch (error) {
    console.error('Error fetching transactions:', error.message);
    res.status(500).json({
      success: false,
      error: error.message,
      transactions: []
    });
  }
});

// Get latest transaction
app.get('/api/transaction/latest', async (req, res) => {
  try {
    if (blockchainConnected && contract) {
      const count = await contract.methods.getTransactionCount().call();
      if (count > 0) {
        const latest = await contract.methods.getLatestTransaction().call();
        const txId = Number(latest.id);
        const isOld = txId <= SCALING_CUTOFF_ID;
        const rawPower  = Number(latest.powerWatts);
        const rawEnergy = Number(latest.energyWh);
        const powerW   = isOld ? rawPower  : rawPower  / POWER_SCALE;
        const energyWh = isOld ? rawEnergy : rawEnergy / ENERGY_SCALE;

        const sellerName = ADDRESS_NAMES[latest.seller.toLowerCase()] || latest.seller;
        const buyerName  = ADDRESS_NAMES[latest.buyer.toLowerCase()]  || latest.buyer;

        return res.json({
          success: true,
          source: 'blockchain',
          transaction: {
            id: txId,
            from: sellerName,
            to: buyerName,
            power: parseFloat(powerW.toFixed(2)),
            energy: parseFloat(energyWh.toFixed(4)),
            timestamp: new Date(Number(latest.timestamp) * 1000).toISOString()
          }
        });
      }
    }

    if (transactions.length === 0) {
      return res.json({
        success: false,
        message: 'No transactions yet'
      });
    }

    res.json({
      success: true,
      source: 'local',
      transaction: transactions[transactions.length - 1]
    });
  } catch (error) {
    res.status(500).json({
      success: false,
      error: error.message
    });
  }
});

// Get statistics
app.get('/api/stats', async (req, res) => {
  try {
    let stats = {
      totalTransactions: 0,
      totalEnergy: 0,
      averagePower: 0,
      totalValue: 0,
      source: 'local'
    };

    if (blockchainConnected && contract) {
      const blockchainStats = await contract.methods.getStatistics().call();

      // The on-chain totals are a MIX of old (raw) and new (scaled) values.
      // For a correct running total we re-compute from local backup which has
      // the true floating-point values — this is the authoritative source.
      const localTotalEnergyWh = transactions.reduce((s, t) => s + (t.energy || 0), 0);
      const localTotalPower    = transactions.reduce((s, t) => s + (t.power  || 0), 0);

      stats = {
        totalTransactions: Number(blockchainStats._totalTransactions),
        totalEnergy: parseFloat((localTotalEnergyWh / 1000).toFixed(3)),  // Wh → kWh
        averagePower: transactions.length > 0
          ? parseFloat((localTotalPower / transactions.length).toFixed(2))
          : 0,
        totalValue: web3.utils.fromWei(blockchainStats._totalValueWei.toString(), 'ether'),
        source: 'blockchain'
      };
    } else if (transactions.length > 0) {
      const totalEnergy = transactions.reduce((sum, t) => sum + (t.energy || 0), 0);
      const totalPower = transactions.reduce((sum, t) => sum + (t.power || 0), 0);
      stats = {
        totalTransactions: transactions.length,
        totalEnergy: (totalEnergy / 1000).toFixed(3),
        averagePower: (totalPower / transactions.length).toFixed(2),
        totalValue: (totalEnergy / 1000 * 0.10).toFixed(4),
        source: 'local'
      };
    }

    res.json({
      success: true,
      stats
    });
  } catch (error) {
    res.status(500).json({
      success: false,
      error: error.message
    });
  }
});

// Export transactions as JSON for ML
app.get('/api/export/ml', (req, res) => {
  const mlData = transactions.map(t => ({
    timestamp: t.timestamp,
    power: t.power,
    energy: t.energy,
    voltage: t.voltage,
    current: t.current,
    solarProduction: t.solarProduction,
    houseAConsumption: t.houseAConsumption,
    hour: new Date(t.timestamp).getHours(),
    dayOfWeek: new Date(t.timestamp).getDay()
  }));

  res.json({
    success: true,
    count: mlData.length,
    data: mlData
  });
});

// Download the full sensor_log.csv for ML training
app.get('/api/export/csv', (req, res) => {
  if (!fs.existsSync(SENSOR_LOG_FILE)) {
    return res.status(404).json({ success: false, message: 'No sensor log yet. Run the system first.' });
  }
  res.setHeader('Content-Type', 'text/csv');
  res.setHeader('Content-Disposition', 'attachment; filename="sensor_log.csv"');
  res.sendFile(SENSOR_LOG_FILE);
});

// ==================== AI / ML PREDICTION PROXY ====================
// These endpoints proxy requests to the Flask ML server (port 5000)

const ML_SERVER_URL = process.env.ML_SERVER_URL || 'http://127.0.0.1:5000';
let mlServerConnected = false;
let latestPrediction = null;
let predictionHistory = [];
const PREDICTIONS_FILE = path.join(DATA_DIR, 'predictions.json');

// Load existing prediction history
if (fs.existsSync(PREDICTIONS_FILE)) {
  try {
    predictionHistory = JSON.parse(fs.readFileSync(PREDICTIONS_FILE, 'utf8'));
    // Keep only last 500
    if (predictionHistory.length > 500) {
      predictionHistory = predictionHistory.slice(-500);
    }
    console.log(`✓ Loaded ${predictionHistory.length} prediction history records`);
  } catch (e) {
    predictionHistory = [];
  }
}

// Helper: call the ML Flask server
async function callMLServer(endpoint, method = 'GET', body = null) {
  try {
    const options = {
      method,
      headers: { 'Content-Type': 'application/json' },
    };
    if (body) options.body = JSON.stringify(body);

    const response = await fetch(`${ML_SERVER_URL}${endpoint}`, options);
    if (!response.ok) {
      throw new Error(`ML server returned ${response.status}`);
    }
    mlServerConnected = true;
    return await response.json();
  } catch (error) {
    mlServerConnected = false;
    throw error;
  }
}

// Poll ML server for predictions every 5 seconds
async function pollPredictions() {
  try {
    const result = await callMLServer('/predict/latest');
    if (result.success) {
      latestPrediction = {
        ...result,
        polled_at: new Date().toISOString(),
      };

      // Store in history
      predictionHistory.push({
        timestamp: result.timestamp,
        predicted: result.predicted_power_watts,
        actual: result.current_power_watts,
        confidence: result.confidence,
        relay_prediction: result.relay_prediction,
      });

      // Trim history
      if (predictionHistory.length > 500) {
        predictionHistory = predictionHistory.slice(-500);
      }

      // Save periodically (every 10 predictions)
      if (predictionHistory.length % 10 === 0) {
        savePredictions();
      }
    }
  } catch (error) {
    // Silently retry — ML server may not be running
  }
}

function savePredictions() {
  try {
    fs.writeFileSync(PREDICTIONS_FILE, JSON.stringify(predictionHistory, null, 2));
  } catch (e) {
    console.error('Error saving predictions:', e.message);
  }
}

// Get latest AI prediction
app.get('/api/predict', async (req, res) => {
  // Return cached prediction if available
  if (latestPrediction) {
    return res.json({
      success: true,
      source: 'cached',
      ...latestPrediction,
    });
  }

  // Try to fetch fresh prediction
  try {
    const result = await callMLServer('/predict/latest');
    latestPrediction = result;
    res.json({ success: true, source: 'live', ...result });
  } catch (error) {
    res.json({
      success: false,
      mlServerConnected: false,
      message: 'AI prediction server not available. Start it with: python predict_server.py',
    });
  }
});

// Get prediction history for charts
app.get('/api/predict/history', (req, res) => {
  const limit = parseInt(req.query.limit) || 50;
  const history = predictionHistory.slice(-limit);
  res.json({
    success: true,
    count: history.length,
    history: history,
  });
});

// Get ML server status
app.get('/api/ml/status', async (req, res) => {
  try {
    const [health, info] = await Promise.all([
      callMLServer('/health'),
      callMLServer('/model/info'),
    ]);

    res.json({
      success: true,
      connected: true,
      health,
      model: info,
      predictions_recorded: predictionHistory.length,
      latest_prediction: latestPrediction,
    });
  } catch (error) {
    res.json({
      success: false,
      connected: false,
      message: 'ML server not running',
      predictions_recorded: predictionHistory.length,
      latest_prediction: latestPrediction,
    });
  }
});

// Get anomaly log
app.get('/api/anomalies', async (req, res) => {
  try {
    const result = await callMLServer('/anomaly/log?limit=20');
    res.json({ success: true, ...result });
  } catch (error) {
    res.json({
      success: false,
      connected: false,
      anomalies: [],
      message: 'ML server not available',
    });
  }
});

// ==================== HELPER FUNCTIONS ====================

function saveTransactions() {
  try {
    fs.writeFileSync(DATA_FILE, JSON.stringify(transactions, null, 2));
  } catch (error) {
    console.error('Error saving transactions:', error);
  }
}

function saveConsumptionData(record) {
  const consumptionFile = path.join(DATA_DIR, 'consumption.json');

  let consumptionData = [];
  if (fs.existsSync(consumptionFile)) {
    try {
      consumptionData = JSON.parse(fs.readFileSync(consumptionFile, 'utf8'));
    } catch (e) {
      consumptionData = [];
    }
  }

  consumptionData.push(record);

  // Keep last 10000 records
  if (consumptionData.length > 10000) {
    consumptionData = consumptionData.slice(-10000);
  }

  fs.writeFileSync(consumptionFile, JSON.stringify(consumptionData, null, 2));
}

// ==================== START SERVER ====================

async function startServer() {
  // Initialize blockchain connection
  await initBlockchain();

  // Start Express server on all network interfaces (0.0.0.0)
  app.listen(PORT, '0.0.0.0', () => {
    console.log('\n' + '='.repeat(50));
    console.log('Smart Microgrid Server v3.0 — AI Edition');
    console.log('='.repeat(50));
    console.log(`✓ Server running on port ${PORT}`);
    console.log(`✓ Local: http://localhost:${PORT}`);
    console.log(`✓ Network: http://0.0.0.0:${PORT}/api/transaction`);
    console.log(`✓ Blockchain: ${blockchainConnected ? 'Connected' : 'Not connected'}`);
    console.log(`✓ Local backup: ${transactions.length} transactions`);
    console.log(`✓ AI Server: ${ML_SERVER_URL} (polling every 5s)`);
    console.log('='.repeat(50) + '\n');

    // Start ML prediction polling
    setInterval(pollPredictions, 5000);
    console.log('✓ AI prediction polling started (every 5s)');
  });
}

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n\nShutting down server...');
  saveTransactions();
  console.log('✓ Data saved');
  process.exit(0);
});

// Start the server
startServer();
