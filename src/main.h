#include "include.h"
#include "lcd.h"
#include "Database.h"
#include "Web.h"
#include "finger.h"
#define mySerial Serial2 // Sử dụng Serial2 cho cảm biến vân tay

#define HSPI_MISO 19
#define HSPI_MOSI 5
#define HSPI_SCLK 18
#define HSPI_SS 15

#define PinBuzz 12
hw_timer_t * timer = NULL;
volatile bool timerFlag = false;
ESP32Time rtc; // Tạo đối tượng RTC
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
RTC_DS1307 dsrtc;                     // Khai báo đối tượng RTC
QueueHandle_t QueueHandle;          // Hàng đợi để truyền ID từ TaskFinger đến TaskSQL
const uint8_t QueueElementSize = 5; // Số lượng phần tử tối đa trong hàng đợi
TaskHandle_t taskitn;               // Handle của TaskInternet
void TaskInternet(void *pvParameters); // Hàm của TaskInternet
// In Thoi Gian
void drawTime(LCD u8g2); // Hàm để vẽ thời gian lên màn hình
// Ve File
void record(User_if user); // Hàm để ghi dữ liệu vào cơ sở dữ liệu

void IRAM_ATTR onTimer();