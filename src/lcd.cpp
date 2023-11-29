#include "lcd.h"
void LCD::drawFile(u8g2_int_t x, u8g2_int_t y, const char *filename)
{
    uint8_t w;
    uint8_t h;
    uint8_t b;
    uint8_t mask;
    uint8_t len;
    u8g2_int_t xpos;
    File myFile = SD.open(filename);
    if (myFile)
    {
        w = myFile.read(); // read the dimension of the bitmap
        h = myFile.read();
        while (h > 0)
        { // handle all lines of the bitmap
            xpos = x;
            len = w; // len will contain the horizontal number of pixel
            mask = 1;
            b = myFile.read(); // load the first 8 pixel into "b"

            for (;;)
            { // draw all pixel of one line
                if (b & mask)
                { // check one pixel
                    setDrawColor(1);
                    drawPixel(xpos, y);
                }
                else
                {
                    setDrawColor(0);
                    drawPixel(xpos, y);
                }
                xpos++;       // calculate next x pos of the pixel
                mask <<= 1;   // update the mask
                len--;        // decrease the horizontal width (remaining pixel)
                if (len == 0) // check if row is finished
                    break;
                if (mask == 0)
                {
                    mask = 1;          // revert mask and ...
                    b = myFile.read(); // ... load the next 8 pixel values from the file
                }
            }

            y++; // goto next line
            h--; // decrease the number of remaining lines
        }
        myFile.close(); // all done, close the file
    }
    setDrawColor(1); // restore color
}