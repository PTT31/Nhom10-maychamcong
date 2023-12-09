#include "sd_.h" // Thư viện để làm việc với thẻ SD
#include "lcd.h"
#include "RTClib.h"               // Thư viện để làm việc với module RTC DS1307
#include <Adafruit_Fingerprint.h> // Thư viện để làm việc với cảm biến vân tay
#include <WiFi.h>                 // Thư viện WiFi
#include <NTPClient.h>            // Thư viện NTPClient cho đồng bộ thời gian
#include <DNSServer.h>            // Thư viện DNSServer để xử lý DNS trên AP
#include <AsyncTCP.h>             // Thư viện AsyncTCP cho ESPAsyncWebServer
#include "ESPAsyncWebServer.h"    // Thư viện ESPAsyncWebServer
#include <SPIFFS.h>
#include "FS.h"
#define mySerial Serial2     // Sử dụng Serial2 cho cảm biến vân tay

#define HSPI_MISO 19
#define HSPI_MOSI 5
#define HSPI_SCLK 18
#define HSPI_SS 15
SPIClass secondSPI(HSPI);
RTC_DS1307 rtc;                     // Khai báo đối tượng RTC
SemaphoreHandle_t spiMutex;         // Semaphore để quản lý truy cập SPI
QueueHandle_t QueueHandle;          // Hàng đợi để truyền ID từ TaskFinger đến TaskSQL
const uint8_t QueueElementSize = 5; // Số lượng phần tử tối đa trong hàng đợi
TaskHandle_t taskitn;               // Handle của TaskInternet
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

enum FingerMode
{
    Scan_finger,
    Insert_finger,
    Correct_finger,
    Incorrect_finger
};

class message_lcd
{
public:
    int mode = Scan_finger;
    String ip = "Conect Wifi...";
    String noti;
};
unsigned long startTime = millis();
unsigned long delayTime = 5000; // 5 giây
unsigned long startTime_f = millis();
unsigned long delayTime_f = 5000; // 5 giây
LCD u8g2;
message_lcd message;                   // Đối tượng để chứa thông điệp cho TaskLCD
DateTime TaskTime();                   // Hàm để lấy thời gian từ module DS1307
void TaskInternet(void *pvParameters); // Hàm của TaskInternet
int Finger_s();                        // Hàm để đọc ID từ cảm biến vân tay
// In Thoi Gian
void drawTime(LCD u8g2); // Hàm để vẽ thời gian lên màn hình
// Ve File
void record(User_if user); // Hàm để ghi dữ liệu vào cơ sở dữ liệu
int Finger_s(LCD finger);  // Hàm để đọc ID từ cảm biến vân tay'
void beep(long time);      // Hàm còi beep
void enrollFingerprint(uint8_t id);
uint8_t findEmptyID();
void
void drawTime(LCD u8g2)
{
    DateTime now;
    u8g2.setFont(u8g2_font_timB10_tr);
    u8g2.setCursor(5, 12);
    now = rtc.now();
    String dateTimeString = String(now.month(), DEC) + '/' +
                            String(now.day(), DEC) + '/' +
                            String(now.year(), DEC) + ' ' +
                            String(now.hour(), DEC) + ':' +
                            String(now.minute(), DEC) + ':' +
                            String(now.second(), DEC);

    u8g2.print(dateTimeString);
};
void record(User_if user)
{
    // if the file is available, write to it:
    File dataFile = SD.open("/record/Login.csv", FILE_APPEND); // Mở file để ghi thêm dữ liệu
    if (dataFile)
    {
        DateTime now = rtc.now();
        char buffer[50]; // Dung lượng đủ lớn để lưu trữ dữ liệu
        sprintf(buffer, " %d/%d/%d,%d:%d:%d,%s,%d",
                now.month(), now.day(), now.year(),
                now.hour(), now.minute(), now.second(),
                user.name, user.finger_id);
        dataFile.println(buffer);
        dataFile.close();
        if (QueueHandle != NULL)
        { // Sanity check just to make sure the queue actually exists
            int ret = xQueueSend(QueueHandle, (void *)&buffer, 0);
            if (ret == errQUEUE_FULL)
            {
                Serial.println("The `TaskReadFromSerial` was unable to send data into the Queue");
            }
        }
        Serial.println(buffer);
    }
    else
    {
        Serial.println("Error opening file for append.");
    }
}
int Finger_s(Adafruit_Fingerprint finger)
{
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK)
        return -1;

    p = finger.image2Tz();
    if (p != FINGERPRINT_OK)
        return -1;

    p = finger.fingerSearch();
    if (p != FINGERPRINT_OK)
    {
        message.mode = Incorrect_finger;
        startTime = millis();
        Serial.println("Not finger dettec");
        return -1;
    }
    // found a match!
    Serial.print("Found ID #");
    Serial.print(finger.fingerID);
    Serial.print(" with confidence of ");
    Serial.println(finger.confidence);
    beep(500);
    startTime_f = millis();
    return finger.fingerID;
}
void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for native USB port only
    }
    pinMode(14, OUTPUT);
    digitalWrite(14, 0);
    beep(500);
    delay(500);
    SPI.begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);
    if (!SD.begin(15, SPI))
    {
        Serial.println("initialization failed!");
        while (1)
            ;
    }
    sqlite3_initialize();
    Serial.println("initialization done.");
    Wire.setPins(27, 26);
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        Serial.flush();
    }
    finger.begin(57600);
    if (finger.verifyPassword())
    {
        Serial.println("Found fingerprint sensor!");
    }
    else
    {
        Serial.println("Did not find fingerprint sensor :(");
        // while (1)
        //     ;
    }

    u8g2.begin();
    u8g2.setContrast(25);
    spiMutex = xSemaphoreCreateBinary();
    xSemaphoreGive(spiMutex);
    Serial.print("Semaphore Start");
    QueueHandle = xQueueCreate(QueueElementSize, sizeof(char[50]));
    //  Check if the queue was successfully created
    if (QueueHandle == NULL)
    {
        Serial.println("Queue could not be created. Halt.");
        while (1)
            delay(1000);
    }
    Serial.print("QueueStart");
    xTaskCreatePinnedToCore(TaskInternet, "TaskInternet", 4000, NULL, 1, &taskitn, 0);
}

void loop()
{
    int finger_id = -1;
    if (millis() - startTime_f > delayTime_f)
    {
        finger_id = Finger_s(finger);
    }
    xSemaphoreTake(spiMutex, portMAX_DELAY);
    u8g2.clearDisplay();
    u8g2.firstPage();
    do
    {
        drawTime(u8g2);
        u8g2.setFont(u8g2_font_timB10_tr);
        u8g2.setCursor(0, 60); // Đặt vị trí để in tên
        u8g2.print(message.ip);
        switch (message.mode)
        {
        case Scan_finger:
            u8g2.setCursor(0, 24); // Đặt vị trí để in tên
            u8g2.print("Welcome to Haui");
            u8g2.setCursor(0, 36);           // Đặt vị trí để in tên
            u8g2.print("Please put finger"); // In tên lên màn hình
            u8g2.setCursor(0, 48);
            u8g2.print("on sensor");

            break;
        case Correct_finger:
            u8g2.setCursor(0, 24); // Đặt vị trí để in tên
            u8g2.print("Welcome to Haui");
            u8g2.setCursor(0, 36);    // Đặt vị trí để in tên
            u8g2.print(message.noti); // In tên lên màn hình
            // u8g2.drawFile(0, 40, "/bin/Correct_finger.bin");
            break;
        case Insert_finger:
            // u8g2.drawFile(0, 40, "/bin/Insert_finger.bin");
            break;
        case Incorrect_finger:
            u8g2.setCursor(0, 24); // Đặt vị trí để in tên
            u8g2.print("Scan again");
            break;
        default:
            break;
        }
    } while (u8g2.nextPage());
    // int ret = xQueueReceive(QueueHandle, &finger_id, 0);
    if (finger_id != -1)
    {
        Serial.println(xPortGetFreeHeapSize());
        Serial.println(esp_get_minimum_free_heap_size());
        User_if user; // Đối tượng để chứa Thông tin ngươi quét vân tay
        Serial.printf("ID find %d:\"\n", finger_id);
        int r = db_query(finger_id, &user);
        Serial.println(r);
        if (r != 1)
        {
            message.mode = Incorrect_finger;
            startTime = millis();
        }
        else
        {
            Serial.println("sadasd");
            Serial.println(user.name);
            record(user);
            message.noti = user.name;
            message.mode = Correct_finger;
            startTime = millis();
        }
    }
    xSemaphoreGive(spiMutex);
    if (message.mode != Scan_finger && (millis() - startTime > delayTime))
    {
        message.noti = "";
        message.mode = Scan_finger;
    }
    vTaskDelay(pdMS_TO_TICKS(500)); // Đợi 1 giây trước khi lặp lại nhiệm vụ
}
void beep(long time)
{
    digitalWrite(14, 1);
    vTaskDelay(pdMS_TO_TICKS(time));
    digitalWrite(14, 0);
}
AsyncWebServer server(80);
void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
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
String processor(const String &var)
{
    if (var == "HELLO_FROM_TEMPLATE")
        Serial.println(var);
    return F("Hello world!");
    return String();
}
void setupServer()
{
    server.serveStatic("/", SD, "/Web/").setTemplateProcessor(processor);
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
    server.begin();
}
void TaskInternet(void *pvParameters)
{
    char ssid[30]; // Tên mạng WiFi và mật khẩu mặc định
    char password[30];
    SPIFFS.begin();
    readWiFiCredentials(ssid, password);
    // Kết nối WiFi
    WiFi.begin(ssid, password);
    for (int count; count < 20 && WiFi.status() != WL_CONNECTED; count++)
    {
        Serial.print(".");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(WiFi.getHostname());
        Serial.print("Wifi STA");
        WiFi.softAP("esp-captive");
        message.ip = "esp-captive";
        setupServer();
    }
    else
    {

        WiFiUDP ntpUDP;
        NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600);
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(WiFi.SSID());
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        timeClient.begin();
        timeClient.update();
        rtc.adjust(DateTime(timeClient.getEpochTime()));
        message.ip = WiFi.localIP().toString();
        setupServer();
    }
    while (1)
    {
        char rc[50];
        int ret = xQueueReceive(QueueHandle, &rc, portMAX_DELAY);
        if (ret == pdPASS)
        {
            Serial.print("Internet: ");
            Serial.println(rc);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void checkAddID(AsyncWebServerRequest *request) {
    if (request->hasParam("name") && request->hasParam("position")) {
        AsyncWebParameter* name = request->getParam("name");
        AsyncWebParameter* position = request->getParam("position");

        uint8_t emptyID = findEmptyID(); // Tìm ID trống
        if (emptyID != 0xFF) { // Kiểm tra xem có ID trống không
            enrollFingerprint(emptyID); // Nạp vân tay vào ID trống
            request->send(200, "text/plain", "Fingerprint loaded successfully");
        } else {
            request->send(404, "text/plain", "Full sensor memory. Fingerprints cannot be added anymore");
        }
    }
}

uint8_t findEmptyID() {
    for (uint8_t id = 1; id <= 162; id++) { // Duyệt qua các ID từ 1 đến 162
        if (finger.loadModel(id) != FINGERPRINT_OK) { // Kiểm tra xem ID đã được sử dụng chưa
            return id;
        }
    }
    return 0xFF; // Trả về 0xFF nếu không tìm thấy ID trống
}
void enrollFingerprint(uint8_t id){
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK) {
        Serial.println("Lỗi khi đọc hình ảnh");
        return;
    }
    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
        Serial.println("Lỗi khi chuyển đổi hình ảnh");
        return;
    }
    p = finger.createModel();
    if (p != FINGERPRINT_OK) {
        Serial.println("Lỗi khi tạo mô hình");
        return;
    }
    p = finger.storeModel(id);
    if (p != FINGERPRINT_OK) {
        Serial.println("Lỗi khi lưu trữ mô hình");
        return;
    }
}
void handleCheckDelID(AsyncWebServerRequest *request) {
  String nameValue;

  if (request->hasParam("from")) {
    nameValue = request->hasParam("from");
    uint8_t id = isIDPresent(nameValue);
    // Kiểm tra xem ID có tồn tại hay không
    if (id != NULL) {
      // Xóa dữ liệu từ cảm biến vân tay
      int idToDelete = id;
      deleteFinger(idToDelete);
      
      // Xóa dữ liệu khỏi cơ sở dữ liệu của bạn ở đây
      bool success = deleteData(nameValue); // Ví dụ: hàm xóa dữ liệu với số nhận được

      if (success) {
        request->send(200, "text/plain", "Data with number " + nameValue + " deleted successfully");
      } else {
        request->send(500, "text/plain", "Failed to delete data");
      }
    } else {
      // Nếu ID không tồn tại, gửi thông báo tương ứng
      request->send(200, "text/plain", "ID not found");
    }
  } else {
    request->send(400, "text/plain", "No 'id' value provided");
  }
}
bool isIDPresent(String nameValue){

}