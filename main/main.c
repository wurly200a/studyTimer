#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_wps.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "study_timer_main";

extern bool isWifiConnected( void );
extern void wps_main(void);
extern void lcdSetup(void);
extern void lcdProc(void);
extern bool PrintLCD( char *msg );
extern void telemetry_upload__main(void);

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define BUFFER_SIZE 32
char szBuffer[BUFFER_SIZE];

static void delay(unsigned long ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

void sub_task(void *args)
{
    lcdSetup();

    for(;;) {
        lcdProc();
        delay(10);
    }
}

void main_task(void *args)
{
    xTaskCreatePinnedToCore(sub_task, "subTask", 8192, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
    PrintLCD("Study Timer!");
    for(;;) {
        delay(1000);
    }
}

void app_main(void)
{
    int counter=0;
    ESP_LOGI(TAG, "APP_MAIN_START");
    xTaskCreatePinnedToCore(main_task, "mainTask", 8192, NULL, 1, NULL, ARDUINO_RUNNING_CORE);

    wps_main();
    while( !isWifiConnected() )
    {
        delay(1000);
        sprintf(szBuffer,"WaitForCnct(%d)",counter);
        PrintLCD(szBuffer);
        counter++;
    }
    PrintLCD("Connected");
    delay(200);
    telemetry_upload__main();
    PrintLCD("Telemetry Upload");
}
