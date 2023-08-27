/* Publish telemetry data to ThingsBoard platform Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"

#include "tbc_mqtt_helper.h"
#include "sys_lib.hpp"

static tbcmh_handle_t tb_client;
static bool s_connected = false;

static const char *TAG = "THINGSBOARD";

#define CONFIG_BROKER_URL "mqtt://demo.thingsboard.io"
#define CONFIG_ACCESS_TOKEN "xxxxxx"

/*!< Callback of connected ThingsBoard MQTT */
void tb_on_connected(tbcmh_handle_t client, void *context)
{
    s_connected = true;
    ESP_LOGI(TAG, "Connected to thingsboard server!");
}

/*!< Callback of disconnected ThingsBoard MQTT */
void tb_on_disconnected(tbcmh_handle_t client, void *context)
{
    s_connected = false;
    ESP_LOGI(TAG, "Disconnected from thingsboard server!");
}

bool isThingsBoardConnected(void)
{
    return s_connected;
}

void thingsBoardLoop(void)
{
    if (tbcmh_has_events(tb_client)) {
        tbcmh_run(tb_client);
    }
}

void thingsBoardSendTelemetry(char *str)
{
    if (tbcmh_is_connected(tb_client)) {
#if 1
        tbcmh_telemetry_upload(tb_client, str,
                               1/*qos*/, 0/*retain*/);
#else
        tbcmh_telemetry_upload(tb_client, "{\"temprature\": 25.5}",
                               1/*qos*/, 0/*retain*/);
#endif
    } else {
        ESP_LOGI(TAG, "Still NOT connected to server!");
    }
}

void connectToThingsBoard(void)
{
    bool go_to_destroy_flag = false;
	//tbc_err_t err;
    const char *access_token = CONFIG_ACCESS_TOKEN;
    const char *uri = CONFIG_BROKER_URL;

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO); //ESP_LOG_DEBUG
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    //esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    esp_log_level_set("tb_mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("tb_mqtt_client_helper", ESP_LOG_VERBOSE);
    esp_log_level_set("attributes_reques", ESP_LOG_VERBOSE);
    esp_log_level_set("clientattribute", ESP_LOG_VERBOSE);
    esp_log_level_set("clientrpc", ESP_LOG_VERBOSE);
    esp_log_level_set("otaupdate", ESP_LOG_VERBOSE);
    esp_log_level_set("serverrpc", ESP_LOG_VERBOSE);
    esp_log_level_set("sharedattribute", ESP_LOG_VERBOSE);
    esp_log_level_set("telemetry_upload", ESP_LOG_VERBOSE);

//    ESP_ERROR_CHECK(nvs_flash_init());
//    ESP_ERROR_CHECK(esp_netif_init());
//    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
//    ESP_ERROR_CHECK(example_connect());


    ESP_LOGI(TAG, "Init tbcmh ...");
    tb_client = tbcmh_init();
    if (!tb_client) {
        ESP_LOGE(TAG, "Failure to init tbcmh!");
        return;
    }

    ESP_LOGI(TAG, "Connect tbcmh ...");
    tbc_transport_config_esay_t config = {
        .uri = uri,                     /*!< Complete ThingsBoard MQTT broker URI */
        .access_token = access_token,   /*!< ThingsBoard Access Token */
        .log_rxtx_package = true                /*!< print Rx/Tx MQTT package */
     };
    bool result = tbcmh_connect_using_url(tb_client, &config,
                        NULL, tb_on_connected, tb_on_disconnected);
    if (!result) {
        ESP_LOGE(TAG, "failure to connect to tbcmh!");
        go_to_destroy_flag = true;
    }
}

void connectFromThingsBoard(void)
{
    ESP_LOGI(TAG, "Disconnect tbcmh ...");
    tbcmh_disconnect(tb_client);

    ESP_LOGI(TAG, "Destroy tbcmh ...");
    tbcmh_destroy(tb_client);
}
