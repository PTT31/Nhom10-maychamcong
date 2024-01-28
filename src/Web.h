#include "include.h"
void WebQueryprocess(AsyncWebServerRequest *request);
void handleWebQuery(AsyncWebServerRequest *request);
void handleCheckDelID(AsyncWebServerRequest *request);
void handleFileDownload(AsyncWebServerRequest *request);
uint8_t checkAddID( AsyncWebServerRequest *request);
String getLastLines(const char* filePath, size_t lineCount);
bool saveWiFiCredentials(const char *ssid, const char *password);
bool readWiFiCredentials(char *ssid, char *password);
void setupServer();