#include "main.h"

message_lcd mess;
LCD u8g2;
unsigned long startTime = millis();
unsigned long delayTime = 5000; // 5 giây
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
    // if (millis() - startTime_f > delayTime_f)
    // {
    //     
    // }
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
            finger_id = Finger_s(finger);
            break;
        case Correct_finger:
            u8g2.setCursor(0, 24); // Đặt vị trí để in tên
            u8g2.print("Welcome to Haui");
            u8g2.setCursor(0, 36);    // Đặt vị trí để in tên
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
    if (mess.mode != Scan_finger && (millis() - startTime > delayTime))
    {
        mess.noti = "";
        mess.mode = Scan_finger;
    }
    vTaskDelay(pdMS_TO_TICKS(500)); // Đợi 1 giây trước khi lặp lại nhiệm vụ
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
        WiFi.softAP("Mcc-Nhom10");
        mess.ip = "Mcc-Nhom10";
        setupServer(finger);
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
        mess.ip = WiFi.localIP().toString();
        setupServer(finger);
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





