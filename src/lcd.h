#include <Arduino.h>
#include "U8g2lib.h"
#include "SD.h"
#include <SPI.h>

#ifndef LCD_DRIVER
#define LCD_DRIVER U8G2_ST7567_JLX12864_F_4W_HW_SPI 
#endif

class LCD : public U8G2_ST7567_JLX12864_F_4W_HW_SPI {
public:
    void drawFile(u8g2_int_t x, u8g2_int_t y, const char *filename);
    LCD() : U8G2_ST7567_JLX12864_F_4W_HW_SPI(U8G2_R0, /* cs=*/21, /* dc=*/4, /* reset=*/0) {
        begin();
    }
};
