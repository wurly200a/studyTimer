#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_wps.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "study_timer_main";

extern void wps_main(void);

void app_main(void)
{
    ESP_LOGI(TAG, "APP_MAIN_START");
    wps_main();
}
