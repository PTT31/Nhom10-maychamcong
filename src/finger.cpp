#include "finger.h"
#include "lcd.h"
extern message_lcd mess;
extern unsigned long startTime;
void beep(long time)
{
    digitalWrite(PinBuzz, 1);
    vTaskDelay(pdMS_TO_TICKS(time));
    digitalWrite(PinBuzz, 0);
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
        Serial.println("Not finger dettec");
        mess.mode = Incorrect_finger;
        startTime = millis();
        beep(200);
        return -1;
    }
    // found a match!
    Serial.print("Found ID #");
    Serial.print(finger.fingerID);
    Serial.print(" with confidence of ");
    Serial.println(finger.confidence);
    beep(500);
    return finger.fingerID;
}
void deleteFinger(Adafruit_Fingerprint finger,uint8_t idToDelete)
{
    if (finger.deleteModel(idToDelete))
    {
        Serial.println("Deleted!");
    }
    else
    {
        Serial.println("Failed to delete");
    }
}
int findEmptyID(Adafruit_Fingerprint finger)
{
    for (uint8_t id = 1; id <= 162; id++)
    { // Duyệt qua các ID từ 1 đến 162
        if (finger.loadModel(id) != FINGERPRINT_OK)
        { // Kiểm tra xem ID đã được sử dụng chưa
            return id;
        }
    }
    return -1;
}
void enrollFingerprint(Adafruit_Fingerprint finger, uint8_t id) {
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK) {
        Serial.println("Lỗi khi đọc hình ảnh");
        mess.mode = Incorrect_finger;
        return;
    }
    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
        mess.mode = Incorrect_finger;
        Serial.println("Lỗi khi chuyển đổi hình ảnh");
        return;
    }
    p = finger.createModel();
    if (p != FINGERPRINT_OK) {
        mess.mode = Incorrect_finger;
        Serial.println("Lỗi khi tạo mô hình");
        return;
    }
    p = finger.storeModel(id);
    if (p != FINGERPRINT_OK) {
        Serial.println("Lỗi khi lưu mô hình vân tay");
        mess.mode = Incorrect_finger;
    }
}
