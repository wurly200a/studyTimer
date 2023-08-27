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
#include "thingsboard.hpp"

void parseAndStoreData(const char* jsonString, unsigned long* todayTotalStudyTime, unsigned long* todayTotalStudyTimeTimeStamp);

static tbcmh_handle_t tb_client;
static bool s_connected = false;
static bool s_clientAttributesGot = false;

static const char *TAG = "THINGSBOARD";

#define BUFFER_SIZE 32

#define CONFIG_BROKER_URL "mqtt://demo.thingsboard.io"
#define CONFIG_ACCESS_TOKEN "xxxxxx"

#define CLIENTATTRIBUTE_SETPOINT    	"todayTotalStudyTimeTimeStamp"
#define SHAREDATTRIBUTE_SNTP_SERVER     "sntp_server"

constexpr const char STUDY_KEY[] = "study";
constexpr const char STUDY_TIME_KEY[] = "todayTotalStudyTime";
constexpr const char STUDY_TIME_STAMP_KEY[] = "todayTotalStudyTimeTimeStamp";

unsigned long todayTotalStudyTime;
unsigned long todayTotalStudyTimeTimeStamp;

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

bool getClientAttribute(unsigned long* todayTotalStudyTimePtr, unsigned long* todayTotalStudyTimeTimeStampPtr)
{
    if( s_clientAttributesGot )
    {
        *todayTotalStudyTimePtr          = todayTotalStudyTime;
        *todayTotalStudyTimeTimeStampPtr = todayTotalStudyTimeTimeStamp;
    }
    else
    {
        // do nothing;
    }

    return s_clientAttributesGot;
}

void tb_attributesrequest_on_response(tbcmh_handle_t client,
                             void *context,
                             const cJSON *client_attributes,
                             const cJSON *shared_attributes) //, uint32_t request_id
{
    ESP_LOGI(TAG, "Receiving response of the attributes request!"); //request_id=%u, request_id

    if (client_attributes) {
        char *pack = cJSON_PrintUnformatted(client_attributes); //cJSON_Print()
        ESP_LOGI(TAG, "client_attributes: %s", pack);

        parseAndStoreData(pack, &todayTotalStudyTime, &todayTotalStudyTimeTimeStamp);
        ESP_LOGI(TAG,"todayTotalStudyTime: %lu\n", todayTotalStudyTime);
        ESP_LOGI(TAG,"todayTotalStudyTimeTimeStamp: %lu\n", todayTotalStudyTimeTimeStamp);

        cJSON_free(pack); // free memory
        s_clientAttributesGot = true;
    }

    if (shared_attributes) {
        char *pack = cJSON_PrintUnformatted(shared_attributes); //cJSON_Print()
        ESP_LOGI(TAG, "shared_attributes: %s", pack);
        cJSON_free(pack); // free memory
    }
}

// return 2 if tbcmh_disconnect()/tbcmh_destroy() is called inside it.
//      Caller (TBCMH library) will process other attributes request timeout.
// return 0/ESP_OK on success
// return -1/ESP_FAIL on failure
void tb_attributesrequest_on_timeout(tbcmh_handle_t client, void *context) //, uint32_t request_id
{
    ESP_LOGI(TAG, "Timeout of the attributes request!"); // request_id=%u, request_id
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

void thingsBoardClientAttributesrequestSend(void)
{
    tbcmh_clientattributes_request(tb_client,NULL,
                             (tbcmh_attributes_on_response_t)tb_attributesrequest_on_response,
                             (tbcmh_attributes_on_timeout_t)tb_attributesrequest_on_timeout,
                             2,STUDY_TIME_KEY, STUDY_TIME_STAMP_KEY);
    s_clientAttributesGot = false;
}

void thingsBoardAttributesrequestSend(void)
{
    ESP_LOGI(TAG, "Request attributes, client attributes: %s; shared attributes: %s",
        CLIENTATTRIBUTE_SETPOINT, SHAREDATTRIBUTE_SNTP_SERVER);

    tbcmh_attributes_request(tb_client, NULL,
                             (tbcmh_attributes_on_response_t)tb_attributesrequest_on_response,
                             (tbcmh_attributes_on_timeout_t)tb_attributesrequest_on_timeout,
                             CLIENTATTRIBUTE_SETPOINT, SHAREDATTRIBUTE_SNTP_SERVER);
}

void thingsBoardSendTelemetry(const char *str)
{
    if (tbcmh_is_connected(tb_client)) {
        tbcmh_telemetry_upload(tb_client, str,
                               1/*qos*/, 0/*retain*/);
    } else {
        ESP_LOGI(TAG, "Still NOT connected to server!");
    }
}

void thingsBoardSendTelemetryInt(const char *key,long value)
{
    char szBuffer[BUFFER_SIZE];

    if (tbcmh_is_connected(tb_client)) {
        sprintf(szBuffer,"{\"%s\": %ld}",key,value);
        tbcmh_telemetry_upload(tb_client, szBuffer,
                               1/*qos*/, 0/*retain*/);
    } else {
        ESP_LOGI(TAG, "Still NOT connected to server!");
    }
}

void thingsBoardSendTelemetryBool(const char *key,bool value)
{
    char szBuffer[BUFFER_SIZE];

    if (tbcmh_is_connected(tb_client)) {
        sprintf(szBuffer,"{\"%s\": %s}",key,value ? "true" : "false" );
        tbcmh_telemetry_upload(tb_client, szBuffer,
                               1/*qos*/, 0/*retain*/);
    } else {
        ESP_LOGI(TAG, "Still NOT connected to server!");
    }
}

void thingsBoardSendAttributeInt(const char *key,long value)
{
    char szBuffer[BUFFER_SIZE];

    if (tbcmh_is_connected(tb_client)) {
        sprintf(szBuffer,"{\"%s\": %ld}",key,value);
        tbcmh_attributes_update(tb_client, szBuffer,
                               1/*qos*/, 0/*retain*/);
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

void parseAndStoreData(const char* jsonString, unsigned long* todayTotalStudyTime, unsigned long* todayTotalStudyTimeTimeStamp) {
    cJSON* root = cJSON_Parse(jsonString);
    if (root == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return;
    }

    cJSON* integrationTimeNode = cJSON_GetObjectItem(root, "todayTotalStudyTime");
    cJSON* sentTimeStampNode = cJSON_GetObjectItem(root, "todayTotalStudyTimeTimeStamp");

    if (integrationTimeNode != NULL && integrationTimeNode->type == cJSON_Number &&
        sentTimeStampNode != NULL && sentTimeStampNode->type == cJSON_Number) {
        *todayTotalStudyTime = (unsigned long)integrationTimeNode->valuedouble;
        *todayTotalStudyTimeTimeStamp = (unsigned long)sentTimeStampNode->valuedouble;
    }

    cJSON_Delete(root);
}
