#include "Web.h"
#include "Database.h"
#include "finger.h"
#include "lcd.h"

extern message_lcd mess;
extern unsigned long startTime;
extern Adafruit_Fingerprint finger;
AsyncWebServer server(80);
AsyncEventSource events("/events");
void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}
String getLastLines(const char *filePath, size_t lineCount)
{
    File file = SD.open(filePath, FILE_READ);
    if (!file)
    {
        return "Unable to open file";
    }

    std::vector<String> lines;
    while (file.available())
    {
        String line = file.readStringUntil('\n');
        lines.push_back(line);
        if (lines.size() > lineCount)
        {
            lines.erase(lines.begin());
        }
    }
    file.close();

    String result;
    for (const String &line : lines)
    {
        result += line + "\n";
    }
    return result;
}
void setupServer()
{
    server.serveStatic("/", SD, "/Web/");
    server.addHandler(&events);
    server.on("/get-last-lines", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        AsyncResponseStream *response = request->beginResponseStream("text/plain");
        String lastLines = getLastLines("/record/Login.csv", 50);
        response->print(lastLines);
        request->send(response); });
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
            String ssid = request->getParam("ssid", true)->value();
            String password = request->getParam("password", true)->value();
            saveWiFiCredentials(ssid.c_str(), password.c_str());
            request->send(200, "text/plain", "Saved WiFi credentials!");
            delay(1000);
            ESP.restart(); // Khởi động lại ESP32 để kết nối với WiFi mới cấu hình
        } else {
            request->send(400, "text/plain", "Missing parameters");
        } });
    // First request will return 0 results unless you start scan from somewhere else (loop/setup)
    // Do not request more often than 3-5 seconds
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request)
              {
  String json = "[";
  int n = WiFi.scanComplete();
  if(n == -2){
    WiFi.scanNetworks(true);
  } else if(n){
    for (int i = 0; i < n; ++i){
      if(i) json += ",";
      json += "{";
      json += "\"rssi\":"+String(WiFi.RSSI(i));
      json += ",\"ssid\":\""+WiFi.SSID(i)+"\"";
      json += ",\"bssid\":\""+WiFi.BSSIDstr(i)+"\"";
      json += ",\"channel\":"+String(WiFi.channel(i));
      json += ",\"secure\":"+String(WiFi.encryptionType(i));
      json += "}";
    }
    WiFi.scanDelete();
    if(WiFi.scanComplete() == -2){
      WiFi.scanNetworks(true);
    }
  }
  json += "]";
  request->send(200, "application/json", json);
  json = String(); });
    server.on("/webquery", HTTP_GET, [](AsyncWebServerRequest *request)
              { handleWebQuery(request); });
    server.on("/query_db", HTTP_POST, [](AsyncWebServerRequest *request)
              { WebQueryprocess(request); });
    // server.on("/addID", HTTP_POST, [finger](AsyncWebServerRequest *request)
    //           { checkAddID(finger,request); });
    server.on("/addid", HTTP_POST, [](AsyncWebServerRequest *request)
              { checkAddID(request); });
    server.on("/handleCheckDelID", HTTP_POST, [](AsyncWebServerRequest *request)
              { handleCheckDelID(request); });
    server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request)
              { handleFileDownload(request); });

    server.begin();
}
void handleWebQuery(AsyncWebServerRequest *request)
{
    String temp;
    int sec = millis() / 1000;
    int min = sec / 60;
    int hr = min / 60;

    temp = "<html><head>\
      <title>MCC-Nhom10</title>\
      <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; font-size: large; Color: #000088; }\
      </style>\
  </head>\
  <body>\
      <h1>Hello from MCC-Nhom10!</h1>\
      <p>Uptime: ";
    temp += hr;
    temp += ":";
    temp += min % 60;
    temp += ":";
    temp += sec % 60;
    temp += "</p>\
      <h2>Query gendered names database</h2>\
      <form name='params' method='POST' action='/query_db'>\
      Enter from: <input type=text style='font-size: large' value='Pham' name='from'/> \
      <br>to: <input type=text style='font-size: large' value='PhamTung' name='to'/> \
      <br><br><input type=submit style='font-size: large' value='Query database'/>\
      </form>\
  </body>\
  </html>";
    request->send(200, "text/html; charset=UTF-8", temp);
}

void WebQueryprocess(AsyncWebServerRequest *request)
{

    sqlite3 *db1;
    sqlite3_stmt *res;
    const char *tail;
    int rec_count = 0;

    String sql = "Select count(*) from users where name between '";
    sql += request->getParam("from", true)->value();
    sql += "' and '";
    sql += request->getParam("to", true)->value();
    sql += "'";
    int rc = sqlite3_open(USER_DB, &db1);
    rc = sqlite3_prepare_v2(db1, sql.c_str(), -1, &res, &tail);
    if (rc != SQLITE_OK)
    {
        String resp = "Failed to fetch data: ";
        resp += sqlite3_errmsg(db1);
        resp += ".<br><br><input type=button onclick='location.href=\"/\"' value='back'/>";
        request->send(200, "text/html; charset=UTF-8", resp);
        Serial.println(resp);
        sqlite3_finalize(res);
        sqlite3_close(db1);
        return;
    }
    while (sqlite3_step(res) == SQLITE_ROW)
    {
        rec_count = sqlite3_column_int(res, 0);
        if (rec_count > 5000)
        {
            String resp = "Too many records: ";
            resp += rec_count;
            resp += ". Please select different range";
            resp += ".<br><br><input type=button onclick='location.href=\"/\"' value='back'/>";
            request->send(200, "text/html; charset=UTF-8", resp);
            Serial.println(resp.c_str());
            sqlite3_finalize(res);
            return;
        }
    }
    sqlite3_finalize(res);
    sql = "Select * from users where name between'";
    sql += request->getParam("from", true)->value();
    sql += "' and '";
    sql += request->getParam("to", true)->value();
    sql += "'";
    Serial.print("sql");
    rc = sqlite3_prepare_v2(db1, sql.c_str(), -1, &res, &tail);
    if (rc != SQLITE_OK)
    {
        String resp = "Failed to fetch data: ";
        resp += sqlite3_errmsg(db1);
        resp += ".<br><br><input type=button onclick='location.href=\"/\"' value='back'/>";
        resp += ".<br><br><input type=button onclick='location.href=\"/\"' value='back'/>";
        request->send(200, "text/html; charset=UTF-8", resp);
        Serial.println(resp.c_str());
        return;
    }
    AsyncResponseStream *response = request->beginResponseStream("text/html; charset=UTF-8");
    rec_count = 0;
    String resp = "<html><head><title>ESP32 Sqlite local database query through web server</title>\
          <style>\
          body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; font-size: large; Color: #000088; }\
          </style><head><body><h1>Sqlite local database query through web server</h1><h2>";
    resp += sql;
    resp += "</h2><br><table cellspacing='1' cellpadding='1' border='1'><tr><td>finger_id</td><td>Name</td><td>position</td>";
    response->print(resp);
    while (sqlite3_step(res) == SQLITE_ROW)
    {
        resp = "<tr><td>";
        resp += sqlite3_column_int(res, 0);
        resp += "</td><td>";
        resp += (const char *)sqlite3_column_text(res, 1);
        resp += "</td><td>";
        resp += (const char *)sqlite3_column_text(res, 2);
        rec_count++;
        response->print(resp);
    }
    resp = "</table><br><br>Number of records: ";
    resp += rec_count;
    resp += "<br><br><input type=button onclick='location.href=\"/webquery\"' value='back'/>";
    response->print(resp);
    request->send(response);
    sqlite3_finalize(res);
    sqlite3_close(db1);
}

uint8_t checkAddID(AsyncWebServerRequest *request)
{
    if (request->hasParam("name", true) && request->hasParam("position", true))
    {
        String name = request->getParam("name", true)->value();
        String position = request->getParam("position", true)->value();
        Serial.print(name);
        Serial.print(position);
        uint8_t emptyID = findEmptyID(); // Tìm ID trống
        Serial.print("B1 ");
        Serial.print(emptyID);
        if (emptyID != -1)
        {
            mess.mode = Insert_finger;
            Serial.println("B2 ");
            db_insert(emptyID, name, position); // Them van tay vao Database
            Serial.println("B3 ");
            request->send(SD, "/Web/event/index.htm");
            return 1;
        }
        else
        {
            request->send(404, "text/plain", "Full sensor memory. Fingerprints cannot be added anymore");
            return 0;
        }
    }
    else
    {
        request->send(404, "text/plain", "Fingerprints don't have params");
        return 0;
    }
}
void handleCheckDelID(AsyncWebServerRequest *request)
{
    String nameValue;

    if (request->hasParam("from"))
    {
        nameValue = request->hasParam("from");
        uint8_t id = isIDPresent(nameValue);
        // Kiểm tra xem ID có tồn tại hay không
        if (id != 0)
        {
            // Xóa dữ liệu từ cảm biến vân tay
            uint8_t idToDelete = id;
            uint8_t enrollSuccess = deleteFinger(finger, idToDelete);
            if (enrollSuccess != 0)
            {
                addNumberInFile(idToDelete);
            }
            // Xóa dữ liệu khỏi cơ sở dữ liệu của bạn ở đây
            bool success = db_delete(nameValue); // Ví dụ: hàm xóa dữ liệu với số nhận được
            if (success)
            {
                request->send(200, "text/plain", "Data with number " + nameValue + " deleted successfully");
            }
            else
            {
                request->send(500, "text/plain", "Failed to delete data");
            }
        }
        else
        {
            // Nếu ID không tồn tại, gửi thông báo tương ứng
            request->send(200, "text/plain", "ID not found");
        }
    }
    else
    {
        request->send(400, "text/plain", "No 'id' value provided");
    }
}
bool saveWiFiCredentials(const char *ssid, const char *password)
{
    File file = SPIFFS.open("/wifi_credentials.txt", FILE_WRITE);
    if (file)
    {
        if (!file)
        {
            Serial.println("- failed to open file for writing");
            return false;
        }
        file.println(ssid);
        if (file.println(password))
        {
            Serial.println("- file written");
        }
        else
        {
            Serial.println("- write failed");
        }

        file.close();
        return true;
    }
    return false;
}
bool readWiFiCredentials(char *ssid, char *password)
{
    File file = SPIFFS.open("/wifi_credentials.txt", "r");
    if (file)
    {
        String savedSSID = "";
        String savedPassword = "";
        savedSSID = file.readStringUntil('\n');
        savedPassword = file.readStringUntil('\n');
        savedSSID.toCharArray(ssid, savedSSID.length());
        savedPassword.toCharArray(password, savedPassword.length());
        file.close();
        return true;
    }
    return false;
}
#define FILE_CHUNK_SIZE 1024
void handleFileDownload(AsyncWebServerRequest *request)
{
    File file;
    size_t fileSize;
    size_t sentSize = 0;
    if (!file || !file.available())
    {
        file = SPIFFS.open("record/login.txt");
        if (!file)
        {
            request->send(404, "text/plain", "File not found");
            return;
        }
        fileSize = file.size();
        sentSize = 0;
    }
}
