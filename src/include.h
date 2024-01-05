#include <stdio.h>
#include <stdlib.h>
#include <Arduino.h>
#include "RTClib.h"               // Thư viện để làm việc với module RTC DS1307
#include <Adafruit_Fingerprint.h> // Thư viện để làm việc với cảm biến vân tay
#include <WiFi.h>                 // Thư viện WiFi
#include <DNSServer.h>            // Thư viện DNSServer để xử lý DNS trên AP
#include "SPIFFS.h"
#include "U8g2lib.h"
#include "SD.h"
#include <SPI.h>
#include <ESP32Time.h>
#include <sqlite3.h>
#include <SPI.h>
#include <FS.h>
#include "SD.h"
#include <AsyncTCP.h>             // Thư viện AsyncTCP cho ESPAsyncWebServer
#include "ESPAsyncWebServer.h"    // Thư viện ESPAsyncWebServer

#define PinBuzz 12