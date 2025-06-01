#ifndef GOOGLE_SHEETS_TASK_H
#define GOOGLE_SHEETS_TASK_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoHttpClient.h>

// Google Apps Script Configuration
extern const char* GOOGLE_SCRIPT_ID;
extern const char* GOOGLE_SCRIPT_URL;

// Attendance data structure
struct AttendanceData {
  String cardId;
  String name;
  String status; // "IN" or "OUT"
  String date;
  String time;
};

// Function declarations
void googleSheetsTask(void *parameter);
void setupGoogleSheets();
bool sendAttendanceToGoogleSheets(const AttendanceData &data);
String getUserNameFromCard(const String &cardId);
bool isCardRegistered(const String &cardId);
void processRFIDForAttendance(const String &cardId);
String getCurrentDateTime();
String determineAttendanceStatus(const String &cardId);

// Database sync functions
bool syncDatabaseFromGoogleSheets();
bool loadUserDatabaseFromSheets();
void printCurrentDatabase();
unsigned long getLastSyncTime();
bool parseDatabaseResponse(const String &response);

#endif /* GOOGLE_SHEETS_TASK_H */ 