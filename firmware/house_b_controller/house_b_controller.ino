/*
 * Smart Microgrid - House B Controller v2.1 - Prototype Production
 * 
 * This ESP32 monitors:
 * - PZEM #2: House B total consumption (from grid + from House A)
 * 
 * Functions:
 * 1. Monitors House B energy consumption
 * 2. Detects relay state via voltage presence (voltage-based, tested)
 * 3. Sends live telemetry to local web dashboard
 * 4. Tracks energy received from House A
 * 5. Records consumption data for AI predictions
 * 
 * Hardware Connections:
 * - PZEM #2 (House B Consumption): TX→GPIO16, RX→GPIO17 (Serial2)
 * 
 * Version: 2.1.0 - Prototype Production
 * Date: January 2026
 */

#include <PZEM004Tv30.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Include secrets file (copy secrets.h.template to secrets.h and configure)
#include "secrets.h"

// ==================== PIN DEFINITIONS ====================

#define LED_PIN 2           // Built-in LED

// PZEM #2 - House B Consumption (Hardware Serial2)
#define PZEM2_RX_PIN 16
#define PZEM2_TX_PIN 17

// ==================== TIMING CONFIGURATION ====================

const unsigned long READING_INTERVAL = 500;         // Read sensor every 0.5 seconds (faster response)
const unsigned long LIVE_SEND_INTERVAL = 500;       // Send live telemetry every 500ms (max speed)
const unsigned long SERVER_SEND_INTERVAL = 30000;   // Send to server every 30 seconds

// ==================== GLOBAL VARIABLES ====================

// PZEM Sensor
HardwareSerial SerialPZEMB(2);
PZEM004Tv30 pzemHouseB(SerialPZEMB, PZEM2_RX_PIN, PZEM2_TX_PIN);

// Timing variables
unsigned long lastReadingTime = 0;
unsigned long lastLiveSendTime = 0;
unsigned long lastServerSendTime = 0;

// House B Consumption Data (PZEM #2)
float houseBVoltage = 0.0;
float houseBCurrent = 0.0;
float houseBPower = 0.0;
float houseBEnergy = 0.0;
float houseBFrequency = 0.0;
float houseBPF = 0.0;

// Tracking
float previousPower = 0.0;
bool receivingFromHouseA = false;
float totalEnergyReceived = 0.0;

// System State
bool wifiConnected = false;
bool serverConnected = false;

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("Smart Microgrid - House B Controller v2.0");
  Serial.println("Blockchain Integration Edition");
  Serial.println("========================================\n");
  
  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Connect to WiFi
  connectWiFi();
  
  // Test PZEM sensor
  testPZEMSensor();
  
  // Test server connection
  if (wifiConnected) {
    testServerConnection();
  }
  
  Serial.println("\n========================================");
  Serial.println("System Ready!");
  Serial.println("========================================\n");
}

// ==================== MAIN LOOP ====================

void loop() {
  unsigned long currentTime = millis();
  
  // Read sensor data every 500ms (fast, no HTTP blocking)
  if (currentTime - lastReadingTime >= READING_INTERVAL) {
    lastReadingTime = currentTime;
    readSensorData();
    detectEnergySource();
    printStatus();
  }
  
  // Stream live telemetry every 1 second (separate from sensor reads)
  if (currentTime - lastLiveSendTime >= LIVE_SEND_INTERVAL) {
    lastLiveSendTime = currentTime;
    sendLiveTelemetry();
  }
  
  // Send to backend server every 30 seconds
  if (currentTime - lastServerSendTime >= SERVER_SEND_INTERVAL) {
    lastServerSendTime = currentTime;
    if (wifiConnected) {
      sendDataToServer();
    }
  }
  
  // Blink LED
  blinkLED();
}

// ==================== WIFI CONNECTION ====================

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n✓ WiFi Connected!");
    Serial.print("  IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println("\n⚠️  WiFi Connection Failed!");
  }
}

// ==================== SENSOR FUNCTIONS ====================

void testPZEMSensor() {
  Serial.println("\nTesting PZEM Sensor...");
  delay(1000);
  
  float testVoltage = pzemHouseB.voltage();
  if (isnan(testVoltage)) {
    Serial.println("⚠️  PZEM #2 (House B) not responding!");
  } else {
    Serial.println("✓ PZEM #2 (House B Consumption) OK");
  }
}

void readSensorData() {
  // Read PZEM #2 - House B Consumption
  houseBVoltage = pzemHouseB.voltage();
  houseBCurrent = pzemHouseB.current();
  houseBPower = pzemHouseB.power();
  houseBEnergy = pzemHouseB.energy();
  houseBFrequency = pzemHouseB.frequency();
  houseBPF = pzemHouseB.pf();
  
  // Handle NaN values
  if (isnan(houseBVoltage)) houseBVoltage = 0.0;
  if (isnan(houseBCurrent)) houseBCurrent = 0.0;
  if (isnan(houseBPower)) houseBPower = 0.0;
  if (isnan(houseBEnergy)) houseBEnergy = 0.0;
  if (isnan(houseBFrequency)) houseBFrequency = 0.0;
  if (isnan(houseBPF)) houseBPF = 0.0;
  
  // Filter out ghost readings when relay is OFF
  if (houseBVoltage < 10.0) {
    houseBPower = 0.0;
    houseBCurrent = 0.0;
  }
}

void detectEnergySource() {
  // Detect relay state by VOLTAGE (tested method)
  // When relay ON → Voltage present (>100V)
  // When relay OFF → No voltage (0V)
  
  if (houseBVoltage > 100.0) {
    // Voltage detected → Relay is ON
    if (!receivingFromHouseA) {
      receivingFromHouseA = true;
      Serial.println("\n🔌 RELAY ON - Receiving energy from House A");
    }
    
    // Track total energy received (convert Ws to kWh)
    totalEnergyReceived = houseBEnergy;
  } else {
    // No voltage → Relay is OFF
    if (receivingFromHouseA) {
      receivingFromHouseA = false;
      Serial.println("\n⏸️  RELAY OFF - Stopped receiving from House A");
    }
  }
}

// ==================== SERVER COMMUNICATION ====================

void testServerConnection() {
  Serial.print("\nTesting server connection: ");
  Serial.println(SERVER_URL_CONSUMPTION);
  
  HTTPClient http;
  WiFiClientSecure client;
  String testUrl = String(SERVER_URL_CONSUMPTION);
  testUrl.replace("/api/consumption", "/api/blockchain/status");
  
  if (testUrl.startsWith("https://")) {
    client.setInsecure();
    http.begin(client, testUrl);
  } else {
    http.begin(testUrl);
  }
  http.setTimeout(5000);
  
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    serverConnected = true;
    Serial.println("✓ Server connected!");
  } else {
    serverConnected = false;
    Serial.print("⚠️  Server connection failed: ");
    Serial.println(httpCode);
  }
  
  http.end();
}

void sendDataToServer() {
  if (!wifiConnected) return;
  
  HTTPClient http;
  WiFiClientSecure client;
  String url = String(SERVER_URL_CONSUMPTION);
  
  if (url.startsWith("https://")) {
    client.setInsecure();
    http.begin(client, url);
  } else {
    http.begin(url);
  }
  
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000);
  
  DynamicJsonDocument doc(400);
  doc["house"] = "House_B";
  doc["power"] = houseBPower;
  doc["energy"] = houseBEnergy;
  doc["voltage"] = houseBVoltage;
  doc["current"] = houseBCurrent;
  doc["frequency"] = houseBFrequency;
  doc["powerFactor"] = houseBPF;
  doc["receivingFromHouseA"] = receivingFromHouseA;
  doc["totalReceived"] = totalEnergyReceived;
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  
  Serial.println("\n📡 Sending data to server...");
  int httpResponseCode = http.POST(jsonPayload);
  
  if (httpResponseCode > 0) {
    serverConnected = true;
    String response = http.getString();
    Serial.print("✓ Server Response (");
    Serial.print(httpResponseCode);
    Serial.print("): ");
    Serial.println(response);
  } else {
    serverConnected = false;
    Serial.print("⚠️  HTTP Error: ");
    Serial.println(httpResponseCode);
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
  DynamicJsonDocument doc(256);
  doc["source"] = "House_B";
  doc["voltage"] = houseBVoltage;
  doc["current"] = houseBCurrent;
  doc["power"] = houseBPower;
  doc["energy"] = houseBEnergy;
  doc["receivingFromHouseA"] = receivingFromHouseA;
  
  serializeJson(doc, jsonString);
  
  // Send silently (do not print to Serial to avoid spam)
  http.POST(jsonString);
  http.end();
}


// ==================== DISPLAY FUNCTIONS ====================

void printStatus() {
  Serial.println("\n╔════════════════════════════════════════════════╗");
  Serial.println("║      HOUSE B - Energy Monitor v2.1             ║");
  Serial.println("╠════════════════════════════════════════════════╣");
  
  Serial.print("║ Consumption:  ");
  Serial.print(houseBPower, 1);
  Serial.println(" W                          ║");
  
  Serial.print("║ Voltage:      ");
  Serial.print(houseBVoltage, 1);
  Serial.println(" V                          ║");
  
  Serial.print("║ Current:      ");
  Serial.print(houseBCurrent, 3);
  Serial.println(" A                          ║");
  
  Serial.println("╠════════════════════════════════════════════════╣");
  
  Serial.print("║ Energy Source: ");
  if (receivingFromHouseA) {
    Serial.println("House A (Solar)              ║");
  } else {
    Serial.println("Grid                         ║");
  }
  
  Serial.print("║ Total Received: ");
  Serial.print(totalEnergyReceived, 3);
  Serial.println(" kWh                   ║");
  
  Serial.print("║ Server: ");
  Serial.print(serverConnected ? "Connected ✓" : "Disconnected");
  Serial.println("                         ║");
  
  Serial.println("╚════════════════════════════════════════════════╝\n");
}

void blinkLED() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  // Blink faster if receiving from House A
  int blinkInterval = receivingFromHouseA ? 250 : 1000;
  
  if (millis() - lastBlink > blinkInterval) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }
}
