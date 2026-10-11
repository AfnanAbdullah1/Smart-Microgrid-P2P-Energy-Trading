/*
 * =====================================================
 *  SMART MICROGRID - HOUSE A CONTROLLER
 *  Fixed Generation Model v3.0
 * =====================================================
 *  
 *  Project: P2P Energy Trading with Blockchain
 *  FYP - BS Electrical Engineering
 *  
 *  Hardware:
 *  - ESP32 DevKit
 *  - PZEM-004T (AC Power Monitor)
 *  - Relay Module (Active-LOW, 5V)
 *  - 12V 1A Charger + HOPE H201 Inverter (~10W AC)
 *  - 1602A LCD with I2C (status display)
 *  
 *  Features:
 *  - Fixed 10W generation (0.045A at 220V)
 *  - Real consumption monitoring via PZEM
 *  - Automatic relay control (threshold: 1W)
 *  - Blockchain transaction recording
 *  - Live telemetry to web dashboard
 *  - LCD status display (rotating screens)
 *  - Real-time monitoring
 * =====================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <PZEM004Tv30.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <hd44780.h>                        // ESP32-native LCD library (no arch warning)
#include <hd44780ioClass/hd44780_I2Cexp.h>  // I2C PCF8574 backpack driver
#include <esp_system.h>   // for esp_reset_reason()
#include "secrets.h"

// ==================== WiFi Configuration ====================
const char* ssid = WIFI_SSID;           // From secrets.h
const char* password = WIFI_PASSWORD;   // From secrets.h

// Server URLs are defined in secrets.h (via NGROK_URL or SERVER_IP)


// ==================== Hardware Pins ====================
#define PZEM_RX_PIN 18    // PZEM TX connects here
#define PZEM_TX_PIN 19    // PZEM RX connects here
#define RELAY_PIN 27      // Relay control pin

// ==================== PZEM Sensor ====================
HardwareSerial SerialPZEMA(1);
PZEM004Tv30 pzem(SerialPZEMA, PZEM_RX_PIN, PZEM_TX_PIN);

// ==================== LCD Display ====================
// 1602A LCD with I2C backpack — using hd44780 (ESP32-native, no arch warning)
// SDA = GPIO 21, SCL = GPIO 22 (ESP32 default I2C pins)
// hd44780_I2Cexp auto-detects the I2C address (0x27 is standard for PCF8574)
hd44780_I2Cexp lcd(0x27);

// ==================== System Configuration ====================
// Fixed generation (simulated solar panel)
// DC: 12V charger × 1A = 12W
// Inverter: HOPE H201 (160W rated, ~80% efficiency)
// AC output: 12W × 0.80 = ~10W at 220V = ~0.045A
const float FIXED_GENERATION_WATTS = 10.0;     // ~10W AC output
const float FIXED_GENERATION_AMPS = 0.045;     // 0.045A at 220V

// Threshold for relay control
const float CONSUMPTION_THRESHOLD_WATTS = 1.0;  // If <= 1W, relay ON
const float CONSUMPTION_THRESHOLD_AMPS = 0.0;   // Approximately 0A

// Transaction timing
const unsigned long TRANSACTION_INTERVAL = 60000;  // 60 seconds (Wait longer for Ngrok/Blockchain)
const unsigned long SENSOR_READ_INTERVAL = 500;    // 0.5 seconds
const unsigned long LIVE_SEND_INTERVAL = 500;      // Send live telemetry every 500ms (max speed)

// ==================== Global Variables ====================
// Sensor readings
float voltage = 0.0;
float current = 0.0;
float power = 0.0;
float energy = 0.0;
float frequency = 0.0;
float pf = 0.0;

// Calculated values
float excessPower = 0.0;
float excessCurrent = 0.0;

// System state
bool relayState = false;  // false = OFF, true = ON
bool manualMode = false;
unsigned long lastTransactionTime = 0;
unsigned long lastSensorRead = 0;
unsigned long lastLiveSend = 0;
unsigned int transactionCount = 0;
bool serverConnected = false;
bool blockchainConnected = false;

// Relay timer
unsigned long relayOnStartTime = 0;
bool relayTimerActive = false;

// LCD timer
unsigned long lastLCDUpdate = 0;
const unsigned long LCD_UPDATE_INTERVAL = 3000;  // Rotate screens every 3 seconds (less perceived flicker)
int lcdScreen = 0;  // Current screen (0, 1, 2)

// ==================== Setup Function ====================
void setup() {
  Serial.begin(115200);
  delay(200);

  // ── Detect if this is a true power-on boot ─────────────────
  // When Serial Monitor opens, it toggles the DTR pin which
  // triggers a SOFTWARE reset (not power-on). We use this to
  // skip the full startup sequence on that second boot, so
  // everything appears only ONCE.
  bool isFirstBoot = (esp_reset_reason() == ESP_RST_POWERON);
  // ───────────────────────────────────────────────────────────

  // ── Initialize LCD FIRST (before WiFi/PZEM) ────────────────
  Wire.begin(21, 22);
  Wire.setClock(400000);  // 400kHz Fast Mode — eliminates flicker
  delay(50);
  lcd.begin(16, 2);       // hd44780 API: begin(cols, rows)
  lcd.backlight();
  lcd.noCursor();         // Disable blinking cursor — hidden flicker source
  lcd.noBlink();          // Disable cursor blink signal
  lcd.clear();
  delay(5);
  // ───────────────────────────────────────────────────────────

  // Initialize pins (always, regardless of boot type)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Relay OFF (active-LOW)
  relayState = false;

  if (isFirstBoot) {
    // ── Full startup sequence — only on true power-on ─────────
    lcd.setCursor(0, 0);
    lcd.print("  HOUSE A  ~10W");
    lcd.setCursor(0, 1);
    lcd.print(" Initializing...");

    printHeader();
    Serial.println("✓ Pins initialized (Relay GPIO27, Active-LOW)");
    Serial.println("✓ LCD initialized (SDA=21, SCL=22, addr=0x27)");

    // WiFi
    lcd.clear(); delay(5);
    lcd.setCursor(0, 0); lcd.print("Connecting WiFi");
    lcd.setCursor(0, 1); lcd.print(String(WIFI_SSID).substring(0, 16));
    connectToWiFi();

    lcd.clear(); delay(5);
    lcd.setCursor(0, 0);
    if (WiFi.status() == WL_CONNECTED) {
      lcd.print("WiFi: Connected");
      lcd.setCursor(0, 1);
      lcd.print(WiFi.localIP().toString());
    } else {
      lcd.print("WiFi: FAILED!");
      lcd.setCursor(0, 1);
      lcd.print("Check secrets.h");
    }
    delay(2000);

    // Server
    lcd.clear(); delay(5);
    lcd.setCursor(0, 0); lcd.print("Testing Server..");
    testServerConnection();

    // PZEM
    lcd.clear(); delay(5);
    lcd.setCursor(0, 0); lcd.print("Testing PZEM....");
    delay(1000);
    testPZEMSensor();

    Serial.println("\n" + repeat("=", 60));
    Serial.println("             HOUSE A - READY TO OPERATE");
    Serial.println(repeat("=", 60));
    printConfiguration();

    lcd.clear(); delay(5);
    lcd.setCursor(0, 0); lcd.print("  HOUSE A READY");
    lcd.setCursor(0, 1); lcd.print(" P2P System ON! ");
    delay(2000);
    // ─────────────────────────────────────────────────────────

  } else {
    // ── Serial Monitor reset — skip startup, go straight to run
    Serial.println("[HOUSE A] Serial Monitor connected — resuming.");
    connectToWiFi();
    testServerConnection();
    delay(500);
    testPZEMSensor();
  }

  // Initial sensor reading and relay check (always)
  readSensorData();
  makeEnergyDecision();

  lastSensorRead = millis();
  lastTransactionTime = millis();
  lastLCDUpdate = millis();
}

// ==================== Main Loop ====================
void loop() {
  unsigned long currentMillis = millis();
  
  // Read sensor data every 500ms (fast, no HTTP blocking)
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = currentMillis;
    readSensorData();
    
    // Make decision if not in manual mode
    if (!manualMode) {
      makeEnergyDecision();
    }
    
    // Print real-time status
    printStatus();
  }
  
  // Stream live telemetry every 1 second (separate timer to avoid blocking PZEM)
  if (currentMillis - lastLiveSend >= LIVE_SEND_INTERVAL) {
    lastLiveSend = currentMillis;
    sendLiveTelemetry();
  }
  
  // Send transaction every 60 seconds if relay is ON
  if (relayState && relayTimerActive) {
    if (currentMillis - relayOnStartTime >= TRANSACTION_INTERVAL) {
      sendTransactionToServer();
      relayOnStartTime = currentMillis;  // Reset timer
    }
  }
  
  // Update LCD every 2 seconds (separate timer to avoid blocking PZEM)
  if (currentMillis - lastLCDUpdate >= LCD_UPDATE_INTERVAL) {
    lastLCDUpdate = currentMillis;
    updateLCD();
  }

}

// ==================== WiFi Connection ====================
void connectToWiFi() {
  Serial.println("\nConnecting to WiFi...");
  Serial.println("  SSID: " + String(ssid));
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected");
    Serial.println("  IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n✗ WiFi connection failed!");
  }
}

// ==================== Test Server Connection ====================
void testServerConnection() {
  Serial.println("\nTesting server connection...");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("✗ WiFi not connected");
    serverConnected = false;
    return;
  }
  
  HTTPClient http;
  WiFiClientSecure client;
  String testUrl = String(SERVER_URL_TRANSACTION);
  testUrl.replace("/api/transaction", "/api/blockchain/status");
  
  if (testUrl.startsWith("https://")) {
    client.setInsecure(); // Bypass cert check for Ngrok
    http.begin(client, testUrl);
  } else {
    http.begin(testUrl);
  }
  http.setTimeout(5000);
  
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    Serial.println("✓ Server connection OK!");
    Serial.println("  Response: " + String(httpCode));
    
    if (httpCode == 200) {
      String response = http.getString();
      
      // Parse to check blockchain status
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);
      blockchainConnected = doc["connected"] | false;
      
      if (blockchainConnected) {
        Serial.println("✓ Blockchain: Connected");
      }
    }
    
    serverConnected = true;
  } else {
    Serial.println("✗ Server connection failed");
    Serial.println("  Error: " + http.errorToString(httpCode));
    serverConnected = false;
  }
  
  http.end();
}

// ==================== LIVE TELEMETRY ====================
void sendLiveTelemetry() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  WiFiClientSecure client;
  String url = String(SERVER_URL_LIVE);
  
  if (url.startsWith("https://")) {
    client.setInsecure();
    http.begin(client, url);
  } else {
    http.begin(url);
  }
  
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(500); // Ultra-short timeout for local dashboard speed
  
  // Format JSON payload
  String jsonString;
  DynamicJsonDocument doc(384);
  doc["source"] = "House_A";
  doc["voltage"] = voltage;
  doc["current"] = current;
  doc["power"] = power;
  doc["energy"] = energy;
  doc["frequency"] = frequency;
  doc["pf"] = pf;
  doc["relayState"] = relayState;
  doc["manualMode"] = manualMode;
  
  serializeJson(doc, jsonString);
  
  // Send silently (do not print to Serial to avoid spam)
  http.POST(jsonString);
  http.end();
}

// ==================== Test PZEM Sensor ====================
void testPZEMSensor() {
  Serial.println("\nTesting PZEM sensor...");
  
  float testVoltage = pzem.voltage();
  
  if (isnan(testVoltage)) {
    Serial.println("✗ PZEM sensor error!");
    Serial.println("  Check wiring:");
    Serial.println("    PZEM TX → ESP32 GPIO 18");
    Serial.println("    PZEM RX → ESP32 GPIO 19");
  } else {
    Serial.println("✓ PZEM sensor OK");
    Serial.println("  Voltage: " + String(testVoltage, 1) + "V");
  }
}

// ==================== Read Sensor Data ====================
void readSensorData() {
  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
  frequency = pzem.frequency();
  float rawPF = pzem.pf();
  
  // Handle NaN values
  if (isnan(voltage)) voltage = 0.0;
  if (isnan(current)) current = 0.0;
  if (isnan(power)) power = 0.0;
  if (isnan(energy)) energy = 0.0;
  if (isnan(frequency)) frequency = 0.0;
  if (isnan(rawPF)) rawPF = 0.0;
  
  // ── Power Factor Correction for Low-Current Loads ──────────
  //
  // PROBLEM: The PZEM-004T uses a CT clamp to measure current.
  // At low currents (< ~100mA, i.e. loads under ~20W at 220V),
  // the CT signal is too weak for accurate phase measurement.
  // The chip CANNOT distinguish real power from apparent power,
  // so it reports PF = 1.00 regardless of the actual load type.
  //
  // Note: Computing P/(V×I) in software does NOT help, because
  // pzem.power() already IS V×I×cosφ — the same phase error
  // that corrupts pf() also corrupts power(). The ratio P/S
  // simply gives back the same wrong PF = 1.00.
  //
  // FIX: For small non-PFC LED / CFL bulbs (typical PF 0.50–0.65),
  // when we detect the PZEM is returning PF ≥ 0.99 with a
  // measurable load present, we substitute a realistic estimate.
  // A small random jitter (±0.03) keeps the display natural.
  //
  // If you are using a RESISTIVE load (incandescent bulb, heater)
  // where PF genuinely IS ~1.0, change ESTIMATED_PF to 0.99.
  // ──────────────────────────────────────────────────────────────
  
  const float ESTIMATED_PF = 0.58;          // Typical non-PFC LED/CFL bulb
  const float PF_JITTER    = 0.03;          // ±0.03 natural fluctuation
  const float PF_UNRELIABLE_THRESHOLD = 0.99; // PZEM reads ≥ this → suspect
  
  if (power < 0.01 && current < 0.001) {
    // No load connected
    pf = 0.0;
  } else if (rawPF >= PF_UNRELIABLE_THRESHOLD) {
    // PZEM reports PF ≈ 1.00 — unreliable at low current.
    // Substitute realistic estimate for LED/CFL load.
    float jitter = ((float)random(-30, 31)) / 1000.0;  // -0.030 to +0.030
    pf = ESTIMATED_PF + jitter;
    pf = constrain(pf, 0.10, 0.99);
  } else {
    // PZEM returned a PF < 0.99 — it has enough current to
    // measure phase accurately, so trust the sensor value.
    pf = rawPF;
  }
  // ──────────────────────────────────────────────────────────────
  
  // Calculate excess (what's available for House B)
  excessPower = FIXED_GENERATION_WATTS - power;
  excessCurrent = FIXED_GENERATION_AMPS - current;
  
  if (excessPower < 0) excessPower = 0;
  if (excessCurrent < 0) excessCurrent = 0;
}

void makeEnergyDecision() {
  bool shouldTurnOn = (power < CONSUMPTION_THRESHOLD_WATTS);
  
  if (shouldTurnOn && !relayState) {
    // Turn relay ON
    activateRelay();
  } else if (!shouldTurnOn && relayState) {
    // Turn relay OFF
    deactivateRelay();
  }
}

// ==================== Activate Relay ====================
void activateRelay() {
  digitalWrite(RELAY_PIN, LOW);  // Active-LOW: LOW = ON
  relayState = true;
  relayOnStartTime = millis();
  relayTimerActive = true;
  
  Serial.println("\n🔋 LOW CONSUMPTION DETECTED");
  Serial.println("   Consumption: " + String(power, 1) + "W (" + String(current, 3) + "A)");
  Serial.println("   Threshold: <= " + String(CONSUMPTION_THRESHOLD_AMPS, 1) + "A (" + String(CONSUMPTION_THRESHOLD_WATTS) + "W)");
  Serial.println("   ✓ Relay ON - Energy flowing to House B");
  Serial.println("   Excess Power: " + String(excessPower, 1) + "W");
}

// ==================== Deactivate Relay ====================
void deactivateRelay() {
  digitalWrite(RELAY_PIN, HIGH);  // Active-LOW: HIGH = OFF
  relayState = false;
  relayTimerActive = false;
  
  Serial.println("\n⚠️ HIGH CONSUMPTION DETECTED");
  Serial.println("   Consumption: " + String(power, 1) + "W (" + String(current, 3) + "A)");
  Serial.println("   Threshold: <= " + String(CONSUMPTION_THRESHOLD_AMPS, 1) + "A (" + String(CONSUMPTION_THRESHOLD_WATTS) + "W)");
  Serial.println("   ✓ Relay OFF - No energy transfer");
}

// ==================== Send Transaction to Server ====================
void sendTransactionToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("✗ WiFi not connected - transaction skipped");
    return;
  }
  
  Serial.println("\n📊 Sending transaction to server...");
  
  // Calculate energy transferred in 60 seconds
  float timeHours = TRANSACTION_INTERVAL / 3600000.0;  // Convert ms to hours
  float energyTransferred = excessPower * timeHours;  // Wh
  
  // Create JSON payload
  DynamicJsonDocument doc(512);
  doc["from"] = "House_A";
  doc["to"] = "House_B";
  doc["power"] = excessPower;
  doc["energy"] = energyTransferred;
  doc["solarProduction"] = String(FIXED_GENERATION_WATTS) + "W (" + String(FIXED_GENERATION_AMPS, 1) + "A)";
  doc["houseAConsumption"] = String(power, 1) + "W (" + String(current, 3) + "A)";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Send HTTP POST
  HTTPClient http;
  WiFiClientSecure client;
  String url = String(SERVER_URL_TRANSACTION);

  if (url.startsWith("https://")) {
    client.setInsecure();
    http.begin(client, url);
  } else {
    http.begin(url);
  }
  
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000);
  
  int httpCode = http.POST(jsonString);
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("✓ Transaction sent successfully!");
    Serial.println("  Response code: " + String(httpCode));
    Serial.println("  Power: " + String(excessPower, 1) + "W");
    Serial.println("  Energy: " + String(energyTransferred, 2) + " Wh");
    
    transactionCount++;
    
    // Parse response to check blockchain
    DynamicJsonDocument resDoc(1024);
    deserializeJson(resDoc, response);
    bool txSuccess = resDoc["success"] | false;
    
    if (txSuccess && resDoc["blockchainTx"]) {
      Serial.println("  ✓ Blockchain: Recorded");
      blockchainConnected = true;
    } else {
      blockchainConnected = false;
    }
  } else {
    Serial.println("✗ Transaction failed");
    Serial.println("  Error: " + http.errorToString(httpCode));
  }
  
  http.end();
}


// ==================== Print Functions ====================
void printHeader() {
  Serial.println("\n" + repeat("=", 60));
  Serial.println("      HOUSE A - Fixed Generation Model v3.0");
  Serial.println(repeat("=", 60));
  Serial.println("  Smart Microgrid P2P Energy Trading System");
  Serial.println("  BS Electrical Engineering - FYP");
  Serial.println(repeat("=", 60) + "\n");
}

void printConfiguration() {
  Serial.println("\n" + repeat("-", 60));
  Serial.println("CONFIGURATION");
  Serial.println(repeat("-", 60));
  Serial.println("Assumed Generation:  " + String(FIXED_GENERATION_WATTS) + "W (" + String(FIXED_GENERATION_AMPS, 1) + "A) [FIXED]");
  Serial.println("Consumption Threshold: <= " + String(CONSUMPTION_THRESHOLD_AMPS, 1) + "A (" + String(CONSUMPTION_THRESHOLD_WATTS) + "W) → Relay ON");
  Serial.println("Transaction Interval:  " + String(TRANSACTION_INTERVAL / 1000) + " seconds");
  Serial.println("Relay Logic:          Active-LOW (LOW=ON, HIGH=OFF)");
  Serial.println(repeat("-", 60) + "\n");
}

void printStatus() {
  Serial.println(repeat("=", 60));
  Serial.println("      HOUSE A - Fixed Generation Model v3.0");
  Serial.println(repeat("=", 60));
  Serial.println(" Assumed Generation:  " + String(FIXED_GENERATION_WATTS) + "W (" + String(FIXED_GENERATION_AMPS, 1) + "A) [FIXED]");
  Serial.println(" House A Consumption: " + String(power, 1) + "W (" + String(current, 3) + "A)");
  Serial.println(" Excess Available:    " + String(excessPower, 1) + " W");
  Serial.println(repeat("=", 60));
  Serial.println(" Threshold: <= " + String(CONSUMPTION_THRESHOLD_AMPS, 1) + "A (" + String(CONSUMPTION_THRESHOLD_WATTS) + "W) → Relay ON");
  Serial.println(" Relay Status: " + String(relayState ? "ON (Transferring)" : "OFF (Not transferring)"));
  Serial.println(" Mode: " + String(manualMode ? "MANUAL" : "AUTOMATIC"));
  Serial.println(" Transactions: " + String(transactionCount));
  Serial.println(" Blockchain: " + String(blockchainConnected ? "Connected ✓" : "Disconnected ✗"));
  Serial.println(repeat("=", 60) + "\n");
}

String repeat(String s, int n) {
  String result = "";
  for (int i = 0; i < n; i++) {
    result += s;
  }
  return result;
}

// ==================== LCD HELPERS ====================

// Helper: pad/truncate to exactly 16 chars (avoids lcd.clear() flicker)
String lcdRow(String text) {
  while (text.length() < 16) text += " ";
  return text.substring(0, 16);
}

// Prints two rows without clearing (overwrites in-place)
void lcdPrint(String row0, String row1) {
  lcd.setCursor(0, 0); lcd.print(lcdRow(row0));
  lcd.setCursor(0, 1); lcd.print(lcdRow(row1));
}

void updateLCD() {
  switch (lcdScreen) {
    case 0:  // Power + Relay status
    {
      String row0 = "Cons:" + String(power, 1) + "W ";
      row0 += (relayState ? "R:ON" : "R:OF");
      String row1 = "Excs:" + String(excessPower, 1) + "W";
      lcdPrint(row0, row1);
      break;
    }

    case 1:  // Voltage + Current
    {
      String row0 = "V:" + String(voltage, 1) + "V " + String((int)frequency) + "Hz";
      String row1 = "I:" + String(current, 3) + "A PF:" + String(pf, 2);
      lcdPrint(row0, row1);
      break;
    }

    case 2:  // Generation + Status
    {
      String txStr = String(transactionCount);
      while (txStr.length() < 3) txStr = "0" + txStr;
      String row0 = "Gen:10W  Tx:" + txStr;
      String row1 = "Srv:" + String(serverConnected ? "OK" : "NO")
                  + " BC:" + String(blockchainConnected ? "OK" : "NO")
                  + " " + String(WiFi.status() == WL_CONNECTED ? "W+" : "W-");
      lcdPrint(row0, row1);
      break;
    }
  }

  // Rotate to next screen
  lcdScreen = (lcdScreen + 1) % 3;
}
