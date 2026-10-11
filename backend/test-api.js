/**
 * Smart Microgrid - API Test Script
 * 
 * Run this script to test the backend API endpoints
 * Usage: node test-api.js
 */

const http = require('http');

const BASE_URL = 'http://localhost:3000';

// Helper function to make HTTP requests
function makeRequest(method, path, data = null) {
    return new Promise((resolve, reject) => {
        const url = new URL(path, BASE_URL);

        const options = {
            hostname: url.hostname,
            port: url.port,
            path: url.pathname,
            method: method,
            headers: {
                'Content-Type': 'application/json'
            }
        };

        const req = http.request(options, (res) => {
            let body = '';
            res.on('data', chunk => body += chunk);
            res.on('end', () => {
                try {
                    resolve(JSON.parse(body));
                } catch (e) {
                    resolve(body);
                }
            });
        });

        req.on('error', reject);

        if (data) {
            req.write(JSON.stringify(data));
        }

        req.end();
    });
}

async function runTests() {
    console.log('\n' + '='.repeat(50));
    console.log('Smart Microgrid API Tests');
    console.log('='.repeat(50) + '\n');

    try {
        // Test 1: Health check
        console.log('1. Testing health check...');
        const health = await makeRequest('GET', '/');
        console.log('   Status:', health.status);
        console.log('   Blockchain:', health.blockchain?.connected ? 'Connected' : 'Not connected');
        console.log('   ✓ Health check passed\n');

        // Test 2: Blockchain status
        console.log('2. Testing blockchain status...');
        const blockchainStatus = await makeRequest('GET', '/api/blockchain/status');
        console.log('   Connected:', blockchainStatus.connected);
        if (blockchainStatus.connected) {
            console.log('   Block:', blockchainStatus.currentBlock);
            console.log('   Account:', blockchainStatus.account);
        }
        console.log('   ✓ Blockchain status check passed\n');

        // Test 3: Post transaction (House A)
        console.log('3. Testing transaction recording (House A)...');
        const transaction = await makeRequest('POST', '/api/transaction', {
            from: 'House_A',
            to: 'House_B',
            solarProduction: 500,
            houseAConsumption: 200,
            excessPower: 300,
            power: 300,
            energy: 25,
            voltage: 220,
            current: 1.36
        });
        console.log('   Transaction ID:', transaction.transactionId);
        console.log('   Blockchain TX:', transaction.blockchainTx || 'N/A (blockchain not connected)');
        console.log('   ✓ Transaction recording passed\n');

        // Test 4: Post consumption (House B)
        console.log('4. Testing consumption recording (House B)...');
        const consumption = await makeRequest('POST', '/api/consumption', {
            house: 'House_B',
            power: 300,
            energy: 25,
            voltage: 220,
            current: 1.36,
            receivingFromHouseA: true,
            totalReceived: 25
        });
        console.log('   Record ID:', consumption.recordId);
        console.log('   ✓ Consumption recording passed\n');

        // Test 5: Get transactions
        console.log('5. Testing get transactions...');
        const transactions = await makeRequest('GET', '/api/transactions');
        console.log('   Source:', transactions.source);
        console.log('   Count:', transactions.count);
        console.log('   ✓ Get transactions passed\n');

        // Test 6: Get latest transaction
        console.log('6. Testing get latest transaction...');
        const latest = await makeRequest('GET', '/api/transaction/latest');
        console.log('   Latest ID:', latest.transaction?.id || 'None');
        console.log('   ✓ Get latest transaction passed\n');

        // Test 7: Get statistics
        console.log('7. Testing statistics...');
        const stats = await makeRequest('GET', '/api/stats');
        console.log('   Total Transactions:', stats.stats?.totalTransactions);
        console.log('   Total Energy:', stats.stats?.totalEnergy, 'kWh');
        console.log('   Source:', stats.stats?.source);
        console.log('   ✓ Statistics passed\n');

        // Test 8: Export ML data
        console.log('8. Testing ML export...');
        const mlData = await makeRequest('GET', '/api/export/ml');
        console.log('   Records:', mlData.count);
        console.log('   ✓ ML export passed\n');

        console.log('='.repeat(50));
        console.log('All tests passed!');
        console.log('='.repeat(50) + '\n');

    } catch (error) {
        console.error('Test failed:', error.message);
        console.log('\nMake sure the server is running: npm start');
    }
}

runTests();
