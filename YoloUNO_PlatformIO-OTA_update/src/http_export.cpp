#include "http_export.h"
#include "energy_management.h"
#include "wifi_task.h"
#include "ap_mode_task.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Global instance
HTTPExporter httpExporter;

// Constructor
HTTPExporter::HTTPExporter() : isEnabled(false) {
    googleScriptURL = "";
}

// Initialize HTTP exporter
void HTTPExporter::begin(const String& scriptURL) {
    googleScriptURL = scriptURL;
    isEnabled = !scriptURL.isEmpty();
    
    if (isEnabled) {
        Serial.println("HTTP Exporter initialized with URL: " + scriptURL);
    } else {
        Serial.println("HTTP Exporter disabled - no URL provided");
    }
}

// Export session to Google Sheets using raw WiFiClientSecure
bool HTTPExporter::exportToGoogleSheets(const UserSession& session) {
    if (!isEnabled) {
        Serial.println("HTTP Export disabled - no URL configured");
        return false;
    }
    
    // Check WiFi connection
    if (!wifiConnected || apMode) {
        Serial.println("HTTP Export failed - no internet connection");
        return false;
    }
    
    Serial.println("=== REAL HTTP EXPORT TO GOOGLE SHEETS ===");
    Serial.printf("URL: %s\n", googleScriptURL.c_str());
    
    // Create JSON payload
    DynamicJsonDocument doc(1024);
    doc["cardId"] = session.cardId;
    doc["startTime"] = session.startTime;
    doc["endTime"] = session.endTime;
    doc["totalEnergy"] = session.totalEnergy;
    doc["averagePower"] = session.averagePower;
    doc["cost"] = session.cost;
    doc["deviceName"] = session.deviceName;
    
    String jsonString;
    serializeJson(doc, jsonString);
    Serial.println("JSON Payload: " + jsonString);
    
    // Use WiFiClientSecure directly
    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate verification
    
    // Connect to Google Apps Script
    if (!client.connect("script.google.com", 443)) {
        Serial.println("❌ Connection to script.google.com failed");
        return false;
    }
    
    // Extract path from URL
    String path = googleScriptURL;
    path.replace("https://script.google.com", "");
    
    // Create HTTP POST request
    String httpRequest = "POST " + path + " HTTP/1.1\r\n";
    httpRequest += "Host: script.google.com\r\n";
    httpRequest += "Content-Type: application/json\r\n";
    httpRequest += "Content-Length: " + String(jsonString.length()) + "\r\n";
    httpRequest += "Connection: close\r\n\r\n";
    httpRequest += jsonString;
    
    Serial.println("Sending HTTPS POST request...");
    client.print(httpRequest);
    
    // Wait for response
    unsigned long timeout = millis() + 10000; // 10 second timeout
    while (client.available() == 0 && millis() < timeout) {
        delay(50);
    }
    
    bool success = false;
    if (client.available()) {
        String response = client.readString();
        Serial.println("Raw Response:");
        Serial.println(response);
        
        // Check if response contains HTTP 200
        if (response.indexOf("HTTP/1.1 200") >= 0 || response.indexOf("HTTP/1.0 200") >= 0) {
            Serial.println("✅ Excel export successful!");
            success = true;
        } else {
            Serial.println("❌ Excel export failed - non-200 response");
        }
    } else {
        Serial.println("❌ No response received");
    }
    
    client.stop();
    Serial.println("==========================================");
    
    return success;
}

// Initialize HTTP exporter (called from main)
void initHTTPExporter() {
    // Enable HTTP export with Google Apps Script URL
    // Replace with actual Google Apps Script deployment URL
    httpExporter.begin("https://script.google.com/macros/s/AKfycbzlkkOhrw5qGugiNZ0AgqVh_tFxIA6CwiJwA7D5dVJAC3i12uKIPoP-dPeIH6gGTiGe/exec");
    
    // To disable HTTP export, use empty string:
    // httpExporter.begin("");
}

// Helper function to send session data
bool sendSessionToGoogleSheets(const UserSession& session) {
    return httpExporter.exportToGoogleSheets(session);
} 