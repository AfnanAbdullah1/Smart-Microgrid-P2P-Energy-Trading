/*
 * House B Test Firmware - Simple Version
 * 
 * NO WiFi, NO Server, NO Blockchain
 * Just basic testing:
 * - PZEM reading (House B consumption)
 * - Serial Monitor output
 * - Detect relay state (receiving power or not)
 * 
 * Hardware:
 * - PZEM #2: GPIO 16 (RX), GPIO 17 (TX)
 * 
 * Upload this for basic hardware testing!
 */

#include <PZEM004Tv30.h>

// ==================== PIN DEFINITIONS ====================

#define LED_PIN 2

// PZEM #2 - House B Consumption
#define PZEM2_RX_PIN 16
#define PZEM2_TX_PIN 17

// ==================== CONFIGURATION ====================

// Timing
const unsigned long READING_INTERVAL = 500; // 500ms = 0.5 seconds

// ==================== HARDWARE SETUP ====================

HardwareSerial SerialPZEM2(2);
PZEM004Tv30 pzemHouseB(SerialPZEM2, PZEM2_RX_PIN, PZEM2_TX_PIN);

// ==================== VARIABLES ====================

unsigned long lastReadingTime = 0;

// House B readings
float houseBVoltage = 0.0;
float houseBCurrent = 0.0;
float houseBPower = 0.0;
float houseBEnergy = 0.0;

// Status
bool receivingPower = false;
float totalReceived = 0.0;

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║   House B - Test Firmware v1.0 (SIMPLIFIED)   ║");
  Serial.println("║   No WiFi | No Server | No Blockchain         ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  
  // Setup pins
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("\n✓ Pins configured");
  Serial.println("  - LED: GPIO 2");
  
  // Test PZEM
  delay(1000);
  Serial.println("\nTesting PZEM...");
  float testV = pzemHouseB.voltage();
  
  if (isnan(testV)) {
    Serial.println("⚠️  PZEM #2 NOT responding!");
    Serial.println("   Check wiring: GPIO 16 (RX), GPIO 17 (TX)");
  } else {
    Serial.println("✓ PZEM #2 OK");
  }
  
  Serial.println("\n🚀 System Ready! Reading every 2 seconds...\n");
}

// ==================== MAIN LOOP ====================

void loop() {
  unsigned long currentTime = millis();
  
  // Read sensors periodically
  if (currentTime - lastReadingTime >= READING_INTERVAL) {
    lastReadingTime = currentTime;
    
    readSensors();
    detectRelayStatus();
    printStatus();
    blinkLED();
  }
}

// ==================== FUNCTIONS ====================

void readSensors() {
  // Read PZEM
  houseBVoltage = pzemHouseB.voltage();
  houseBCurrent = pzemHouseB.current();
  houseBPower = pzemHouseB.power();
  houseBEnergy = pzemHouseB.energy();
  
  // Handle NaN
  if (isnan(houseBVoltage)) houseBVoltage = 0.0;
  if (isnan(houseBCurrent)) houseBCurrent = 0.0;
  if (isnan(houseBPower)) houseBPower = 0.0;
  if (isnan(houseBEnergy)) houseBEnergy = 0.0;
}

void detectRelayStatus() {
  // Detect relay state by VOLTAGE (not power!)
  // When relay ON → Voltage present (220V)
  // When relay OFF → No voltage
  
  if (houseBVoltage > 100.0) {
    // Voltage detected → Relay is ON
    if (!receivingPower) {
      Serial.println("\n🔌 RELAY ON - RECEIVING POWER FROM HOUSE A!");
      receivingPower = true;
    }
    totalReceived = houseBEnergy;
  } else {
    // No voltage → Relay is OFF
    if (receivingPower) {
      Serial.println("\n🔌 RELAY OFF - Power from House A STOPPED");
      receivingPower = false;
    }
  }
}

void printStatus() {
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║        HOUSE B - Consumer (Prototype)          ║");
  Serial.println("╠════════════════════════════════════════════════╣");
  
  // Consumption details
  Serial.print("║ Power Consumption:   ");
  if (houseBPower < 10.0) Serial.print(" ");
  if (houseBPower < 100.0) Serial.print(" ");
  Serial.print(houseBPower, 1);
  Serial.println("W                       ║");
  
  Serial.print("║ Current:             ");
  if (houseBCurrent < 1.0) Serial.print(" ");
  Serial.print(houseBCurrent, 3);
  Serial.println("A                       ║");
  
  Serial.print("║ Voltage:             ");
  if (houseBVoltage < 100.0) Serial.print(" ");
  Serial.print(houseBVoltage, 1);
  Serial.println("V                       ║");
  
  Serial.print("║ Total Energy:        ");
  if (houseBEnergy < 1.0) Serial.print(" ");
  if (houseBEnergy < 10.0) Serial.print(" ");
  Serial.print(houseBEnergy, 3);
  Serial.println(" kWh                 ║");
  
  Serial.println("╠════════════════════════════════════════════════╣");
  
  // Relay status
  Serial.print("║ Relay Status:        ");
  Serial.print(receivingPower ? "ON (Receiving)          " : "OFF (No Power)          ");
  Serial.println("║");
  
  if (receivingPower) {
    Serial.println("║ Source:              House A via Relay         ║");
    Serial.print("║ Total Received:      ");
    if (totalReceived < 1.0) Serial.print(" ");
    if (totalReceived < 10.0) Serial.print(" ");
    Serial.print(totalReceived, 3);
    Serial.println(" kWh                 ║");
  } else {
    Serial.println("║ Source:              Not Connected             ║");
  }
  
  Serial.println("╚════════════════════════════════════════════════╝");
  Serial.println();
}

void blinkLED() {
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
}
