/*
 * House A Test Firmware - Simple Version
 * 
 * NO WiFi, NO Server, NO Blockchain
 * Just basic testing:
 * - PZEM reading (House A consumption)
 * - Relay control (threshold-based)
 * - Serial Monitor output
 * 
 * Hardware:
 * - PZEM #1: GPIO 18 (RX), GPIO 19 (TX)
 * - Relay: GPIO 27
 * 
 * Upload this for basic hardware testing!
 */

#include <PZEM004Tv30.h>

// ==================== PIN DEFINITIONS ====================

#define RELAY_PIN 27  // GPIO 27 (safer boot state than 25/26)
#define LED_PIN 2

// PZEM #1 - House A Consumption
#define PZEM1_RX_PIN 18
#define PZEM1_TX_PIN 19

// ==================== CONFIGURATION ====================

// 12V 1A Charger (replaces YiHua supply)
const float DC_VOLTAGE = 12.0;                  // 12V charger
const float FIXED_GENERATION_AMPS_DC = 1.0;     // 1A from charger
const float FIXED_GENERATION_WATTS_DC = 12.0;   // DC input power (12V × 1A)

// AC side (HOPE H201 Inverter: 12V DC → 220V AC, ~80% efficiency)
const float INVERTER_MAX_WATTS_AC = 10.0;       // Actual AC output (~10W with 12W input)
const float FIXED_GENERATION_AMPS_AC = 0.045;   // 10W ÷ 220V = 0.045A
const float FIXED_GENERATION_WATTS = 10.0;      // Inverter AC output with current charger

// Threshold (adjusted for 10W generation - prototype mode)
const float CONSUMPTION_THRESHOLD_AMPS = 0.005; // Amperes - if House A < 0.005A, relay ON
const float CONSUMPTION_THRESHOLD_WATTS = 1.0;  // 1W threshold
const float HYSTERESIS_WATTS = 0.0;             // No hysteresis - relay OFF exactly at 1W

// Timing
const unsigned long READING_INTERVAL = 500; // 500ms = 0.5 seconds (faster response)

// ==================== HARDWARE SETUP ====================

HardwareSerial SerialPZEM1(1);
PZEM004Tv30 pzemHouseA(SerialPZEM1, PZEM1_RX_PIN, PZEM1_TX_PIN);

// ==================== VARIABLES ====================

unsigned long lastReadingTime = 0;
bool relayState = false;

// House A readings
float houseAVoltage = 0.0;
float houseACurrent = 0.0;
float houseAPower = 0.0;
float houseAEnergy = 0.0;

// Calculated
float excessPower = 0.0;

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║   House A - PROTOTYPE Mode (Simplified)       ║");
  Serial.println("║   12V 1A Charger → HOPE H201 Inverter → AC    ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  
  // Setup pins (ACTIVE LOW relay - start with HIGH = OFF)
  // CRITICAL: Set HIGH before pinMode to prevent boot glitch
  digitalWrite(RELAY_PIN, HIGH);  // Set HIGH FIRST
  pinMode(RELAY_PIN, OUTPUT);     // Then set as output
  digitalWrite(RELAY_PIN, HIGH);  // Ensure HIGH again
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("\n✓ Pins configured");
  Serial.println("  - Relay: GPIO 27");
  Serial.println("  - LED: GPIO 2");
  
  // Test PZEM
  delay(1000);
  Serial.println("\nTesting PZEM...");
  float testV = pzemHouseA.voltage();
  
  if (isnan(testV)) {
    Serial.println("⚠️  PZEM #1 NOT responding!");
    Serial.println("   Check wiring: GPIO 18 (RX), GPIO 19 (TX)");
  } else {
    Serial.println("✓ PZEM #1 OK");
  }
  
  // Print configuration
  Serial.println("\n📊 Prototype Configuration:");
  Serial.println("  [DC SUPPLY]");
  Serial.print("  12V Charger: ");
  Serial.print(DC_VOLTAGE, 0);
  Serial.println("V (adjustable current)");
  
  Serial.println("\n  [AC INVERTER]");
  Serial.print("  12V DC → HOPE H201 Inverter: ");
  Serial.print(INVERTER_MAX_WATTS_AC, 0);
  Serial.println("W max");
  Serial.print("  Max AC Output: ");
  Serial.print(FIXED_GENERATION_AMPS_AC, 1);
  Serial.println("A @ 220V");
  
  Serial.println("\n  [THRESHOLD]");
  
  Serial.print("  Threshold: < ");
  Serial.print(CONSUMPTION_THRESHOLD_AMPS, 1);
  Serial.print("A (");
  Serial.print(CONSUMPTION_THRESHOLD_WATTS, 0);
  Serial.println("W) → Relay ON");
  
  Serial.println("\n🚀 System Ready! Reading every 0.5 seconds...\n");
}

// ==================== MAIN LOOP ====================

void loop() {
  unsigned long currentTime = millis();
  
  // Read sensors periodically
  if (currentTime - lastReadingTime >= READING_INTERVAL) {
    lastReadingTime = currentTime;
    
    readSensors();
    calculateExcess();
    controlRelay();
    printStatus();
    blinkLED();
  }
}

// ==================== FUNCTIONS ====================

void readSensors() {
  // Read PZEM
  houseAVoltage = pzemHouseA.voltage();
  houseACurrent = pzemHouseA.current();
  houseAPower = pzemHouseA.power();
  houseAEnergy = pzemHouseA.energy();
  
  // Handle NaN
  if (isnan(houseAVoltage)) houseAVoltage = 0.0;
  if (isnan(houseACurrent)) houseACurrent = 0.0;
  if (isnan(houseAPower)) houseAPower = 0.0;
  if (isnan(houseAEnergy)) houseAEnergy = 0.0;
}

void calculateExcess() {
  excessPower = FIXED_GENERATION_WATTS - houseAPower;
  if (excessPower < 0) excessPower = 0;
}

void controlRelay() {
  // Logic: If House A < threshold → Relay ON
  // NOTE: This relay is ACTIVE LOW (LOW = ON, HIGH = OFF)
  
  if (!relayState && houseAPower < CONSUMPTION_THRESHOLD_WATTS) {
    // Turn ON (ACTIVE LOW)
    digitalWrite(RELAY_PIN, LOW);
    relayState = true;
    
    Serial.println("\n🔋 RELAY ON - Low consumption detected!");
    Serial.print("   House A: ");
    Serial.print(houseAPower, 1);
    Serial.print("W (");
    Serial.print(houseACurrent, 3);
    Serial.println("A)");
    Serial.print("   Excess: ");
    Serial.print(excessPower, 1);
    Serial.println("W available for House B\n");
  }
  else if (relayState && houseAPower >= (CONSUMPTION_THRESHOLD_WATTS + HYSTERESIS_WATTS)) {
    // Turn OFF (ACTIVE LOW - use HIGH to turn off)
    Serial.println("\n⏸️  RELAY OFF - High consumption detected!");
    Serial.println("   Sending GPIO 27 = HIGH...");
    
    digitalWrite(RELAY_PIN, HIGH);
    delay(100);  // Give time for GPIO to settle
    digitalWrite(RELAY_PIN, HIGH);  // Send again to ensure
    
    relayState = false;
    
    Serial.print("   House A: ");
    Serial.print(houseAPower, 1);
    Serial.print("W - GPIO 27 set to HIGH");
    Serial.println("\n");
  }
}

void printStatus() {
  // Calculate DC input power (estimated from AC output + inverter efficiency ~80%)
  float estimatedDC_Amps = (houseAPower / 0.8) / DC_VOLTAGE;  // Assuming 80% efficiency
  float estimatedDC_Watts = DC_VOLTAGE * estimatedDC_Amps;
  
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║        HOUSE A - Prototype Mode               ║");
  Serial.println("╠════════════════════════════════════════════════╣");
  
  // DC Supply Side
  Serial.println("║ [DC SUPPLY INPUT]                              ║");
  Serial.print("║ Charger: ");
  Serial.print(DC_VOLTAGE, 0);
  Serial.print("V × ~");
  Serial.print(estimatedDC_Amps, 2);
  Serial.print("A = ");
  Serial.print(estimatedDC_Watts, 1);
  Serial.println("W (estimated) ║");
  
  Serial.println("║                                                ║");
  
  // AC Inverter Side
  Serial.println("║ [AC INVERTER OUTPUT - ~10W]                    ║");
  Serial.print("║ Generation Capacity: ");
  Serial.print(INVERTER_MAX_WATTS_AC, 0);
  Serial.println("W (0.045A @ 220V) ║");
  
  Serial.print("║ House A Consumption: ");
  Serial.print(houseAPower, 1);
  Serial.print("W (");
  Serial.print(houseACurrent, 3);
  Serial.println("A)         ║");
  
  Serial.print("║ Voltage:             ");
  Serial.print(houseAVoltage, 1);
  Serial.println("V                      ║");
  
  Serial.print("║ Excess Available:    ");
  Serial.print(excessPower, 1);
  Serial.println(" W                    ║");
  
  Serial.println("╠════════════════════════════════════════════════╣");
  
  Serial.print("║ Threshold: < ");
  Serial.print(CONSUMPTION_THRESHOLD_WATTS, 0);
  Serial.print("W (");
  Serial.print(CONSUMPTION_THRESHOLD_AMPS, 2);
  Serial.println("A) → Relay ON           ║");
  
  Serial.print("║ Relay Status: ");
  Serial.print(relayState ? "ON (Transferring)        " : "OFF (Standby)            ");
  Serial.println("║");
  
  Serial.println("╚════════════════════════════════════════════════╝");
  Serial.println();
}

void blinkLED() {
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
}
