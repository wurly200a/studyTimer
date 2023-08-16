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

extern void wps_main(void);
extern void setup(void);
extern void loop(void);

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

void main_task(void *args)
{
    setup();
    for(;;) {
        loop();
#ifdef SDL_EMULATION
        sdl_read_analog(0);
#endif
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "APP_MAIN_START");
    xTaskCreatePinnedToCore(main_task, "mainTask", 8192, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
    wps_main();
}
