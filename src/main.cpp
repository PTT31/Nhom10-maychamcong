#include "main.h"

message_lcd mess;
LCD u8g2;
unsigned long startTime = millis();
unsigned long delayTime = 5000; // 5 giây
void drawTime(LCD u8g2)
{
    // DateTime now;
    u8g2.setFont(u8g2_font_timB10_tr);
    u8g2.setCursor(5, 12);
    // now = rtc.getTime();
    // String dateTimeString = rtc.getTimeDate();
    u8g2.print(rtc.getTimeDate());
};
void record(User_if user)
{
    // if the file is available, write to it:
    File dataFile = SD.open("/record/Login.csv", FILE_APPEND); // Mở file để ghi thêm dữ liệu
    if (dataFile)
    {
        char buffer[50]; // Dung lượng đủ lớn để lưu trữ dữ liệu
        sprintf(buffer, "%s,%s,%d", rtc.getTimeDate(),
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
void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for native USB port only
    }
    pinMode(PinBuzz, OUTPUT);
    digitalWrite(PinBuzz, 0);
    beep(500);
    delay(500);
    SPI.begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);
    if (!SD.begin(15, SPI))
    {
        Serial.println("initialization failed!");
    }
    else{
    sqlite3_initialize();
    Serial.println("initialization done.");
    }
    Wire.setPins(27, 26);
    if (!dsrtc.begin())
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
    // Lấy thời gian từ NTP Server
    // configTime(0, 0, "pool.ntp.org");
    // rtc.updateTime();

    // Cài đặt bộ hẹn giờ để cập nhật thời gian sau mỗi 12 giờ
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 43200000000, true); // 12 giờ (12 * 60 * 60 * 1000 * 1000)
    timerAlarmEnable(timer);
}
void IRAM_ATTR onTimer()
{
    rtc.setTime();
    // printCurrentTime();
}

void loop()
{
    int finger_id = -1;
    finger_id = Finger_s(finger);
    // finger.LEDcontrol(1);
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
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
            mess.mode = Incorrect_finger;
            startTime = millis();
        }
        else
        {
            Serial.println(user.name);
            record(user);
            mess.noti = user.name;
            mess.mode = Correct_finger;
            startTime = millis();
        }
    }
    if ((mess.mode != Scan_finger && (millis() - startTime > delayTime)) && !(mess.mode == Insert_finger))
    {
        mess.noti = "";
        mess.mode = Scan_finger;
    }
    u8g2.clearDisplay();
    u8g2.firstPage();
    do
    {
        drawTime(u8g2);
        u8g2.setFont(u8g2_font_timB10_tr);
        u8g2.setCursor(0, 60); // Đặt vị trí để in tên
        u8g2.print(mess.ip);
        switch (mess.mode)
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
            u8g2.setCursor(0, 36); // Đặt vị trí để in tên
            u8g2.print(mess.noti); // In tên lên màn hình
            // u8g2.drawFile(0, 40, "/bin/Correct_finger.bin");
            break;
        case Insert_finger:
            u8g2.setCursor(0, 24); // Đặt vị trí để in tên
            u8g2.print("Finger Insert");
            u8g2.setCursor(0, 36);
            u8g2.print(mess.noti);
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

    vTaskDelay(pdMS_TO_TICKS(700)); // Đợi 1 giây trước khi lặp lại nhiệm vụ
}

void TaskInternet(void *pvParameters)
{
    char ssid[30]; // Tên mạng WiFi và mật khẩu mặc định
    char password[30];
    SPIFFS.begin();
    readWiFiCredentials(ssid, password);
    // Kết nối WiFi
    rtc.setTime(dsrtc.now().unixtime());
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
        WiFi.softAP("Mcc-Nhom10");
        mess.ip = "Mcc-Nhom10";
        rtc.setTime(dsrtc.now().unixtime());
        // Cấu trúc thời gian cho ESP32
        setupServer();
    }
    else
    {

        WiFiUDP ntpUDP;
        configTime(25200, 0, "pool.ntp.org"); // Đặt múi giờ và NTP server
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(WiFi.SSID());
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        struct tm timeinfo;
        while (!getLocalTime(&timeinfo))
        {
            Serial.println("Waiting for NTP time sync");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        rtc.setTimeStruct(timeinfo);
        dsrtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
        mess.ip = WiFi.localIP().toString();
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
