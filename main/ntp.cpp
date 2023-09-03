#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "sys_lib.hpp"

static void initialize_sntp(void);

static const char* TAG = "ntp_client";

static bool s_time_sync_completed = false;
#define NTP_RETRY_COUNT 5000

void time_sync_notification_cb(struct timeval *tv)
{
    s_time_sync_completed = true;
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

void ntp_init(void) {
    ESP_LOGI(TAG, "Initializing SNTP");
    initialize_sntp();

    int retry = 0;
    const int retry_count = NTP_RETRY_COUNT;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        if( !(retry % 100) )
        {
            ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        }
        delay(10);
    }
}

unsigned int getEpochTime(void) {
    unsigned int rtn = 0;

    if( s_time_sync_completed )
    {
        time_t now = {0};
        struct tm timeinfo;

        time(&now);
        localtime_r(&now, &timeinfo);

        struct tm* gm_timeinfo = gmtime(&now);
        int timezone_offset = (int)difftime(mktime(&timeinfo), mktime(gm_timeinfo));

        rtn = now + timezone_offset;
    }
    else
    {
        // do nothing
    }
    return (unsigned int)rtn;
}

static void initialize_sntp(void)
{
    setenv("TZ", "JST-9", 1);
    tzset();

    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
//    esp_sntp_setservername(0, "ntp.nict.jp");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    esp_sntp_init();
}

bool isNtpSyncCompleted(void)
{
    return s_time_sync_completed;
}
