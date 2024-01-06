#include "finger.h"
#include "lcd.h"
extern AsyncEventSource events;
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
uint8_t deleteFinger(Adafruit_Fingerprint finger, uint8_t idToDelete)
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
int findEmptyID()
{
  File myFile = SD.open("/record/finger.txt"); // Khai báo và mở file
  if (myFile)
  {
    String numberString = myFile.readStringUntil('\n'); // Đọc chuỗi từ file
    myFile.close();                                     // Đóng file sau khi đọc xong
    // Chuyển chuỗi sang số nguyên
    int number = numberString.toInt();
    return number;
  }
  else
  {
    Serial.println("Không thể mở file.");
  }
  return 128;
}
void deleteNumberInFile(uint8_t numberToRemove)
{
  String filePath = "/record/finger.txt";
  File originalFile = SD.open(filePath, FILE_READ);
  if (!originalFile)
  {
    Serial.println("Failed to open the original file for reading");
    return;
  }

  // Tạo tên file tạm thời
  String tempFilePath = String(filePath) + ".tmp";
  File tempFile = SD.open(tempFilePath.c_str(), FILE_WRITE);
  if (!tempFile)
  {
    Serial.println("Failed to open the temp file for writing");
    originalFile.close();
    return;
  }

  while (originalFile.available())
  {
    String line = originalFile.readStringUntil('\n');
    line.trim(); // Xóa các khoảng trắng
    if (line.toInt() != numberToRemove)
    {
      tempFile.println(line); // Ghi nếu không phải số cần xóa
    }
  }

  // Đóng cả hai file
  originalFile.close();
  tempFile.close();

  // Xóa file gốc và đổi tên file tạm thành file gốc
  SD.remove(filePath);
  SD.rename(tempFilePath.c_str(), filePath);
}
void addNumberInFile(uint8_t numberToAdd)
{
  File myFile = SD.open("/record/finger.txt", FILE_APPEND);

  if (myFile)
  {
    // myFile.seek(myFile.size()); // Di chuyển con trỏ đến cuối file

    // Ghi số mới vào file
    myFile.println(numberToAdd);

    myFile.close(); // Đóng file sau khi ghi
  }
  else
  {
    Serial.println("Không thể mở file.");
  }
}
int enrollFingerprint(Adafruit_Fingerprint finger, uint8_t id)
{
  events.send("Put inger on sensor!", "new_fingerprint", millis());
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  events.send("Remove finger", "new_fingerprint", millis());
  delay(2000);
  events.send("Put inger on sensor again!", "new_fingerprint", millis());
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  String send = "Add finger Id: ";
  send += id;
  events.send(send.c_str(), "new_fingerprint", millis());
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}

// {

//   int p = -1;
//   mess.mode = Insert_finger;
//   Serial.print("Waiting for valid finger to enroll as #");
//   Serial.println(id);
//   events.send("Put Finger On Sensor!", "new_fingerprint", millis());
//    while (p != FINGERPRINT_OK) {
//     p = finger.getImage();
//     switch (p) {
//     case FINGERPRINT_OK:
//       Serial.println("Image taken");
//       break;
//     case FINGERPRINT_NOFINGER:
//       Serial.print(".");
//       vTaskDelay(pdMS_TO_TICKS(300));
//       break;
//     case FINGERPRINT_PACKETRECIEVEERR:
//       Serial.println("Communication error");
//       break;
//     case FINGERPRINT_IMAGEFAIL:
//       Serial.println("Imaging error");
//       break;
//     default:
//       Serial.println("Unknown error");
//       break;
//     }
//   }

//   // OK success!

//   // p = finger.image2Tz(1);
//   if (finger.image2Tz(1)) {
//       Serial.println("Image converted");
//   }
//   else 
//     return 0;

//   Serial.println("Remove finger");
//   events.send("Remove finger!", "new_fingerprint", millis());
//   vTaskDelay(pdMS_TO_TICKS(3000));
//   p = 0;
//   while (p != FINGERPRINT_NOFINGER)
//   {
//     p = finger.getImage();
//   }
//   Serial.print("ID ");
//   Serial.println(id);
//   p = -1;
//   Serial.println("Place same finger again");
//   events.send("Place same finger again!", "new_fingerprint", millis());
//   while (p != FINGERPRINT_OK)
//   {
//     p = finger.getImage();
//     switch (p)
//     {
//     case FINGERPRINT_OK:
//       Serial.println("Image taken");
//       break;
//     case FINGERPRINT_NOFINGER:
//       Serial.print(".");
//         vTaskDelay(pdMS_TO_TICKS(300));
//       break;
//     case FINGERPRINT_PACKETRECIEVEERR:
//       Serial.println("Communication error");
//       break;
//     case FINGERPRINT_IMAGEFAIL:
//       Serial.println("Imaging error");
//       break;
//     default:
//       Serial.println("Unknown error");
//       break;
//     }
//   }

//   // OK success!
//   if (finger.image2Tz(2)) {
//       Serial.println("Image converted");
//   }
//   else 
//     return 0;
//   // OK converted!
//   Serial.print("Creating model for #");
//   Serial.println(id);

//   p = finger.createModel();
//   if (p == FINGERPRINT_OK)
//   {
//     Serial.println("Prints matched!");
//   }
//   else if (p == FINGERPRINT_PACKETRECIEVEERR)
//   {
//     Serial.println("Communication error");
//     return p;
//   }
//   else if (p == FINGERPRINT_ENROLLMISMATCH)
//   {
//     Serial.println("Fingerprints did not match");
//     return p;
//   }
//   else
//   {
//     Serial.println("Unknown error");
//     return p;
//   }

//   Serial.print("ID ");
//   Serial.println(id);
//   p = finger.storeModel(id);
//   if (p == FINGERPRINT_OK)
//   {
//     Serial.println("Stored!");
//   }
//   else if (p == FINGERPRINT_PACKETRECIEVEERR)
//   {
//     Serial.println("Communication error");
//     return p;
//   }
//   else if (p == FINGERPRINT_BADLOCATION)
//   {
//     Serial.println("Could not store in that location");
//     return p;
//   }
//   else if (p == FINGERPRINT_FLASHERR)
//   {
//     Serial.println("Error writing to flash");
//     return p;
//   }
//   else
//   {
//     Serial.println("Unknown error");
//     return p;
//   }
//   events.send("Fingerprint stored successfully!", "new_fingerprint", millis());
//   return -2;
// }
