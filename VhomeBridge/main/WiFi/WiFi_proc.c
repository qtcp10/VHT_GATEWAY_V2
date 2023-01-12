/*
 * WiFi_proc.c
 *
 *  Created on: Jun 16, 2022
 *      Author: ASUS
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "WiFi_proc.h"
#include "mqtt_client.h"
#include "../common.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group_;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

extern enum system_state_t STATE;

static const char *TAG = "WIFI";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
		int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		ESP_LOGI(TAG, "start connect to AP");
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		wifi_event_sta_disconnected_t *sta_disconnect_evt = (wifi_event_sta_disconnected_t*)event_data;
		ESP_LOGI(TAG, "wifi disconnect reason: %d", sta_disconnect_evt->reason);
		esp_wifi_connect();
		STATE = WIFI_DISCONNECTED;
		ESP_LOGI(TAG, "STATE = WIFI_DISCONNECTED");
		ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
		wifi_ap_record_t ap;
		esp_wifi_sta_get_ap_info(&ap);
		printf("rssi: %d\n", ap.rssi);
		STATE = LOCAL_MODE;
		ESP_LOGI(TAG, "STATE = LOCAL_MODE");
		ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		xEventGroupSetBits(s_wifi_event_group_, WIFI_CONNECTED_BIT);
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_BEACON_TIMEOUT) {
		wifi_ap_record_t ap;
		esp_wifi_sta_get_ap_info(&ap);
		printf("----------------------------rssi: %d\n", ap.rssi);
		esp_wifi_connect();
	}
}

bool wifi_init_sta(wifi_config_t wifi_config, wifi_mode_t mode)
{
	s_wifi_event_group_ = xEventGroupCreate();

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
			ESP_EVENT_ANY_ID,
			&wifi_event_handler,
			NULL,
			&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
			IP_EVENT_STA_GOT_IP,
			&wifi_event_handler,
			NULL,
			&instance_got_ip));

	ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start() );
	//    ESP_ERROR_CHECK(esp_wifi_connect());

	ESP_LOGI(TAG, "wifi_init_sta finished.");

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	 EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group_,
			 WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
			 pdTRUE,
			 pdFALSE,
			 portMAX_DELAY);

	 /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	  * happened. */

	 if (bits & WIFI_CONNECTED_BIT) {
		 ESP_LOGI(TAG, "Connected to ap SSID:%s password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
		 return true;
	 } else if (bits & WIFI_FAIL_BIT) {
		 ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
		 return false;
	 } else {
		 ESP_LOGE(TAG, "UNEXPECTED EVENT");
		 return false;
	 }

//	 ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
//	 ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
//	 vEventGroupDelete(s_wifi_event_group_);
	 /* The event will not be processed after unregister */
}

void init_wifi()
{
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_ap();
	esp_netif_create_default_wifi_sta();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
}


