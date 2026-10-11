// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

/**
 * @title EnergyTrading
 * @dev Smart contract for P2P energy trading in a smart microgrid
 * 
 * This contract records energy transactions between prosumers (House A)
 * and consumers (House B) in an immutable blockchain ledger.
 * 
 * @author Smart Microgrid Project - BS Electrical Engineering FYP
 * @notice For educational purposes - Final Year Project
 */
contract EnergyTrading {
    
    // ==================== DATA STRUCTURES ====================
    
    struct Transaction {
        uint256 id;
        address seller;          // House A (solar producer)
        address buyer;           // House B (consumer)
        uint256 powerWatts;      // Instantaneous power in Watts
        uint256 energyWh;        // Energy transferred in Watt-hours
        uint256 pricePerKwh;     // Price per kWh in wei
        uint256 totalCost;       // Total transaction cost in wei
        uint256 timestamp;       // Block timestamp
        string solarProduction;  // Solar production reading
        string houseAConsumption; // House A consumption reading
    }
    
    struct House {
        string name;
        address wallet;
        bool isRegistered;
        bool isSeller;           // true = producer, false = consumer
        uint256 totalEnergyTraded;
        uint256 transactionCount;
    }
    
    // ==================== STATE VARIABLES ====================
    
    Transaction[] public transactions;
    mapping(address => House) public houses;
    mapping(uint256 => Transaction) public transactionById;
    
    address public owner;
    uint256 public transactionCounter;
    uint256 public defaultPricePerKwh;  // in wei
    
    // Statistics
    uint256 public totalEnergyTraded;   // in Wh
    uint256 public totalTransactions;
    uint256 public totalValueTraded;    // in wei
    
    // ==================== EVENTS ====================
    
    event HouseRegistered(
        address indexed wallet,
        string name,
        bool isSeller
    );
    
    event EnergyTransferred(
        uint256 indexed transactionId,
        address indexed seller,
        address indexed buyer,
        uint256 powerWatts,
        uint256 energyWh,
        uint256 timestamp
    );
    
    event TransactionRecorded(
        uint256 indexed transactionId,
        bytes32 blockHash,
        uint256 blockNumber
    );
    
    // ==================== MODIFIERS ====================
    
    modifier onlyOwner() {
        require(msg.sender == owner, "Only owner can call this function");
        _;
    }
    
    modifier onlyRegistered() {
        require(houses[msg.sender].isRegistered, "House not registered");
        _;
    }
    
    // ==================== CONSTRUCTOR ====================
    
    constructor() {
        owner = msg.sender;
        transactionCounter = 0;
        defaultPricePerKwh = 0.0001 ether; // 0.0001 ETH per kWh (~$0.10)
        
        // Auto-register default houses for demo
        _registerHouse(address(0x1111111111111111111111111111111111111111), "House_A", true);
        _registerHouse(address(0x2222222222222222222222222222222222222222), "House_B", false);
    }
    
    // ==================== REGISTRATION FUNCTIONS ====================
    
    function registerHouse(string memory _name, bool _isSeller) external {
        require(!houses[msg.sender].isRegistered, "House already registered");
        _registerHouse(msg.sender, _name, _isSeller);
    }
    
    function _registerHouse(address _wallet, string memory _name, bool _isSeller) internal {
        houses[_wallet] = House({
            name: _name,
            wallet: _wallet,
            isRegistered: true,
            isSeller: _isSeller,
            totalEnergyTraded: 0,
            transactionCount: 0
        });
        
        emit HouseRegistered(_wallet, _name, _isSeller);
    }
    
    // ==================== TRANSACTION FUNCTIONS ====================
    
    /**
     * @dev Record an energy transaction from ESP32 via backend server
     * @param _seller Address of the selling house (House A)
     * @param _buyer Address of the buying house (House B)
     * @param _powerWatts Instantaneous power in Watts
     * @param _energyWh Energy transferred in Watt-hours
     * @param _solarProduction Solar production reading as string
     * @param _houseAConsumption House A consumption reading as string
     */
    function recordTransaction(
        address _seller,
        address _buyer,
        uint256 _powerWatts,
        uint256 _energyWh,
        string memory _solarProduction,
        string memory _houseAConsumption
    ) external returns (uint256) {
        
        transactionCounter++;
        
        uint256 cost = (_energyWh * defaultPricePerKwh) / 1000; // Convert Wh to kWh
        
        Transaction memory newTx = Transaction({
            id: transactionCounter,
            seller: _seller,
            buyer: _buyer,
            powerWatts: _powerWatts,
            energyWh: _energyWh,
            pricePerKwh: defaultPricePerKwh,
            totalCost: cost,
            timestamp: block.timestamp,
            solarProduction: _solarProduction,
            houseAConsumption: _houseAConsumption
        });
        
        transactions.push(newTx);
        transactionById[transactionCounter] = newTx;
        
        // Update statistics
        totalEnergyTraded += _energyWh;
        totalTransactions++;
        totalValueTraded += cost;
        
        // Update house statistics
        if (houses[_seller].isRegistered) {
            houses[_seller].totalEnergyTraded += _energyWh;
            houses[_seller].transactionCount++;
        }
        if (houses[_buyer].isRegistered) {
            houses[_buyer].totalEnergyTraded += _energyWh;
            houses[_buyer].transactionCount++;
        }
        
        emit EnergyTransferred(
            transactionCounter,
            _seller,
            _buyer,
            _powerWatts,
            _energyWh,
            block.timestamp
        );
        
        emit TransactionRecorded(
            transactionCounter,
            blockhash(block.number - 1),
            block.number
        );
        
        return transactionCounter;
    }
    
    /**
     * @dev Simplified transaction recording for testing
     * Uses pre-registered House A and House B addresses
     */
    function recordSimpleTransaction(
        uint256 _powerWatts,
        uint256 _energyWh
    ) external returns (uint256) {
        return this.recordTransaction(
            address(0x1111111111111111111111111111111111111111),
            address(0x2222222222222222222222222222222222222222),
            _powerWatts,
            _energyWh,
            "",
            ""
        );
    }
    
    // ==================== VIEW FUNCTIONS ====================
    
    function getTransaction(uint256 _id) external view returns (
        uint256 id,
        address seller,
        address buyer,
        uint256 powerWatts,
        uint256 energyWh,
        uint256 timestamp,
        uint256 totalCost
    ) {
        Transaction memory t = transactionById[_id];
        return (
            t.id,
            t.seller,
            t.buyer,
            t.powerWatts,
            t.energyWh,
            t.timestamp,
            t.totalCost
        );
    }
    
    function getLatestTransaction() external view returns (
        uint256 id,
        address seller,
        address buyer,
        uint256 powerWatts,
        uint256 energyWh,
        uint256 timestamp
    ) {
        require(transactions.length > 0, "No transactions yet");
        Transaction memory t = transactions[transactions.length - 1];
        return (t.id, t.seller, t.buyer, t.powerWatts, t.energyWh, t.timestamp);
    }
    
    function getTransactionCount() external view returns (uint256) {
        return transactions.length;
    }
    
    function getStatistics() external view returns (
        uint256 _totalTransactions,
        uint256 _totalEnergyWh,
        uint256 _totalValueWei,
        uint256 _averagePowerWatts
    ) {
        uint256 avgPower = 0;
        if (transactions.length > 0) {
            uint256 totalPower = 0;
            for (uint256 i = 0; i < transactions.length; i++) {
                totalPower += transactions[i].powerWatts;
            }
            avgPower = totalPower / transactions.length;
        }
        
        return (
            totalTransactions,
            totalEnergyTraded,
            totalValueTraded,
            avgPower
        );
    }
    
    function getHouseInfo(address _wallet) external view returns (
        string memory name,
        bool isSeller,
        uint256 totalEnergy,
        uint256 txCount
    ) {
        House memory h = houses[_wallet];
        return (h.name, h.isSeller, h.totalEnergyTraded, h.transactionCount);
    }
    
    // ==================== ADMIN FUNCTIONS ====================
    
    function setPricePerKwh(uint256 _newPrice) external onlyOwner {
        defaultPricePerKwh = _newPrice;
    }
    
    function getContractBalance() external view returns (uint256) {
        return address(this).balance;
    }
}
