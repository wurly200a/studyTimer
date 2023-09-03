#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_wps.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <string.h>
#include "sys_lib.hpp"
#include "lcd_lib.hpp"
#include <string>
#include "thingsboard.hpp"

using namespace std;

enum
{
  STATE_INITIAL,
  STATE_IDLE   ,
  STATE_ERROR  ,
  STATE_NUM_MAX
};
int currentStatus = STATE_INITIAL;

bool bRequestingAttributes = false;

#define TRIGGER_100MSEC 0x00000001
#define TRIGGER_1SEC    0x00000002
#define TRIGGER_5SEC    0x00000004

constexpr const char STUDY_KEY[] = "study";
constexpr const char STUDY_TIME_KEY[] = "todayTotalStudyTime";
constexpr const char STUDY_TIME_STAMP_KEY[] = "todayTotalStudyTimeTimeStamp";

unsigned long currentTime=0;
unsigned long startTime=0;
unsigned long integrationTime=0;
unsigned long lastIntegrationTime=0;
unsigned long lastSentTimeStamp=0;

#define DIN_PIN 2
#define DIN_MODE_PIN 0

enum
{
  SWITCH_START_STOP,
  SWITCH_MODE,
  SWITCH_NUM_MAX
};

const int switchOnTable[SWITCH_NUM_MAX]=
{
  DIN_PIN,
  DIN_MODE_PIN,
};

int switchOnCount[SWITCH_NUM_MAX];
bool measureOn;
int displayMode;
bool switchTrigger[SWITCH_NUM_MAX];
int switchTriggerMaskCount[SWITCH_NUM_MAX];

static const char *TAG = "study_timer_main";

extern void wifi_main();
extern bool isWifiConnected( void );
extern void wps_main(void);
extern void ntp_init(void);
extern bool isNtpSyncCompleted(void);

extern unsigned int getEpochTime();

extern void gpio_setup(void);
extern bool gpio_port_read(int io_num);
extern void gpio_main(void);

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define BUFFER_SIZE 32

int matchStrings(string str1, string str2);
void outputString(string str);
void outputTimeToDisplay(unsigned long time, int lineNum);
void portCheck(void);
void portOnTriggerProc(void);
void modeChangeProc(void);
string epochTimeToDateString(unsigned long epochTime);
string getFormattedTime(unsigned long secs);
void printMsg(string msg);
void updateTimeProc(bool forceOn);
void displayInit(bool displayOn);
void actionFuncInitial(unsigned int previousStatus);
int stateFuncInitial(unsigned int eventTrigger);
void actionFuncIdle(unsigned int previousStatus);
int stateFuncIdle(unsigned int eventTrigger);
void actionFuncError(unsigned int previousStatus);
int stateFuncError(unsigned int eventTrigger);
int mySetup(void);
void setup();
void loop();

void sub_task(void *args)
{
    lcdCommonSetup();
    lcdSetup(LCD_DISPLAY_MODE1);
    lcdSetup(LCD_DISPLAY_MODE2);

    for(;;) {
        lcdProc();
        delay(1);
    }
}

int (*stateFunc[STATE_NUM_MAX])(unsigned int)={
    stateFuncInitial,
    stateFuncIdle   ,
    stateFuncError
};

void (*actionFunc[STATE_NUM_MAX])(unsigned int)={
    actionFuncInitial,
    actionFuncIdle   ,
    actionFuncError
};

void setup() {
    int nextStatus = mySetup();
    actionFunc[nextStatus](0);
    currentStatus = nextStatus;
}

#define MAIN_TSK_TICK 1 /* msec */

static int idle5secTimer;
static int idle1secTimer;
static int idle100msecTimer;
void main_task(void *args)
{
//    int counter=0;
    static char szBuffer[BUFFER_SIZE];

    xTaskCreatePinnedToCore(sub_task, "subTask", 8192, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
    setup();

    for(;;) {
        int nextStatus;
        unsigned int eventTrigger = 0;

        if( 100/MAIN_TSK_TICK <= idle100msecTimer ){
#if 0
            char tempString[20];
            sprintf(tempString,"%d,%d,%d",idle100msecTimer,idle1secTimer,idle5secTimer);
            printMsg(tempString);
#endif
            eventTrigger |= TRIGGER_100MSEC;
            idle100msecTimer=0;
        }
        idle100msecTimer++;

        if( 1000/MAIN_TSK_TICK <= idle1secTimer ){
            eventTrigger |= TRIGGER_1SEC;
            idle1secTimer=0;
#if 0
            sprintf(szBuffer,"currentSts:%01d",currentStatus);
            SetStringToLCD(LCD_DISPLAY_MODE2,LCD_SPRITE_NUM3,szBuffer);
            counter++;
#endif
        }
        idle1secTimer++;

        if( 5000/MAIN_TSK_TICK <= idle5secTimer ){
            eventTrigger |= TRIGGER_5SEC;
            idle5secTimer=0;
#if 0
            sprintf(szBuffer,"counter:%d",counter);
            PrintLCD(LCD_DISPLAY_MODE1,szBuffer);
            counter++;
#endif
        }
        idle5secTimer++;

        nextStatus = stateFunc[currentStatus](eventTrigger);

        if( nextStatus != currentStatus ){
            actionFunc[nextStatus](currentStatus);
        }
        currentStatus = nextStatus;

        delay(MAIN_TSK_TICK);
    }
}

extern "C" void study_timer_main(void)
{
    int counter=0;
    static char szBuffer[BUFFER_SIZE];

    ESP_LOGI(TAG, "STUDY_TIMER_MAIN_START");
    xTaskCreatePinnedToCore(main_task, "mainTask", 8192, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
}

int matchStrings(string str1, string str2) {
    int matchCount = 0;
    for(int i=0; i<min(str1.length(), str2.length()); i++) {
        if(str1[i] == str2[i]) {
            matchCount++;
        } else {
            break;
        }
    }
    return matchCount;
}

string lastString;
void outputString(string str)
{
    int size=1;

    if( displayMode == 0 ){
        int matchStringNum = matchStrings(lastString,str);
//        PrintLCD(LCD_DISPLAY_MODE1,(char *)str.c_str());
        lastString = str;
    } else {
        // do nothing
    }

    ESP_LOGI(TAG, "outputString: %s", str.c_str());
}

enum
{
    LINE_NUM1,
    LINE_NUM2,
    LINE_NUM3,
    LINE_NUM_MAX,
};
string lastTimeString[LINE_NUM_MAX];
void outputTimeToDisplay(unsigned long time, int lineNum)
{
  int size=2;

  if( displayMode == 0 ){
    string timeString = getFormattedTime(time);
    int matchStringNum = matchStrings(lastTimeString[lineNum],timeString);

    LCD_SPRITE_NUM num;
    switch( lineNum )
    {
    case LINE_NUM1:
        num = LCD_SPRITE_NUM0;
        break;
    case LINE_NUM2:
        num = LCD_SPRITE_NUM1;
        break;
    case LINE_NUM3:
        num = LCD_SPRITE_NUM2;
        break;
    default:
        num = LCD_SPRITE_NUM3;
        break;
    }
    SetStringToLCD(LCD_DISPLAY_MODE2,num,(char *)timeString.c_str());
    lastTimeString[lineNum] = timeString;
  } else {
    // do nothing
  }
}

void portCheck(void)
{
    for( int i=0; i<SWITCH_NUM_MAX; i++ ){
        if( switchTriggerMaskCount[i] ){
            switchTriggerMaskCount[i]--;
        } else {
            if( gpio_port_read(switchOnTable[i]) ){
                switchOnCount[i] = 0;
            } else {
                switchOnCount[i]++;
                if( 5 < switchOnCount[i] ){
                    switchTrigger[i] = true;
                    switchTriggerMaskCount[i] = 500;
                } else {
                    switchOnCount[i]++;
                }
            }
        }
    }
}

void portOnTriggerProc(void)
{
    if( switchTrigger[SWITCH_START_STOP] ){
        if( measureOn ){
            // from ON to OFF
            integrationTime += currentTime-startTime;
            outputTimeToDisplay(integrationTime,LINE_NUM3);
            startTime = 0;
            if( isThingsBoardConnected() ){
                thingsBoardSendTelemetryBool(STUDY_KEY,false);
                thingsBoardSendAttributeInt(STUDY_TIME_KEY,integrationTime);
                thingsBoardSendAttributeInt(STUDY_TIME_STAMP_KEY,getEpochTime());
                thingsBoardSendTelemetryInt(STUDY_TIME_KEY,integrationTime);
            }
            measureOn = false;
        } else {
            // from OFF to ON
            startTime = currentTime;
            if( isThingsBoardConnected() ){
                thingsBoardSendTelemetryBool(STUDY_KEY,true);
            }
            measureOn = true;
        }
        switchTrigger[SWITCH_START_STOP]= false;
    } else {
        // do nothing
    }
}

void modeChangeProc(void)
{
    if( switchTrigger[SWITCH_MODE] ){
        displayMode ^= 1;
        displayInit(!displayMode);
        switchTrigger[SWITCH_MODE]= false;
    } else {
        // do nothing
    }
}

string epochTimeToDateString(unsigned long epochTime) {
    time_t t = epochTime;

    struct tm *tm = localtime(&t);

    char dateString[36];
    sprintf(dateString, "%04d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);

    return string(dateString);
}

string getFormattedTime(unsigned long secs) {
    unsigned long rawTime = secs;
    unsigned long hours = (rawTime % 86400L) / 3600;
    string hoursStr = hours < 10 ? "0" + to_string(hours) : to_string(hours);

    unsigned long minutes = (rawTime % 3600) / 60;
    string minuteStr = minutes < 10 ? "0" + to_string(minutes) : to_string(minutes);

    unsigned long seconds = rawTime % 60;
    string secondStr = seconds < 10 ? "0" + to_string(seconds) : to_string(seconds);

    return hoursStr + ":" + minuteStr + ":" + secondStr;
}

void printMsg(string msg) {
    outputString(msg);
}

void updateTimeProc(bool forceOn) {
    unsigned int nowTime = 0;

//  timeClient.update();
    nowTime = getEpochTime();

    if( forceOn || currentTime != nowTime ) {
        outputTimeToDisplay(nowTime,LINE_NUM1);
        if( startTime ){
            outputTimeToDisplay(nowTime-startTime,LINE_NUM2);
        }
        currentTime = nowTime;
    }
}

void displayInit(bool displayOn){
    if( displayOn ){
        updateTimeProc(true);
        outputTimeToDisplay(0,LINE_NUM2);
        outputTimeToDisplay(integrationTime,LINE_NUM3);
        printMsg("");
    } else {
        ClearLCD();
    }
}

///////////////////////////////////////////////////////////////////////////

void actionFuncInitial(unsigned int previousStatus){
#if 1
    wifi_main();
#else
    wps_main();
#endif
}

int stateFuncInitial(unsigned int eventTrigger){
    static int initailSubStatus;
    int nextStatus = STATE_INITIAL;
    int counter=0;

    if( isWifiConnected() )
    {
        if( initailSubStatus == 0 )
        {
            PrintLCD(LCD_DISPLAY_MODE1,"Connected");
            ntp_init();
            initailSubStatus = 1;
        }
        else
        {
            // do nothing
        }

        if( isNtpSyncCompleted() )
        {
            if( initailSubStatus == 1 )
            {
                PrintLCD(LCD_DISPLAY_MODE1,"Got current time");
                PrintLCD(LCD_DISPLAY_MODE1,"Connect to ThingsBoard");
                connectToThingsBoard();
                initailSubStatus = 2;
                counter=0;
            }
            else
            {
                if( isThingsBoardConnected() )
                {
                    if( initailSubStatus == 2 )
                    {
                        thingsBoardClientAttributesrequestSend();
                        initailSubStatus = 3;
                    }
                    else
                    {
                        if( getClientAttribute(&lastIntegrationTime,&lastSentTimeStamp) )
                        {
                            nextStatus = STATE_IDLE;
                        }
                        else
                        {
                            // do nothing
                        }
                    }
                } else {
                    if( initailSubStatus == 2 )
                    {
                        counter++;
                        if( 7000 < counter )
                        {
                            // retry
                            disconnectFromThingsBoard();
                            connectToThingsBoard();
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


                if( 2 <= initailSubStatus )
                {
                    thingsBoardLoop();
                }
                else
                {
                    // do nothing
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

    return nextStatus;
}

void actionFuncIdle(unsigned int previousStatus){
  if( previousStatus == STATE_INITIAL ) {
//    timeClient.update();
    string lastDate = epochTimeToDateString(lastSentTimeStamp);
    string nowDate = epochTimeToDateString(getEpochTime());

    printMsg(lastDate);
    printMsg(nowDate);
    if( lastDate.compare(nowDate) ){
      integrationTime = 0;
    } else {
      integrationTime = lastIntegrationTime;
    }

    //  integrationTime = 1140; // to set initial value
    //  tb.sendAttributeInt(STUDY_TIME_KEY,integrationTime); // to set initial value
    //  timeClient.update(); // to set initial value
    //  tb.sendAttributeInt(STUDY_TIME_STAMP_KEY,timeClient.getEpochTime()); // to set initial value
    displayInit(true);
  } else {
    // do nothing
  }
}

int stateFuncIdle(unsigned int eventTrigger){
    int nextStatus = STATE_IDLE;

    portCheck();

    if( eventTrigger & TRIGGER_100MSEC ){
        updateTimeProc(false);
        portOnTriggerProc();
        modeChangeProc();
    }

    if( eventTrigger & TRIGGER_1SEC ){
        if (!isThingsBoardConnected()) {
            nextStatus = STATE_ERROR;
        }else{
            // do nothing
        }
        thingsBoardLoop(); // event loop of MQTT client
    }

    if( eventTrigger & TRIGGER_5SEC ){
//        static int counter;
//        static char szBuffer[BUFFER_SIZE];
//        counter++;
//        sprintf(szBuffer,"{\"counter\": %d}",counter);
//        thingsBoardSendTelemetry(szBuffer);
    }

    return nextStatus;
}

void actionFuncError(unsigned int previousStatus){
    printMsg("Connect error");

    disconnectFromThingsBoard();
    connectToThingsBoard();
}

int stateFuncError(unsigned int eventTrigger){
    int nextStatus = STATE_ERROR;

    if( eventTrigger & TRIGGER_1SEC ){
        if (isThingsBoardConnected()) {
            nextStatus = STATE_IDLE;
        }else{
            thingsBoardLoop(); // event loop of MQTT client
        }
    }

    return nextStatus;
}

///////////////////////////////////////////////////////////////////////////

int mySetup(void)
{
    int nextStatus = STATE_INITIAL;

    PrintLCD(LCD_DISPLAY_MODE1,"Study Timer!");

    gpio_setup();

    return nextStatus;
}

