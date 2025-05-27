#ifndef ENERGY_MANAGEMENT_H
#define ENERGY_MANAGEMENT_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include "UNIT_ACMEASURE.h"

// RFID Configuration
#define RST_PIN         22
#define SS_PIN          21
#define RFID_SPI_SPEED  1000000

// M5 Stack AC Measure Unit I2C Address
#define AC_MEASURE_I2C_ADDR 0x42

// Energy Management Constants
#define MAX_USERS           50
#define MAX_CARD_ID_LENGTH  20
#define ELECTRICITY_RATE    3500.0  // VND per kWh
#define POWER_SAMPLE_INTERVAL 1000  // ms
#define MAX_SESSION_TIME    86400   // 24 hours in seconds

// User Session Structure
struct UserSession {
    char cardId[MAX_CARD_ID_LENGTH];
    unsigned long startTime;
    unsigned long endTime;
    float totalEnergy;      // kWh
    float averagePower;     // W
    float cost;            // VND
    bool isActive;
    char deviceName[32];
};

// Energy Management Class
class EnergyManager {
private:
    MFRC522 rfid;
    UserSession sessions[MAX_USERS];
    int activeSessionIndex;
    float currentPower;
    float accumulatedEnergy;
    unsigned long lastPowerSample;
    unsigned long sessionStartTime;
    bool deviceInUse;
    
    // NTP for time synchronization
    WiFiUDP ntpUDP;
    NTPClient timeClient;
    
    // Helper functions
    String getCardId();
    bool findActiveSession(const String& cardId, int& index);
    bool findEmptySession(int& index);
    float readPowerFromM5Stack();
    void updateEnergyCalculation();
    String formatDateTime(unsigned long timestamp);
    bool sendToExcel(const UserSession& session);
    bool sendToThingsBoard(const UserSession& session);
    
public:
    EnergyManager();
    void begin();
    void loop();
    bool isCardPresent();
    void handleCardScan();
    void startSession(const String& cardId);
    void endSession(const String& cardId);
    float getCurrentPower() { return currentPower; }
    bool isDeviceInUse() { return deviceInUse; }
    void exportSessionToExcel(const UserSession& session);
    void printSessionInfo(const UserSession& session);
    
    // ThingsBoard integration
    void sendEnergyTelemetry();
    void sendSessionData(const UserSession& session);
};

// Global instances
extern EnergyManager energyManager;
extern UNIT_ACMEASURE acMeasure;

// Function declarations for task integration
void energyManagementTask(void *parameter);
void initEnergyManagement();

#endif /* ENERGY_MANAGEMENT_H */ 