/*
    MIT License

    Copyright (c) 2018-2020, Alexey Dynda

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
/**
 *   Nano/Atmega328 PINS: connect LCD to D5 (D/C), D4 (CS), D3 (RES), D11(DIN), D13(CLK)
 *   Attiny SPI PINS:     connect LCD to D4 (D/C), GND (CS), D3 (RES), D1(DIN), D2(CLK)
 *   ESP8266: connect LCD to D1(D/C), D2(CS), RX(RES), D7(DIN), D5(CLK)
 */

/* !!! THIS DEMO RUNS in FULL COLOR MODE */

#include "lcdgfx.h"
#include "lcdgfx_gui.h"
#include "sova.h"

// The parameters are  RST pin, BUS number, CS pin, DC pin, FREQ (0 means default), CLK pin, MOSI pin
//DisplaySSD1331_96x64x8_SPI display(3,{-1, 4, 5, 0,-1,-1}); // Use this line for Atmega328p
DisplaySSD1331_96x64x8_SPI display(4,{-1, 17, 16, 0,18,23});
//DisplaySSD1331_96x64x8_SPI display(4,{-1, -1, 5, 0,-1,-1});  // Use this line for ESP8266 Arduino style rst=4, CS=-1, DC=5
                                                             // And ESP8266 RTOS IDF. GPIO4 is D2, GPIO5 is D1 on NodeMCU boards
NanoPoint sprite;
NanoEngine8<DisplaySSD1331_96x64x8_SPI> engine(display);

#define FONT_HEIGHT 8
#define SPRITE_NUM 8
#define BUFFER_SIZE 32
NanoPoint sprites[SPRITE_NUM];
int writeIndex = 0;
int readIndex = 0;
int spriteCounter = 0;
char displayDataArray[SPRITE_NUM][BUFFER_SIZE];
char displayDataArrayBuffer[BUFFER_SIZE];
bool bufferExist = false;

extern "C" void lcdSetup()
{
    display.setFixedFont(ssd1306xled_font6x8);
    display.begin();

    // RGB functions do not work in default SSD1306 compatible mode
    display.fill( 0x00 );
    display.clear();

    for (int j=0; j<SPRITE_NUM; j++)
    {
      sprites[j].x = 0;
      sprites[j].y = (FONT_HEIGHT*j);
    }

    // We not need to clear screen, engine will do it for us
    engine.begin();
    // Force engine to refresh the screen
    engine.refresh();
    // Set function to draw our sprite1
    engine.drawCallback( []()->bool {
        engine.getCanvas().clear();
        engine.getCanvas().setColor( RGB_COLOR8(255, 32, 32) );
        engine.getCanvas().setFixedFont(ssd1306xled_font6x8);
        engine.getCanvas().printFixedPgm( sprites[0].x, sprites[0].y, displayDataArray[0], STYLE_NORMAL );
        engine.getCanvas().printFixedPgm( sprites[1].x, sprites[1].y, displayDataArray[1], STYLE_NORMAL );
        engine.getCanvas().printFixedPgm( sprites[2].x, sprites[2].y, displayDataArray[2], STYLE_NORMAL );
        engine.getCanvas().printFixedPgm( sprites[3].x, sprites[3].y, displayDataArray[3], STYLE_NORMAL );
        engine.getCanvas().printFixedPgm( sprites[4].x, sprites[4].y, displayDataArray[4], STYLE_NORMAL );
        engine.getCanvas().printFixedPgm( sprites[5].x, sprites[5].y, displayDataArray[5], STYLE_NORMAL );
        engine.getCanvas().printFixedPgm( sprites[6].x, sprites[6].y, displayDataArray[6], STYLE_NORMAL );
        engine.getCanvas().printFixedPgm( sprites[7].x, sprites[7].y, displayDataArray[7], STYLE_NORMAL );
        return true;
    } );
}

bool readFromDataArray(void) {

    readIndex = (readIndex + 1) % SPRITE_NUM;

    return true;
}

extern "C" void lcdProc()
{
  if( bufferExist )
  {
      strncpy(displayDataArray[writeIndex], displayDataArrayBuffer, BUFFER_SIZE);
      bufferExist = false;
      writeIndex = (writeIndex + 1) % SPRITE_NUM;
//      readFromDataArray();

      for (int j=0; j<SPRITE_NUM; j++ )
      {
          sprites[j].y-=FONT_HEIGHT;
          if (sprites[j].y < 0)
          {
              sprites[j].y = FONT_HEIGHT*7;
          }
          engine.refresh( sprites[j].x, sprites[j].y, sprites[j].x + 100 - 1, sprites[j].y + 8 - 1 );
      }
      engine.display();
  }
  else
  {
      // do nothing
  }
}

extern "C" bool PrintLCD(char *msg) {
    if (strlen(msg) >= BUFFER_SIZE) {
        return false;
    }

    if( spriteCounter < SPRITE_NUM )
    {
      strncpy(displayDataArray[spriteCounter], msg, BUFFER_SIZE);
      spriteCounter++;

      for (int j=0; j<SPRITE_NUM; j++ )
      {
          engine.refresh( sprites[j].x, sprites[j].y, sprites[j].x + 100 - 1, sprites[j].y + 8 - 1 );
      }
      engine.display();
    }
    else
    {
      if( bufferExist )
      {
        // do nothing
      }
      else
      {
        strncpy(displayDataArrayBuffer, msg, BUFFER_SIZE);
        bufferExist = true;
      }
    }

    return true;
}
