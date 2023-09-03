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
NanoEngine8<DisplaySSD1331_96x64x8_SPI> engine1(display);
NanoEngine8<DisplaySSD1331_96x64x8_SPI> engine2(display);

#define SPRITE1_NUM_MAX 2
#define SPRITE2_NUM_MAX 3

#define SPRITE1_FONT_HEIGHT 8
#define SPRITE2_FONT_HEIGHT 16

NanoPoint sprites1[LCD_SPRITE_NUM_MAX];
NanoPoint sprites2[LCD_SPRITE_NUM_MAX];
S_LCD_CTRL_DATA lcdCtrlData[LCD_DISPLAY_MODE_MAX];

NanoPoint *getSpritesPtr(LCD_DISPLAY_MODE mode)
{
    NanoPoint *ptr;

    if( mode == LCD_DISPLAY_MODE2 )
    {
        ptr = (NanoPoint *)&sprites2;
    }
    else
    {
        ptr = (NanoPoint *)&sprites1;
    }

    return ptr;
}

void lcdCommonSetup(void)
{
    display.begin();
    display.fill( 0x00 );
    display.clear();
}

void lcdSetup(LCD_DISPLAY_MODE mode)
{
    S_LCD_CTRL_DATA *lcdCtrlDataPtr = &lcdCtrlData[mode%LCD_DISPLAY_MODE_MAX];

    switch( mode )
    {
    case LCD_DISPLAY_MODE2:
        lcdCtrlDataPtr->fontHeight = SPRITE2_FONT_HEIGHT;
        lcdCtrlDataPtr->spriteNum = SPRITE2_NUM_MAX;
        lcdCtrlDataPtr->posYmin = 0;
        lcdCtrlDataPtr->posYmax = lcdCtrlDataPtr->fontHeight*lcdCtrlDataPtr->spriteNum;

        for (int j=0; j<lcdCtrlDataPtr->spriteNum; j++)
        {
            sprites2[j].x = 0;
            sprites2[j].y = (lcdCtrlDataPtr->fontHeight*j);
        }
        engine2.begin();
        engine2.refresh();

        engine2.drawCallback( []()->bool {
            engine2.getCanvas().clear();
            engine2.getCanvas().setColor( RGB_COLOR8(255,255,255) );
            engine2.getCanvas().setFixedFont(ssd1306xled_font8x16);
            engine2.getCanvas().printFixedPgm( sprites2[0].x, sprites2[0].y, lcdCtrlData[LCD_DISPLAY_MODE2].displayDataArray[0], STYLE_NORMAL );
            engine2.getCanvas().printFixedPgm( sprites2[1].x, sprites2[1].y, lcdCtrlData[LCD_DISPLAY_MODE2].displayDataArray[1], STYLE_NORMAL );
            engine2.getCanvas().printFixedPgm( sprites2[2].x, sprites2[2].y, lcdCtrlData[LCD_DISPLAY_MODE2].displayDataArray[2], STYLE_NORMAL );
            return true;
        } );
        break;
    case LCD_DISPLAY_MODE1:
    default:
        lcdCtrlDataPtr->fontHeight = SPRITE1_FONT_HEIGHT;
        lcdCtrlDataPtr->spriteNum = SPRITE1_NUM_MAX;
        lcdCtrlDataPtr->posYmin = SPRITE2_FONT_HEIGHT*SPRITE2_NUM_MAX;
        lcdCtrlDataPtr->posYmax = display.height();

        for (int j=0; j<lcdCtrlDataPtr->spriteNum; j++)
        {
            sprites1[j].x = 0;
            sprites1[j].y = (SPRITE2_FONT_HEIGHT*SPRITE2_NUM_MAX) + (lcdCtrlDataPtr->fontHeight*j);
        }
        engine1.begin();
        engine1.refresh();

        engine1.drawCallback( []()->bool {
            engine1.getCanvas().clear();
            engine1.getCanvas().setColor( RGB_COLOR8(255, 32, 32) );
            engine1.getCanvas().setFixedFont(ssd1306xled_font6x8);
            engine1.getCanvas().printFixedPgm( sprites1[0].x, sprites1[0].y, lcdCtrlData[LCD_DISPLAY_MODE1].displayDataArray[0], STYLE_NORMAL );
            engine1.getCanvas().printFixedPgm( sprites1[1].x, sprites1[1].y, lcdCtrlData[LCD_DISPLAY_MODE1].displayDataArray[1], STYLE_NORMAL );
            return true;
        } );
        break;
    }
}

void lcdProc()
{
    for( int i=0; i<LCD_DISPLAY_MODE_MAX; i++ )
    {
        S_LCD_CTRL_DATA *lcdCtrlDataPtr = &lcdCtrlData[i];
        NanoPoint *spritesPtr = getSpritesPtr((LCD_DISPLAY_MODE)i);

        if( lcdCtrlDataPtr->bufferExist )
        {
            strncpy(lcdCtrlDataPtr->displayDataArray[lcdCtrlDataPtr->writeIndex], lcdCtrlDataPtr->displayDataArrayBuffer, BUFFER_SIZE);
            lcdCtrlDataPtr->writeIndex = (lcdCtrlDataPtr->writeIndex + 1) % lcdCtrlDataPtr->spriteNum;

            for (int j=0; j<lcdCtrlDataPtr->spriteNum; j++ )
            {
                spritesPtr->y-=lcdCtrlDataPtr->fontHeight;
                if (spritesPtr->y < lcdCtrlDataPtr->posYmin)
                {
                    spritesPtr->y = lcdCtrlDataPtr->posYmax - lcdCtrlDataPtr->fontHeight;
                }
                if( i==LCD_DISPLAY_MODE2 )
                {
                    engine2.refresh( spritesPtr->x, spritesPtr->y, spritesPtr->x + display.width()-1, spritesPtr->y + lcdCtrlDataPtr->fontHeight - 1 );
                }
                else
                {
                    engine1.refresh( spritesPtr->x, spritesPtr->y, spritesPtr->x + display.width()-1, spritesPtr->y + lcdCtrlDataPtr->fontHeight - 1 );
                }
            }
#if 0
            if( i==LCD_DISPLAY_MODE2 )
            {
                engine2.refresh( spritesPtr->x, lcdCtrlDataPtr->posYmin, spritesPtr->x + display.width()-1, lcdCtrlDataPtr->posYmax-1 );
            }
            else
            {
                engine1.refresh( spritesPtr->x, lcdCtrlDataPtr->posYmin, spritesPtr->x + display.width()-1, lcdCtrlDataPtr->posYmax-1 );
            }
#endif

            if( i==LCD_DISPLAY_MODE2 )
            {
                engine2.display();
            }
            else
            {
                engine1.display();
            }

            lcdCtrlDataPtr->bufferExist = false;
        }
        else
        {
            // do nothing
        }
    }
}

bool PrintLCD(LCD_DISPLAY_MODE mode,char *msg) {
    S_LCD_CTRL_DATA *lcdCtrlDataPtr = &lcdCtrlData[mode%LCD_DISPLAY_MODE_MAX];
    NanoPoint *spritesPtr = getSpritesPtr(mode);

    if( lcdCtrlDataPtr->spriteCounter < lcdCtrlDataPtr->spriteNum )
    {
        lcdCtrlDataPtr->spriteCounter++;
        strncpy(lcdCtrlDataPtr->displayDataArray[lcdCtrlDataPtr->spriteCounter], msg, BUFFER_SIZE);

        if( mode==LCD_DISPLAY_MODE2 )
        {
            for (int j=0; j<lcdCtrlDataPtr->spriteNum; j++ )
            {
                engine2.refresh( spritesPtr->x, spritesPtr->y, spritesPtr->x + display.width()-1, spritesPtr->y + lcdCtrlDataPtr->fontHeight - 1 );
            }
            engine2.display();
        }
        else
        {
            for (int j=0; j<lcdCtrlDataPtr->spriteNum; j++ )
            {
                engine1.refresh( spritesPtr->x, spritesPtr->y, spritesPtr->x + display.width()-1, spritesPtr->y + lcdCtrlDataPtr->fontHeight - 1 );
            }
            engine1.display();
        }
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
    return true;
}

bool SetStringToLCD(LCD_DISPLAY_MODE mode,LCD_SPRITE_NUM num,char *msg) {
    S_LCD_CTRL_DATA *lcdCtrlDataPtr = &lcdCtrlData[mode%LCD_DISPLAY_MODE_MAX];
    NanoPoint *spritesPtr = getSpritesPtr(mode);

    if( num < lcdCtrlDataPtr->spriteNum )
    {
        strncpy(lcdCtrlDataPtr->displayDataArray[num], msg, BUFFER_SIZE);
        if( mode==LCD_DISPLAY_MODE2 )
        {
            engine2.refresh( (spritesPtr+num)->x, (spritesPtr+num)->y, (spritesPtr+num)->x + display.width()-1, (spritesPtr+num)->y + lcdCtrlDataPtr->fontHeight - 1 );
            engine2.display();
        }
        else
        {
            engine1.refresh( (spritesPtr+num)->x, (spritesPtr+num)->y, (spritesPtr+num)->x + display.width()-1, (spritesPtr+num)->y + lcdCtrlDataPtr->fontHeight - 1 );
            engine1.display();
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
