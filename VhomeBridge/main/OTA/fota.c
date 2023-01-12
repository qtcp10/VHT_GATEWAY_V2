/*
 * fota.c
 *
 *  Created on: Jun 21, 2022
 *      Author: ASUS
 */

#include "../OTA/fota.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "../common.h"

static const char *TAG = "OTA";
extern esp_mqtt_client_handle_t client;
#define OTA_URL_SIZE 256
extern char topic_msg[100];

static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }

    return ESP_OK;
}

static esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client)
{
    esp_err_t err = ESP_OK;
    return err;
}

void esp_ota_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting Advanced OTA");
    char url[100];
    if(pvParameter != 0)
    {
    	sprintf(url, "%s", (char*) pvParameter);
    	ESP_LOGI(TAG, "url_sv: %s", url);
    }
    else
    {
    	sprintf(url, "%s", URL_OTA);
    	ESP_LOGI(TAG, "url: %s", url);
    }
    int total_size_ota = 0;
    char pub_data[50];
    int threshold_percent = 0;
    esp_err_t ota_finish_err = ESP_OK;
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 5000,
        .keep_alive_enable = true,
		 config.skip_cert_common_name_check = true,
    };

    printf("5\r\n");
    esp_https_ota_config_t ota_config = {
        .http_config = &config,
        .http_client_init_cb = _http_client_init_cb, // Register a callback to be invoked after esp_http_client is initialized
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed");
        vTaskDelete(NULL);
    }

    total_size_ota = esp_https_ota_get_image_size(https_ota_handle);
    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed");
        goto ota_end;
    }
    err = validate_image_header(&app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "image header verification failed");
        goto ota_end;
    }

    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }

        if(esp_https_ota_get_image_len_read(https_ota_handle) * 100/ total_size_ota >= threshold_percent)
        {
        	threshold_percent = threshold_percent + 5;
			ESP_LOGI(TAG, "Dowload: %d%%", esp_https_ota_get_image_len_read(https_ota_handle) * 100/ total_size_ota);
			sprintf(pub_data, "{\"fwProcess\":%d}", esp_https_ota_get_image_len_read(https_ota_handle) * 100/ total_size_ota);
			esp_mqtt_client_publish(client, topic_msg, pub_data, strlen(pub_data), 0, 0);
        }
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true) {
        // the OTA image was not completely received and user can customise the response to this situation.
        ESP_LOGE(TAG, "Complete data was not received.");
    } else {
        ota_finish_err = esp_https_ota_finish(https_ota_handle);
        if ((err == ESP_OK) && (ota_finish_err == ESP_OK)) {
            ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
        } else {
            if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
                ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            }
            ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
            vTaskDelete(NULL);
        }
    }

ota_end:
    esp_https_ota_abort(https_ota_handle);
    ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed");
    vTaskDelete(NULL);
}
