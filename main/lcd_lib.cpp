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
#include "lcdgfx.h"
#include "lcdgfx_gui.h"
#include "sova.h"
#include "lcd_lib.hpp"

// The parameters are  RST pin, BUS number, CS pin, DC pin, FREQ (0 means default), CLK pin, MOSI pin
DisplaySSD1331_96x64x8_SPI display(4,{-1, 17, 16, 0,18,23});
NanoEngine8<DisplaySSD1331_96x64x8_SPI> engine(display);

NanoPoint sprites[LCD_SPRITE_NUM_MAX];
S_LCD_CTRL_DATA lcdCtrlData[LCD_DISPLAY_MODE_MAX];
S_LCD_CTRL_DATA *lcdCtrlDataPtr;

void lcdSetup(LCD_DISPLAY_MODE mode)
{
    lcdCtrlDataPtr = &lcdCtrlData[mode%LCD_DISPLAY_MODE_MAX];

    switch( mode )
    {
    case LCD_DISPLAY_MODE2:
        display.setFixedFont(ssd1306xled_font8x16);
        lcdCtrlDataPtr->fontHeight = 16;
        break;
    case LCD_DISPLAY_MODE1:
    default:
        display.setFixedFont(ssd1306xled_font6x8);
        lcdCtrlDataPtr->fontHeight = 8;
        break;
    }

    display.begin();
    lcdCtrlDataPtr->spriteNum = display.height()/lcdCtrlDataPtr->fontHeight;

    // RGB functions do not work in default SSD1306 compatible mode
    display.fill( 0x00 );
    display.clear();

    for (int j=0; j<LCD_SPRITE_NUM_MAX; j++)
    {
        sprites[j].x = 0;
        sprites[j].y = (lcdCtrlDataPtr->fontHeight*j);
    }

    // We not need to clear screen, engine will do it for us
    engine.begin();
    // Force engine to refresh the screen
    engine.refresh();
    // Set function to draw our sprite1

    switch( mode )
    {
    case LCD_DISPLAY_MODE2:
        engine.drawCallback( []()->bool {
            engine.getCanvas().clear();
            engine.getCanvas().setColor( RGB_COLOR8(255,255,255) );
            engine.getCanvas().setFixedFont(ssd1306xled_font8x16);
            engine.getCanvas().printFixedPgm( sprites[0].x, sprites[0].y, lcdCtrlDataPtr->displayDataArray[0], STYLE_NORMAL );
            engine.getCanvas().printFixedPgm( sprites[1].x, sprites[1].y, lcdCtrlDataPtr->displayDataArray[1], STYLE_NORMAL );
            engine.getCanvas().printFixedPgm( sprites[2].x, sprites[2].y, lcdCtrlDataPtr->displayDataArray[2], STYLE_NORMAL );
            engine.getCanvas().printFixedPgm( sprites[3].x, sprites[3].y, lcdCtrlDataPtr->displayDataArray[3], STYLE_NORMAL );
            engine.getCanvas().printFixedPgm( sprites[4].x, sprites[4].y, lcdCtrlDataPtr->displayDataArray[4], STYLE_NORMAL );
            engine.getCanvas().printFixedPgm( sprites[5].x, sprites[5].y, lcdCtrlDataPtr->displayDataArray[5], STYLE_NORMAL );
            engine.getCanvas().printFixedPgm( sprites[6].x, sprites[6].y, lcdCtrlDataPtr->displayDataArray[6], STYLE_NORMAL );
            engine.getCanvas().printFixedPgm( sprites[7].x, sprites[7].y, lcdCtrlDataPtr->displayDataArray[7], STYLE_NORMAL );
            return true;
        } );
        break;
    case LCD_DISPLAY_MODE1:
    default:
        engine.drawCallback( []()->bool {
            engine.getCanvas().clear();
            engine.getCanvas().setColor( RGB_COLOR8(255, 32, 32) );
            engine.getCanvas().setFixedFont(ssd1306xled_font6x8);
            engine.getCanvas().printFixedPgm( sprites[0].x, sprites[0].y, lcdCtrlDataPtr->displayDataArray[0], STYLE_NORMAL );
            engine.getCanvas().printFixedPgm( sprites[1].x, sprites[1].y, lcdCtrlDataPtr->displayDataArray[1], STYLE_NORMAL );
            engine.getCanvas().printFixedPgm( sprites[2].x, sprites[2].y, lcdCtrlDataPtr->displayDataArray[2], STYLE_NORMAL );
            engine.getCanvas().printFixedPgm( sprites[3].x, sprites[3].y, lcdCtrlDataPtr->displayDataArray[3], STYLE_NORMAL );
            engine.getCanvas().printFixedPgm( sprites[4].x, sprites[4].y, lcdCtrlDataPtr->displayDataArray[4], STYLE_NORMAL );
            engine.getCanvas().printFixedPgm( sprites[5].x, sprites[5].y, lcdCtrlDataPtr->displayDataArray[5], STYLE_NORMAL );
            engine.getCanvas().printFixedPgm( sprites[6].x, sprites[6].y, lcdCtrlDataPtr->displayDataArray[6], STYLE_NORMAL );
            engine.getCanvas().printFixedPgm( sprites[7].x, sprites[7].y, lcdCtrlDataPtr->displayDataArray[7], STYLE_NORMAL );
            return true;
        } );
        break;
    }
}

void lcdProc()
{
    if( lcdCtrlDataPtr != nullptr )
    {
        if( lcdCtrlDataPtr->bufferExist )
        {
            strncpy(lcdCtrlDataPtr->displayDataArray[lcdCtrlDataPtr->writeIndex], lcdCtrlDataPtr->displayDataArrayBuffer, BUFFER_SIZE);
            lcdCtrlDataPtr->bufferExist = false;
            lcdCtrlDataPtr->writeIndex = (lcdCtrlDataPtr->writeIndex + 1) % lcdCtrlDataPtr->spriteNum;

            for (int j=0; j<lcdCtrlDataPtr->spriteNum; j++ )
            {
                sprites[j].y-=lcdCtrlDataPtr->fontHeight;
                if (sprites[j].y < 0)
                {
                    sprites[j].y = lcdCtrlDataPtr->fontHeight*(lcdCtrlDataPtr->spriteNum-1);
                }
                engine.refresh( sprites[j].x, sprites[j].y, sprites[j].x + display.width()-1, sprites[j].y + lcdCtrlDataPtr->fontHeight - 1 );
            }
            engine.display();
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        // do nothing
    }
}

bool PrintLCD(LCD_DISPLAY_MODE mode,char *msg) {
    if( &lcdCtrlData[mode%LCD_DISPLAY_MODE_MAX] == lcdCtrlDataPtr )
    {
        if( lcdCtrlDataPtr != nullptr )
        {
            if( lcdCtrlDataPtr->spriteCounter < lcdCtrlDataPtr->spriteNum )
            {
                strncpy(lcdCtrlDataPtr->displayDataArray[lcdCtrlDataPtr->spriteCounter], msg, BUFFER_SIZE);
                lcdCtrlDataPtr->spriteCounter++;

                for (int j=0; j<lcdCtrlDataPtr->spriteNum; j++ )
                {
                    engine.refresh( sprites[j].x, sprites[j].y, sprites[j].x + display.width()-1, sprites[j].y + lcdCtrlDataPtr->fontHeight - 1 );
                }
                engine.display();
            }
            else
            {
                if( lcdCtrlDataPtr->bufferExist )
                {
                    // do nothing
                }
                else
                {
                    strncpy(lcdCtrlDataPtr->displayDataArrayBuffer, msg, BUFFER_SIZE);
                    lcdCtrlDataPtr->bufferExist = true;
                }
            }
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        // do nothing
    }

    return true;
}

bool SetStringToLCD(LCD_DISPLAY_MODE mode,LCD_SPRITE_NUM num,char *msg) {

    if( &lcdCtrlData[mode%LCD_DISPLAY_MODE_MAX] == lcdCtrlDataPtr )
    {
        if( lcdCtrlDataPtr != nullptr )
        {
            if( num < lcdCtrlDataPtr->spriteNum )
            {
                strncpy(lcdCtrlDataPtr->displayDataArray[num], msg, BUFFER_SIZE);
                engine.refresh( sprites[num].x, sprites[num].y, sprites[num].x + display.width()-1, sprites[num].y + lcdCtrlDataPtr->fontHeight - 1 );
                engine.display();
            }
            else
            {
                // do nothing
            }
        }
        else
        {
            // do nothing
        }
    }
    else
    {
        // do nothing
    }

    return true;
}

void ClearLCD(void) {
    display.fill( 0x00 );
    display.clear();
}
