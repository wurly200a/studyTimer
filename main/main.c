#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_wps.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <string.h>
#include "lcd_lib.hpp"

static const char *TAG = "study_timer_main";

extern bool isWifiConnected( void );
extern void wps_main(void);
extern void telemetry_upload__main(void);

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define BUFFER_SIZE 32

static void delay(unsigned long ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

void sub_task(void *args)
{
    lcdSetup(LCD_DISPLAY_MODE1);

    for(;;) {
        lcdProc();
        delay(10);
    }
}

void main_task(void *args)
{
    int counter=0;
    char szBuffer[BUFFER_SIZE];

    xTaskCreatePinnedToCore(sub_task, "subTask", 8192, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
    PrintLCD(LCD_DISPLAY_MODE1,"Study Timer!");
    for(;;) {
//        PrintLCD(LCD_DISPLAY_MODE2,"00:00:00");
        sprintf(szBuffer,"%09d",counter);
        SetStringToLCD(LCD_DISPLAY_MODE2,LCD_SPRITE_NUM0,szBuffer);
        delay(1000);
        counter++;
    }
}

void app_main(void)
{
    int counter=0;
    char szBuffer[BUFFER_SIZE];

    ESP_LOGI(TAG, "APP_MAIN_START");
    xTaskCreatePinnedToCore(main_task, "mainTask", 8192, NULL, 1, NULL, ARDUINO_RUNNING_CORE);

    wps_main();
    while( !isWifiConnected() )
    {
        delay(1000);
        sprintf(szBuffer,"WaitForCnct(%d)",counter);
        PrintLCD(LCD_DISPLAY_MODE1,szBuffer);
        counter++;
    }
    PrintLCD(LCD_DISPLAY_MODE1,"Connected");
    delay(200);
    telemetry_upload__main();
    PrintLCD(LCD_DISPLAY_MODE1,"Telemetry Upload");
    lcdSetup(LCD_DISPLAY_MODE2);
}
