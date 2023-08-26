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

#if 0
const Attribute_Request_Callback clientCallback(REQUESTED_CLIENT_ATTRIBUTES.cbegin(), REQUESTED_CLIENT_ATTRIBUTES.cend(), &processClientAttributeRequest);
#endif

// Statuses for requesting of attributes
bool requestedClient = false;

//WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP);

unsigned long currentTime=0;
unsigned long startTime=0;
unsigned long integrationTime=0;
unsigned long lastIntegrationTime=0;
unsigned long lastSentTimeStamp=0;

//int status = WL_IDLE_STATUS;

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
extern void telemetry_upload__main(void);
extern void ntp_init(void);
extern bool isNtpSyncCompleted(void);

extern unsigned int getEpochTime();
extern void tentativeTimeIncrement();

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
void updateTimeProc(void);
void displayInit(bool displayOn);
bool connectToThingsBoard(void);
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
    lcdSetup(LCD_DISPLAY_MODE1);

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
    int counter=0;
    char szBuffer[BUFFER_SIZE];

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
            tentativeTimeIncrement();
        }
        idle1secTimer++;

        if( 5000/MAIN_TSK_TICK <= idle5secTimer ){
            eventTrigger |= TRIGGER_5SEC;
            idle5secTimer=0;
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
    char szBuffer[BUFFER_SIZE];

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
        SetStringToLCD(LCD_DISPLAY_MODE2,LCD_SPRITE_NUM3,(char *)str.c_str());
        lastString = str;
    } else {
        // do nothing
    }
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
//  int y=STR_HEIGHT*lineNum;

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
#if 0
            if( tb.connected() ){
                tb.sendTelemetryBool(STUDY_KEY,false);
                tb.sendAttributeInt(STUDY_TIME_KEY,integrationTime);
                tb.sendAttributeInt(STUDY_TIME_STAMP_KEY,timeClient.getEpochTime());
                tb.sendTelemetryInt(STUDY_TIME_KEY,integrationTime);
            }
#endif
            measureOn = false;
        } else {
            // from OFF to ON
            startTime = currentTime;
#if 0
            if( tb.connected() ){
                tb.sendTelemetryBool(STUDY_KEY,true);
            }
#endif
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
//  Serial.println(msg);
}

void updateTimeProc(void) {
//  timeClient.update();
    unsigned int nowTime = 0;

  nowTime = getEpochTime();

    if( currentTime != nowTime ) {
        outputTimeToDisplay(nowTime,LINE_NUM1);
        if( startTime ){
            outputTimeToDisplay(nowTime-startTime,LINE_NUM2);
        }
        currentTime = nowTime;
    }
}

#if 0
void processClientAttributeRequest(const Shared_Attribute_Data &data) {
    for (auto it = data.begin(); it != data.end(); ++it) {
        if( !strcmp(it->key().c_str(),STUDY_TIME_KEY) ) {
            lastIntegrationTime = it->value().as<int>();
        } else if( !strcmp(it->key().c_str(),STUDY_TIME_STAMP_KEY) ) {
            lastSentTimeStamp = it->value().as<int>();
        } else {
//      Serial.println(it->key().c_str());
//      Serial.println(it->value().as<int>());
        }
    }

    bRequestingAttributes = false;
}
#endif

void displayInit(bool displayOn){
    if( displayOn ){
        outputTimeToDisplay(0,LINE_NUM2);
        outputTimeToDisplay(integrationTime,LINE_NUM3);
        printMsg("");
    } else {
        ClearLCD();
    }
}

bool connectToThingsBoard(void){
    bool rtn = false;

    printMsg("Connecting...");
//  Serial.print(THINGSBOARD_SERVER);
//  Serial.print(" with token ");
//  Serial.println(TOKEN);
#if 0
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
        printMsg("Disconnected");
    } else {
        printMsg("Connected");
        rtn = true;
    }
#endif

    return rtn;
}

///////////////////////////////////////////////////////////////////////////

void actionFuncInitial(unsigned int previousStatus){
    if (!requestedClient) {
        printMsg("Requesting attrs...");
//    requestedClient = tb.Client_Attributes_Request(clientCallback);
        if (!requestedClient) {
            printMsg("Failed to get attrs");
        } else {
            bRequestingAttributes = true;
        }
    }
}

int stateFuncInitial(unsigned int eventTrigger){
    static int initailSubStatus;
    int nextStatus = STATE_INITIAL;

#if 1
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
            PrintLCD(LCD_DISPLAY_MODE1,"Got current time");
            nextStatus = STATE_IDLE; /* tentative!!!! */
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
#else
    if( bRequestingAttributes ){
        // wait to get attributes
    } else {        nextStatus = STATE_IDLE;
    }

    if (!tb.connected()) {
        nextStatus = STATE_ERROR;
    }else{
        tb.loop(); // event loop of MQTT client
    }
#endif

    return nextStatus;
}

void actionFuncIdle(unsigned int previousStatus){
  if( previousStatus == STATE_INITIAL ) {
//    timeClient.update();
    string lastDate = epochTimeToDateString(lastSentTimeStamp);
    string nowDate = epochTimeToDateString(getEpochTime());

    lcdSetup(LCD_DISPLAY_MODE2);

    printMsg(lastDate);
    printMsg(nowDate);
    if( !lastDate.compare(nowDate) ){
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
        updateTimeProc();
        portOnTriggerProc();
        modeChangeProc();
    }

    if( eventTrigger & TRIGGER_1SEC ){
#if 0
        if (!tb.connected()) {
            nextStatus = STATE_ERROR;
        }else{
            // do nothing
        }

        tb.loop(); // event loop of MQTT client
#endif
    }

    if( eventTrigger & TRIGGER_5SEC ){
    }

    return nextStatus;
}

void actionFuncError(unsigned int previousStatus){
//  printMsg("Connect error");
}

int stateFuncError(unsigned int eventTrigger){
    int nextStatus = STATE_ERROR;

    if( eventTrigger & TRIGGER_1SEC ){
#if 0
        if (!tb.connected()) {
            if( connectToThingsBoard() ) {
//        printMsg("Recovered");
                nextStatus = STATE_IDLE;
            } else {
                // do nothing
            }
        }else{
            // do nothing
        }

#endif
    }

    return nextStatus;
}

///////////////////////////////////////////////////////////////////////////

int mySetup(void)
{
    int nextStatus = STATE_INITIAL;

    PrintLCD(LCD_DISPLAY_MODE1,"Study Timer!");

//  Serial.begin(115200); // Start serial monitor

    gpio_setup();

#if 1
    wifi_main();
#else
    wps_main();
#endif

#if 0
    telemetry_upload__main();
    PrintLCD(LCD_DISPLAY_MODE1,"Telemetry Upload");
#endif

#if 0
    if( connectToThingsBoard() ) {
        nextStatus = STATE_INITIAL;
    } else {
        nextStatus = STATE_ERROR;
    }
#endif
    nextStatus = STATE_INITIAL;
    return nextStatus;
}

