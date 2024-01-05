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
uint8_t deleteFinger(Adafruit_Fingerprint finger,uint8_t idToDelete)
{
    if (finger.deleteModel(idToDelete))
    {
        Serial.println("Deleted!");
        return 1;
    }
    else
    {
        Serial.println("Failed to delete");
        return 0;
    }
}
int findEmptyID() {
    File myFile = SD.open("fingerid.txt"); // Khai báo và mở file
    if (myFile) {
        String numberString = myFile.readStringUntil('\n'); // Đọc chuỗi từ file
        myFile.close(); // Đóng file sau khi đọc xong
        // Chuyển chuỗi sang số nguyên
        int number = numberString.toInt();
        return number;
    } else {
        Serial.println("Không thể mở file.");
    }
    return -1;
}
void deleteNumberInFile(uint8_t numberToDelete) {
    File myFile = SD.open("fingerid.txt", FILE_WRITE); // Mở file để ghi dữ liệu (append)

    if (myFile) {
        myFile.seek(0); // Di chuyển con trỏ về đầu file

        while (myFile.available()) {
            String numberString = myFile.readStringUntil('\n');
            uint8_t number = numberString.toInt();

            if (number != numberToDelete) {
                myFile.println(number);
            }
        }
        myFile.close(); // Đóng file sau khi hoàn thành
    } else {
        Serial.println("Không thể mở file.");
    }
}
void addNumberInFile(uint8_t numberToAdd) {
    File myFile = SD.open("fingerid.txt", FILE_WRITE);

    if (myFile) {
        myFile.seek(myFile.size()); // Di chuyển con trỏ đến cuối file

        // Ghi số mới vào file
        myFile.println(numberToAdd);

        myFile.close(); // Đóng file sau khi ghi
    } else {
        Serial.println("Không thể mở file.");
    }
}
uint8_t enrollFingerprint(Adafruit_Fingerprint finger, uint8_t id) {
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK) {
        Serial.println("Lỗi khi đọc hình ảnh");
        mess.mode = Incorrect_finger;
        return -1;
    }
    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
        mess.mode = Incorrect_finger;
        Serial.println("Lỗi khi chuyển đổi hình ảnh");
        return -1;
    }
    p = finger.createModel();
    if (p != FINGERPRINT_OK) {
        mess.mode = Incorrect_finger;
        Serial.println("Lỗi khi tạo mô hình");
        return -1;
    }
    p = finger.storeModel(id);
    if (p != FINGERPRINT_OK) {
        Serial.println("Lỗi khi lưu mô hình vân tay");
        mess.mode = Incorrect_finger;
        return -1;
    }
    return 1;
}
