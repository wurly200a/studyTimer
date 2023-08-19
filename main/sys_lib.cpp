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

void delay(unsigned long ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}
