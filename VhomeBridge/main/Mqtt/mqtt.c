/* MQTT (over TCP) Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "esp_wifi_types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "freertos/ringbuf.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "../jsonUser/json_user.h"
#include "../OTA/fota.h"
#include "../common.h"
#include "../BLE/ble_mesh.h"
#include "../common.h"
#include "../Zigbee/controller.h"
#include "../Zigbee/fota.h"
#include "../Zigbee/uart.h"

#include "../linkNode/json_parser/json_parser.h"
#include "../linkNode/linkButton.h"
#include "../linkNode/map/map.h"

static const char *TAG = "MQTT";

extern bool LINK_BUTTON;

esp_mqtt_client_config_t mqtt_cfg;
esp_mqtt_client_handle_t client;
RingbufHandle_t buf_handle;
extern QueueHandle_t queueIdButton;
extern QueueHandle_t queue_mqtt_handle;

extern char topic_msg[100];
extern char topic_cmd_set[100];
extern char fwVersion[30];
extern char topic_linkButton[100];
extern uint8_t provision_status;
extern enum system_state_t STATE;
extern map_str_t map;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
	esp_mqtt_event_handle_t event = event_data;
	esp_mqtt_client_handle_t client = event->client;
	switch ((esp_mqtt_event_id_t)event_id) {
	case MQTT_EVENT_CONNECTED:
		STATE = NORMAL;
		ESP_LOGI(TAG, "STATE = NORMAL");
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		esp_mqtt_client_subscribe(client, topic_cmd_set, 0);
		esp_mqtt_client_subscribe(client, topic_linkButton, 0);
		esp_mqtt_client_publish(client, topic_msg, fwVersion, strlen(fwVersion), 0, 0);
		break;
	case MQTT_EVENT_DISCONNECTED:
		STATE = LOCAL_MODE;
		ESP_LOGI(TAG, "STATE = LOCAL_MODE");
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		break;

	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_DATA:
	{
		if (event->data_len > 0 && event->data) {
			xRingbufferSendFromISR(buf_handle, event->data, event->data_len, (BaseType_t *)10);
			gpio_set_level(3, 0);
			vTaskDelay(100/portTICK_RATE_MS);
			gpio_set_level(3, 1);
//			xQueueSend(queue_mqtt_handle, (void *) (event->data), (TickType_t)10);
		}
		break;
	}
	case MQTT_EVENT_ERROR:
		ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
		break;
	default:
		ESP_LOGI(TAG, "Other event id:%d", event->event_id);
		break;
	}
}

void mqtt_handle(void *arg) {
	cmd command_set;
	char * item = NULL;
	size_t item_size;
	while(1) {
		item = (char *)xRingbufferReceiveFromISR(buf_handle, &item_size);
		if((item_size > 0) && item)
		{
			item[item_size] = '\0';
			ESP_LOGI(TAG, "PAYLOAD: %s", item);
			memset(&command_set, 0, sizeof(command_set));
			JSON_analyze_SUB_MQTT(item, &command_set);
			ESP_LOGI(TAG, "action: %s\n", (char*)command_set.action);
			if(strcmp(command_set.action, "upgrade") == 0)
			{
				if(strlen(command_set.url) > 0){
					ble_deinit();
					xTaskCreate(&esp_ota_task, "ota_task", 8192, command_set.url, 5, NULL);
				} else {
					ble_deinit();
					xTaskCreate(&esp_ota_task, "ota_task", 8192, NULL, 5, NULL);
				}
			} else if(strcmp(command_set.action, "zigbee_upgrade") == 0){
				ble_deinit();
				zigbee_boot();
				vTaskDelay(500/portTICK_RATE_MS);
				xTaskCreate(&zigbee_fota_task, "zigbee_fota_task", 1024 * 4, NULL, 100, NULL);
			} else if(strcmp(command_set.action, "bleopen") == 0){
				provision_status = 1;
			} else if(strcmp(command_set.action, "blereset") == 0){
				ble_mesh_node_reset(mParseHex(command_set.nodeID, 6));
			} else if(strcmp(command_set.action, "open") == 0){
				char data[100] = {'\0'};
				sprintf(data, "custom action-open\n");
				uart_write_bytes(UART_NUM_1, data, strlen(data));
				printf("%s\r\n", data);
			} else if(strcmp(command_set.action, "close") == 0){
				provision_status = 0;
				char data[100] = {'\0'};
				sprintf(data, "custom action-close\n");
				uart_write_bytes(UART_NUM_1, data, strlen(data));
			} else if(strcmp(command_set.action, "restart") == 0){
				esp_restart();
			} else if(strcmp(command_set.action, "on-off") == 0){
				if(strcmp(command_set.protocol, "ble") == 0){
					int value = (strcmp(command_set.value, "off") == 0) ? 0:1;
					ble_mesh_command_set(ONOFF, (uint16_t)mParseHex(command_set.nodeID, 6), command_set.endpoint - 1, value, 1);
					vTaskDelay(100/portTICK_RATE_MS);
				} else {
					char data[100] = {'\0'};
					sprintf(data, "custom action-onoff %s %s %d\n", command_set.value, command_set.nodeID, command_set.endpoint);
					printf("------%s\r\n", data);
					uart_write_bytes(UART_NUM_1, data, strlen(data));
				}
				if (LINK_BUTTON) {
					linkButton_t dataLinkButton;
					sprintf(dataLinkButton.idButton, "%s%s%d", command_set.nodeID, command_set.protocol, command_set.endpoint);
					if (strcmp(command_set.value, "off") == 0) {
						memcpy(dataLinkButton.stateButton, "off", strlen("off") + 1);
					} else {
						memcpy(dataLinkButton.stateButton, "on", strlen("on") + 1);
					}
					xQueueSend(queueIdButton, (void *) &dataLinkButton, (TickType_t)100);
				}

			} else if(strcmp(command_set.action, "lock") == 0) {
				char data[50] = {'\0'};
				sprintf(data, "zcl lock lock \"000000\"\n");
				uart_write_bytes(UART_NUM_1, data, strlen(data));
				vTaskDelay(10/portTICK_RATE_MS);
				printf("%s\r\n", data);
				sprintf(data, "send %s 0x01 0x01\n", command_set.nodeID);
				uart_write_bytes(UART_NUM_1, data, strlen(data));
			} else if(strcmp(command_set.action, "unlock") == 0) {
				char data[50] = {'\0'};
				sprintf(data, "zcl lock unlock \"000000\"\n");
				uart_write_bytes(UART_NUM_1, data, strlen(data));
				vTaskDelay(10/portTICK_RATE_MS);
				printf("%s\r\n", data);
				sprintf(data, "send %s 0x01 0x01\n", command_set.nodeID);
				uart_write_bytes(UART_NUM_1, data, strlen(data));
			}
			else if (strcmp(command_set.action, "linkButton") == 0) {
				writetofile("link_node", item);
				map = json_parsing(item);
			}
			vRingbufferReturnItem(buf_handle, (void *)item);
		}
		vTaskDelay(10/portTICK_RATE_MS);
	}
}

void mqtt_app_start(char *broker, char *client_id, char *passowrd)
{
	buf_handle = xRingbufferCreate(4096, RINGBUF_TYPE_NOSPLIT);
	if (buf_handle == NULL) {
		ESP_LOGE(TAG, "Failed to create ring buffer\n");
	}
	mqtt_cfg.uri = broker;
	mqtt_cfg.username = client_id;
	mqtt_cfg.client_id = client_id;
	mqtt_cfg.password = passowrd;
	mqtt_cfg.keepalive = 60;
	client = esp_mqtt_client_init(&mqtt_cfg);
	xTaskCreate(mqtt_handle, "mqtt_handle", 8192, NULL, 3, NULL);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
	esp_mqtt_client_start(client);
}
