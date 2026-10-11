/**
 * Smart Microgrid - EnergyTrading Contract Tests
 */

const { expect } = require("chai");
const { ethers } = require("hardhat");

describe("EnergyTrading Contract", function () {
    let EnergyTrading;
    let energyTrading;
    let owner;
    let houseA;
    let houseB;

    const HOUSE_A_DEFAULT = "0x1111111111111111111111111111111111111111";
    const HOUSE_B_DEFAULT = "0x2222222222222222222222222222222222222222";

    beforeEach(async function () {
        [owner, houseA, houseB] = await ethers.getSigners();

        EnergyTrading = await ethers.getContractFactory("EnergyTrading");
        energyTrading = await EnergyTrading.deploy();
        await energyTrading.waitForDeployment();
    });

    describe("Deployment", function () {
        it("Should set the correct owner", async function () {
            expect(await energyTrading.owner()).to.equal(owner.address);
        });

        it("Should initialize with zero transactions", async function () {
            expect(await energyTrading.getTransactionCount()).to.equal(0);
        });

        it("Should set default price per kWh", async function () {
            const price = await energyTrading.defaultPricePerKwh();
            expect(price).to.equal(ethers.parseEther("0.0001"));
        });

        it("Should register default houses", async function () {
            const houseAInfo = await energyTrading.getHouseInfo(HOUSE_A_DEFAULT);
            expect(houseAInfo.name).to.equal("House_A");
            expect(houseAInfo.isSeller).to.equal(true);

            const houseBInfo = await energyTrading.getHouseInfo(HOUSE_B_DEFAULT);
            expect(houseBInfo.name).to.equal("House_B");
            expect(houseBInfo.isSeller).to.equal(false);
        });
    });

    describe("House Registration", function () {
        it("Should allow new house registration", async function () {
            await energyTrading.connect(houseA).registerHouse("House_C", true);

            const houseInfo = await energyTrading.getHouseInfo(houseA.address);
            expect(houseInfo.name).to.equal("House_C");
            expect(houseInfo.isSeller).to.equal(true);
        });

        it("Should emit HouseRegistered event", async function () {
            await expect(energyTrading.connect(houseA).registerHouse("House_C", true))
                .to.emit(energyTrading, "HouseRegistered")
                .withArgs(houseA.address, "House_C", true);
        });

        it("Should prevent duplicate registration", async function () {
            await energyTrading.connect(houseA).registerHouse("House_C", true);

            await expect(
                energyTrading.connect(houseA).registerHouse("House_D", false)
            ).to.be.revertedWith("House already registered");
        });
    });

    describe("Transaction Recording", function () {
        it("Should record a simple transaction", async function () {
            const tx = await energyTrading.recordSimpleTransaction(500, 100);
            await tx.wait();

            expect(await energyTrading.getTransactionCount()).to.equal(1);
        });

        it("Should record full transaction with details", async function () {
            await energyTrading.recordTransaction(
                HOUSE_A_DEFAULT,
                HOUSE_B_DEFAULT,
                500,  // 500W
                100,  // 100Wh
                "Solar: 600W",
                "Consumption: 100W"
            );

            const txData = await energyTrading.getTransaction(1);
            expect(txData.powerWatts).to.equal(500);
            expect(txData.energyWh).to.equal(100);
            expect(txData.seller).to.equal(HOUSE_A_DEFAULT);
            expect(txData.buyer).to.equal(HOUSE_B_DEFAULT);
        });

        it("Should emit EnergyTransferred event", async function () {
            await expect(energyTrading.recordSimpleTransaction(500, 100))
                .to.emit(energyTrading, "EnergyTransferred");
        });

        it("Should update statistics after transaction", async function () {
            await energyTrading.recordSimpleTransaction(500, 100);
            await energyTrading.recordSimpleTransaction(300, 50);

            const stats = await energyTrading.getStatistics();
            expect(stats._totalTransactions).to.equal(2);
            expect(stats._totalEnergyWh).to.equal(150);  // 100 + 50
            expect(stats._averagePowerWatts).to.equal(400);  // (500 + 300) / 2
        });

        it("Should return correct transaction ID", async function () {
            const tx1 = await energyTrading.recordSimpleTransaction(500, 100);
            const receipt1 = await tx1.wait();

            const tx2 = await energyTrading.recordSimpleTransaction(300, 50);
            const receipt2 = await tx2.wait();

            expect(await energyTrading.transactionCounter()).to.equal(2);
        });
    });

    describe("View Functions", function () {
        beforeEach(async function () {
            await energyTrading.recordSimpleTransaction(500, 100);
            await energyTrading.recordSimpleTransaction(300, 50);
            await energyTrading.recordSimpleTransaction(700, 200);
        });

        it("Should get transaction by ID", async function () {
            const tx = await energyTrading.getTransaction(2);
            expect(tx.powerWatts).to.equal(300);
            expect(tx.energyWh).to.equal(50);
        });

        it("Should get latest transaction", async function () {
            const latest = await energyTrading.getLatestTransaction();
            expect(latest.powerWatts).to.equal(700);
            expect(latest.energyWh).to.equal(200);
        });

        it("Should get correct statistics", async function () {
            const stats = await energyTrading.getStatistics();
            expect(stats._totalTransactions).to.equal(3);
            expect(stats._totalEnergyWh).to.equal(350);  // 100 + 50 + 200
            expect(stats._averagePowerWatts).to.equal(500);  // (500 + 300 + 700) / 3
        });
    });

    describe("Admin Functions", function () {
        it("Should allow owner to set price", async function () {
            const newPrice = ethers.parseEther("0.0002");
            await energyTrading.setPricePerKwh(newPrice);

            expect(await energyTrading.defaultPricePerKwh()).to.equal(newPrice);
        });

        it("Should prevent non-owner from setting price", async function () {
            const newPrice = ethers.parseEther("0.0002");

            await expect(
                energyTrading.connect(houseA).setPricePerKwh(newPrice)
            ).to.be.revertedWith("Only owner can call this function");
        });
    });

    describe("Edge Cases", function () {
        it("Should handle zero power transaction", async function () {
            await energyTrading.recordSimpleTransaction(0, 0);

            const tx = await energyTrading.getTransaction(1);
            expect(tx.powerWatts).to.equal(0);
        });

        it("Should revert getLatestTransaction when no transactions", async function () {
            const freshContract = await EnergyTrading.deploy();
            await freshContract.waitForDeployment();

            await expect(freshContract.getLatestTransaction())
                .to.be.revertedWith("No transactions yet");
        });
    });
});
