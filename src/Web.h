#include "include.h"
void WebQueryprocess(AsyncWebServerRequest *request);
void handleWebQuery(AsyncWebServerRequest *request);
void handleCheckDelID(Adafruit_Fingerprint finger,AsyncWebServerRequest *request);
uint8_t checkAddID(Adafruit_Fingerprint finger, AsyncWebServerRequest *request);
bool saveWiFiCredentials(const char *ssid, const char *password);
bool readWiFiCredentials(char *ssid, char *password);
void setupServer(Adafruit_Fingerprint finger);