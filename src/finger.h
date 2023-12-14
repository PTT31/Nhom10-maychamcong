#include "include.h"
int findEmptyID(Adafruit_Fingerprint finger);
int Finger_s(Adafruit_Fingerprint finger);
void enrollFingerprint(Adafruit_Fingerprint finger,uint8_t id);
void deleteFinger(Adafruit_Fingerprint finger,uint8_t idToDelete);
uint8_t isIDPresent(Adafruit_Fingerprint finger,String nameValue);
void beep(long time);
