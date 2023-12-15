#include "include.h"
int findEmptyID(Adafruit_Fingerprint finger);
int Finger_s(Adafruit_Fingerprint finger);
void enrollFingerprint(Adafruit_Fingerprint finger,uint8_t id);
void deleteFinger(Adafruit_Fingerprint finger,uint8_t idToDelete);
void beep(long time);
