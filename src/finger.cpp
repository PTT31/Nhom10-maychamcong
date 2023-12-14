#include "finger.h"
#include "lcd.h"
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
        return -1;
        beep(200);
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
void enrollFingerprint(Adafruit_Fingerprint finger,uint8_t id)
{
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK)
    {
        Serial.println("Lỗi khi đọc hình ảnh");
        // message.mode = Incorrect_finger;
        return;
    }
    p = finger.image2Tz();
    if (p != FINGERPRINT_OK)
    {
        // message.mode = Incorrect_finger;
        Serial.println("Lỗi khi chuyển đổi hình ảnh");
        return;
    }
    p = finger.createModel();
    if (p != FINGERPRINT_OK)
    {
        // message.mode = Incorrect_finger;
        Serial.println("Lỗi khi tạo mô hình");
        return;
    }
    p = finger.storeModel(id);
}

uint8_t isIDPresent(Adafruit_Fingerprint finger,String nameValue)
{
    return 0;
}
