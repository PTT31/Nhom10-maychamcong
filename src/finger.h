#include "include.h"
int findEmptyID();
int Finger_s(Adafruit_Fingerprint finger);
uint8_t enrollFingerprint(Adafruit_Fingerprint finger,uint8_t id);
uint8_t deleteFinger(Adafruit_Fingerprint finger,uint8_t idToDelete);
void beep(long time);
void deleteNumberInFile(uint8_t numberToDelete);
void addNumberInFile(uint8_t numberToAdd);