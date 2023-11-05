#pragma once

typedef enum tag_LCD_DISPLAY_MODE
{
    LCD_DISPLAY_MODE1,
    LCD_DISPLAY_MODE2,
    LCD_DISPLAY_MODE_MAX
} LCD_DISPLAY_MODE;

typedef enum tag_LCD_SPRITE_NUM
{
    LCD_SPRITE_NUM0,
    LCD_SPRITE_NUM1,
    LCD_SPRITE_NUM2,
    LCD_SPRITE_NUM3,
    LCD_SPRITE_NUM4,
    LCD_SPRITE_NUM5,
    LCD_SPRITE_NUM6,
    LCD_SPRITE_NUM7,
    LCD_SPRITE_NUM_MAX
} LCD_SPRITE_NUM;

#define BUFFER_SIZE 32

typedef struct tag_S_LCD_CTRL_DATA
{
    int writeIndex;
    int spriteCounter;
    char displayDataArray[LCD_SPRITE_NUM_MAX][BUFFER_SIZE];
    char displayDataArrayBuffer[BUFFER_SIZE];
    bool bufferExist;
    int fontHeight;
    int spriteNum;
    int posYmin;
    int posYmax;
} S_LCD_CTRL_DATA;

void lcdCommonSetup(void);
void lcdSetup(LCD_DISPLAY_MODE mode);

void lcdProc();

bool PrintLCD(LCD_DISPLAY_MODE mode,char *msg);

bool SetStringToLCD(LCD_DISPLAY_MODE mode,LCD_SPRITE_NUM num,char *msg);

bool SetGraphicToLCD(bool turnOn);

bool SetHeartToLCD(bool turnOn, int heartNum, bool blinkOn);

void ClearLCD(void);
