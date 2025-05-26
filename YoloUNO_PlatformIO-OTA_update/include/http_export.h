#ifndef HTTP_EXPORT_H
#define HTTP_EXPORT_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Forward declaration of UserSession
struct UserSession;

// HTTP Export Class for Google Sheets integration
class HTTPExporter {
private:
    String googleScriptURL;
    bool isEnabled;
    
public:
    HTTPExporter();
    void begin(const String& scriptURL);
    bool exportToGoogleSheets(const UserSession& session);
    void setEnabled(bool enabled) { isEnabled = enabled; }
    bool getEnabled() { return isEnabled; }
};

// Global instance
extern HTTPExporter httpExporter;

// Function declarations
void initHTTPExporter();
bool sendSessionToGoogleSheets(const UserSession& session);

#endif /* HTTP_EXPORT_H */ 