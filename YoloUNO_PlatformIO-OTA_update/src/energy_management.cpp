#include "energy_management.h"
#include "mqtt_task.h"
#include "wifi_task.h"
#include "ap_mode_task.h"
#include "http_export.h"
#include "UNIT_ACMEASURE.h"
#include <Wire.h>

// Global instance
EnergyManager energyManager;
UNIT_ACMEASURE acMeasure;

// Constructor
EnergyManager::EnergyManager() : 
    rfid(SS_PIN, RST_PIN),
    activeSessionIndex(-1),
    currentPower(0.0),
    accumulatedEnergy(0.0),
    lastPowerSample(0),
    sessionStartTime(0),
    deviceInUse(false),
    timeClient(ntpUDP, "pool.ntp.org", 7*3600, 60000) // GMT+7 for Vietnam
{
    // Initialize sessions array
    for (int i = 0; i < MAX_USERS; i++) {
        sessions[i].isActive = false;
        memset(sessions[i].cardId, 0, MAX_CARD_ID_LENGTH);
        sessions[i].startTime = 0;
        sessions[i].endTime = 0;
        sessions[i].totalEnergy = 0.0;
        sessions[i].averagePower = 0.0;
        sessions[i].cost = 0.0;
        strcpy(sessions[i].deviceName, "Shared Device");
    }
}

// Initialize the energy management system
void EnergyManager::begin() {
    Serial.println("Initializing Energy Management System...");
    
    // Initialize SPI for RFID
    SPI.begin();
    rfid.PCD_Init();
    rfid.PCD_DumpVersionToSerial();
    
    // Initialize M5 Stack AC Measure Unit
    if (!acMeasure.begin(&Wire, AC_MEASURE_I2C_ADDR, 21, 22)) {
        Serial.println("Failed to initialize M5 Stack AC Measure Unit!");
    } else {
        Serial.println("M5 Stack AC Measure Unit initialized successfully");
    }
    
    // Initialize NTP client
    timeClient.begin();
    timeClient.update();
    
    Serial.println("Energy Management System initialized successfully");
}

// Main loop function
void EnergyManager::loop() {
    // Update NTP time periodically
    static unsigned long lastNTPUpdate = 0;
    if (millis() - lastNTPUpdate > 60000) { // Update every minute
        timeClient.update();
        lastNTPUpdate = millis();
    }
    
    // Check for RFID card
    if (isCardPresent()) {
        handleCardScan();
    }
    
    // Update power measurement and energy calculation
    if (deviceInUse) {
        updateEnergyCalculation();
        
        // Send telemetry to ThingsBoard every 5 seconds
        static unsigned long lastTelemetry = 0;
        if (millis() - lastTelemetry > 5000) {
            sendEnergyTelemetry();
            lastTelemetry = millis();
        }
    }
}

// Check if RFID card is present
bool EnergyManager::isCardPresent() {
    return rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial();
}

// Get card ID as string
String EnergyManager::getCardId() {
    String cardId = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        cardId += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        cardId += String(rfid.uid.uidByte[i], HEX);
    }
    cardId.toUpperCase();
    return cardId;
}

// Handle RFID card scan
void EnergyManager::handleCardScan() {
    String cardId = getCardId();
    Serial.println("Card detected: " + cardId);
    
    int sessionIndex;
    if (findActiveSession(cardId, sessionIndex)) {
        // End existing session
        endSession(cardId);
        Serial.println("Session ended for card: " + cardId);
    } else {
         // ✅ KIỂM TRA: Có thiết bị đang được dùng không?
        if (deviceInUse && activeSessionIndex >= 0) {
            Serial.printf("❌ Device busy! Card %s is currently using the device\n", 
                         sessions[activeSessionIndex].cardId);
            Serial.println("Please wait for current session to end");
            return; // TỪNG CHỐI thẻ mới
        }
        // Start new session
        startSession(cardId);
        Serial.println("Session started for card: " + cardId);
    }
    
    // Halt PICC
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    
    delay(1000); // Prevent multiple reads
}

// Find active session for a card
bool EnergyManager::findActiveSession(const String& cardId, int& index) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (sessions[i].isActive && strcmp(sessions[i].cardId, cardId.c_str()) == 0) {
            index = i;
            return true;
        }
    }
    return false;
}

// Find empty session slot
bool EnergyManager::findEmptySession(int& index) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (!sessions[i].isActive) {
            index = i;
            return true;
        }
    }
    return false;
}

// Start a new session
void EnergyManager::startSession(const String& cardId) {
    int sessionIndex;
    if (!findEmptySession(sessionIndex)) {
        Serial.println("Error: No available session slots");
        return;
    }
    
    // Initialize session
    strcpy(sessions[sessionIndex].cardId, cardId.c_str());
    sessions[sessionIndex].startTime = timeClient.getEpochTime();
    sessions[sessionIndex].isActive = true;
    sessions[sessionIndex].totalEnergy = 0.0;
    sessions[sessionIndex].averagePower = 0.0;
    sessions[sessionIndex].cost = 0.0;
    
    activeSessionIndex = sessionIndex;
    deviceInUse = true;
    sessionStartTime = millis();
    accumulatedEnergy = 0.0;
    lastPowerSample = millis();
    
    Serial.printf("Session started - Card: %s, Time: %s\n", 
                  cardId.c_str(), 
                  formatDateTime(sessions[sessionIndex].startTime).c_str());
    
    // Send session start event to ThingsBoard
    if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        tb.sendTelemetryData("session_start", 1);
        tb.sendTelemetryData("active_card_id", cardId);
        tb.sendTelemetryData("device_in_use", 1);
        xSemaphoreGive(tbMutex);
    }
}

// End a session
void EnergyManager::endSession(const String& cardId) {
    int sessionIndex;
    if (!findActiveSession(cardId, sessionIndex)) {
        Serial.println("Error: No active session found for card: " + cardId);
        return;
    }
    
    // Finalize session data
    sessions[sessionIndex].endTime = timeClient.getEpochTime();
    sessions[sessionIndex].isActive = false;
    sessions[sessionIndex].totalEnergy = accumulatedEnergy;
    
    // Calculate average power
    unsigned long sessionDuration = (millis() - sessionStartTime) / 1000; // seconds
    if (sessionDuration > 0) {
        sessions[sessionIndex].averagePower = (accumulatedEnergy * 1000.0 * 3600.0) / sessionDuration; // Convert kWh to W
    }
    
    // Calculate cost
    sessions[sessionIndex].cost = sessions[sessionIndex].totalEnergy * ELECTRICITY_RATE;
    
    deviceInUse = false;
    activeSessionIndex = -1;
    
    // Print session summary
    printSessionInfo(sessions[sessionIndex]);
    
    // Export to Excel and send to ThingsBoard
    exportSessionToExcel(sessions[sessionIndex]);
    sendSessionToGoogleSheets(sessions[sessionIndex]);
    sendSessionData(sessions[sessionIndex]);
    
    Serial.printf("Session ended - Card: %s, Energy: %.3f kWh, Cost: %.0f VND\n",
                  cardId.c_str(),
                  sessions[sessionIndex].totalEnergy,
                  sessions[sessionIndex].cost);
}

// Read power from M5 Stack AC Measure Unit
float EnergyManager::readPowerFromM5Stack() {
    // Get power in watts from M5 Stack AC Measure Unit
    uint32_t powerRaw = acMeasure.getPower();
    float power = static_cast<float>(powerRaw) / 10.0f; // Convert to watts (assuming power is in 0.1W units)
    
    // Get voltage and current for logging
    uint16_t voltageRaw = acMeasure.getVoltage();
    uint16_t currentRaw = acMeasure.getCurrent();
    float voltage = static_cast<float>(voltageRaw) / 10.0f; // Convert to volts
    float current = static_cast<float>(currentRaw) / 1000.0f; // Convert to amperes
    
    // Get power factor
    uint8_t powerFactor = acMeasure.getPowerFactor();
    
    // Log measurements
    Serial.printf("AC Measurements - V: %.1fV, I: %.3fA, P: %.1fW, PF: %d%%\n", 
                  voltage, current, power, powerFactor);
    
    return power;
}

// Update energy calculation
void EnergyManager::updateEnergyCalculation() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastPowerSample >= POWER_SAMPLE_INTERVAL) {
        // Read current power
        currentPower = readPowerFromM5Stack();
        
        // Calculate energy increment (kWh)
        float timeDelta = (currentTime - lastPowerSample) / 3600000.0; // Convert ms to hours
        float energyIncrement = (currentPower / 1000.0) * timeDelta; // Convert W to kW and multiply by time
        
        accumulatedEnergy += energyIncrement;
        lastPowerSample = currentTime;
        
        Serial.printf("Power: %.2f W, Energy: %.6f kWh\n", currentPower, accumulatedEnergy);
    }
}

// Format timestamp to readable string
String EnergyManager::formatDateTime(unsigned long timestamp) {
    setTime(timestamp);
    char buffer[32];
    sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d", 
            day(), month(), year(), hour(), minute(), second());
    return String(buffer);
}

// Export session data to Excel (via HTTP API)
void EnergyManager::exportSessionToExcel(const UserSession& session) {
    if (!wifiConnected || apMode) {
        Serial.println("Cannot export to Excel: No internet connection");
        return;
    }
    
    // For now, just print the data that would be exported
    // TODO: Implement actual HTTP POST to Google Apps Script
    Serial.println("=== EXCEL EXPORT DATA ===");
    Serial.printf("Card ID: %s\n", session.cardId);
    Serial.printf("Start Time: %s\n", formatDateTime(session.startTime).c_str());
    Serial.printf("End Time: %s\n", formatDateTime(session.endTime).c_str());
    Serial.printf("Total Energy: %.6f kWh\n", session.totalEnergy);
    Serial.printf("Average Power: %.2f W\n", session.averagePower);
    Serial.printf("Cost: %.0f VND\n", session.cost);
    Serial.printf("Device: %s\n", session.deviceName);
    Serial.println("========================");
    
    // Create JSON payload for future HTTP implementation
    DynamicJsonDocument doc(1024);
    doc["cardId"] = session.cardId;
    doc["startTime"] = session.startTime; // Unix timestamp for Google Apps Script
    doc["endTime"] = session.endTime; // Unix timestamp for Google Apps Script
    doc["totalEnergy"] = session.totalEnergy;
    doc["averagePower"] = session.averagePower;
    doc["cost"] = session.cost;
    doc["deviceName"] = session.deviceName;
    
    String jsonString;
    serializeJson(doc, jsonString);
    Serial.println("JSON Data: " + jsonString);
}

// Send energy telemetry to ThingsBoard
void EnergyManager::sendEnergyTelemetry() {
    if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        tb.sendTelemetryData("current_power", currentPower);
        tb.sendTelemetryData("accumulated_energy", accumulatedEnergy);
        tb.sendTelemetryData("device_in_use", deviceInUse ? 1 : 0);
        
        if (activeSessionIndex >= 0) {
            tb.sendTelemetryData("session_duration", (millis() - sessionStartTime) / 1000);
            tb.sendTelemetryData("estimated_cost", accumulatedEnergy * ELECTRICITY_RATE);
        }
        
        xSemaphoreGive(tbMutex);
    }
}

// Send completed session data to ThingsBoard
void EnergyManager::sendSessionData(const UserSession& session) {
    if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        tb.sendTelemetryData("session_end", 1);
        tb.sendTelemetryData("session_energy", session.totalEnergy);
        tb.sendTelemetryData("session_cost", session.cost);
        tb.sendTelemetryData("session_avg_power", session.averagePower);
        tb.sendTelemetryData("session_duration_total", session.endTime - session.startTime);
        
        xSemaphoreGive(tbMutex);
    }
}

// Print session information
void EnergyManager::printSessionInfo(const UserSession& session) {
    Serial.println("=== SESSION SUMMARY ===");
    Serial.printf("Card ID: %s\n", session.cardId);
    Serial.printf("Device: %s\n", session.deviceName);
    Serial.printf("Start Time: %s\n", formatDateTime(session.startTime).c_str());
    Serial.printf("End Time: %s\n", formatDateTime(session.endTime).c_str());
    Serial.printf("Duration: %lu seconds\n", session.endTime - session.startTime);
    Serial.printf("Total Energy: %.6f kWh\n", session.totalEnergy);
    Serial.printf("Average Power: %.2f W\n", session.averagePower);
    Serial.printf("Cost: %.0f VND\n", session.cost);
    Serial.println("=======================");
}

// Initialize energy management (called from main)
void initEnergyManagement() {
    energyManager.begin();
}

// Energy management task for RTOS
void energyManagementTask(void *parameter) {
    while (true) {
        // Run the energy management loop
        energyManager.loop();
        
        // If device is in use, update power measurements more frequently
        if (energyManager.isDeviceInUse()) {
            // Update power measurements every 200ms for more accurate readings
            static unsigned long lastPowerUpdate = 0;
            if (millis() - lastPowerUpdate >= 200) {
                // Get power measurements
                uint32_t powerRaw = acMeasure.getPower();
                uint16_t voltageRaw = acMeasure.getVoltage();
                uint16_t currentRaw = acMeasure.getCurrent();
                uint32_t apparentPowerRaw = acMeasure.getApparentPower();
                uint8_t powerFactor = acMeasure.getPowerFactor();
                
                // Convert to real units
                float power = static_cast<float>(powerRaw) / 10.0f; // W
                float voltage = static_cast<float>(voltageRaw) / 10.0f; // V
                float current = static_cast<float>(currentRaw) / 1000.0f; // A
                float apparentPower = static_cast<float>(apparentPowerRaw) / 10.0f; // VA
                
                // Send telemetry to ThingsBoard
                if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                    tb.sendTelemetryData("voltage", voltage);
                    tb.sendTelemetryData("current", current);
                    tb.sendTelemetryData("power", power);
                    tb.sendTelemetryData("apparent_power", apparentPower);
                    tb.sendTelemetryData("power_factor", powerFactor);
                    xSemaphoreGive(tbMutex);
                }
                
                lastPowerUpdate = millis();
            }
        }
        
        // Give other tasks a chance to run
        vTaskDelay(pdMS_TO_TICKS(50));
    }
} 