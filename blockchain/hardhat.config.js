require("@nomicfoundation/hardhat-toolbox");

/** @type import('hardhat/config').HardhatUserConfig */
module.exports = {
  solidity: {
    version: "0.8.20",
    settings: {
      optimizer: {
        enabled: true,
        runs: 200
      }
    }
  },
  networks: {
    // Local Ganache network
    ganache: {
      url: "http://127.0.0.1:7545",
      chainId: 1337,
      // Ganache default accounts (first account)
      accounts: [
        // Ganache Account #1
        "0x82564c0d85b9b728504c7e4f67c723837db71593e11a2fa89365c78c7f66fa93"
      ]
    },
    // Hardhat's built-in network for testing
    hardhat: {
      chainId: 31337
    },
    // Localhost (for running your own node)
    localhost: {
      url: "http://127.0.0.1:8545",
      chainId: 31337
    }
  },
  paths: {
    sources: "./contracts",
    tests: "./test",
    cache: "./cache",
    artifacts: "./artifacts"
  },
  mocha: {
    timeout: 40000
  }
};
