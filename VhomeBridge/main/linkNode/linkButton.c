#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "freertos/queue.h"

#include "esp_ble_mesh_defs.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/uart.h"

#include "mqtt_client.h"

#include "../common.h"
#include "../Zigbee/uart.h"
#include "../BLE/ble_mesh.h"

#include "./json_parser/json_parser.h"
#include "./map/map.h"
#include "linkButton.h"

static const char *TAG = "LINK_BUTTON";

extern map_str_t map;

QueueHandle_t queueIdButton;
QueueHandle_t queueCmdZigbee;
QueueHandle_t queueCmdBLE;
QueueHandle_t queueGetStateOnoffBLE;

void linkButtonInit(void) {
	queueIdButton = xQueueCreate(100, sizeof(linkButton_t));
	queueCmdZigbee = xQueueCreate(50, sizeof(linkButton_t));
	queueCmdBLE = xQueueCreate(50, sizeof(linkButton_t));
	queueGetStateOnoffBLE = xQueueCreate(50, sizeof(uint8_t));

	xTaskCreate(sendIdButton, "sendIdButton", 4096, NULL, 199, NULL);
	xTaskCreate(sendCommandZigbee, "sendCommandZigbee", 4096, NULL, 199, NULL);
	xTaskCreate(sendCommandBLE, "sendCommandBLE", 4096, NULL, 199, NULL);
}

map_str_t json_parsing(char *textBuf) {
	map_str_t map;
	map_init(&map);
	jparse_ctx_t jctx;

	int ret = json_parse_start(&jctx, textBuf, strlen(textBuf));
	if (ret != OS_SUCCESS) {
		printf("Parser failed\n");
	}
	char node_id[20];
	char protocol[10];
	char endpoint[5];
	int num_elem_obj_arr;
	int num_elem_arr;
	if (json_obj_get_array(&jctx, "link", &num_elem_obj_arr) == OS_SUCCESS) {
		for (uint32_t i = 0; i < num_elem_obj_arr; i++) {
			if (json_arr_get_array(&jctx, i, &num_elem_arr) == OS_SUCCESS) {
				for (uint32_t j = 0; j < num_elem_arr; j++) {
					if (json_arr_get_object(&jctx, j) == OS_SUCCESS) {
						if (json_obj_get_string(&jctx, "node_id", node_id,
								sizeof(node_id)) == OS_SUCCESS) {
						}
						if (json_obj_get_string(&jctx, "protocol", protocol,
								sizeof(protocol)) == OS_SUCCESS) {
						}
						if (json_obj_get_string(&jctx, "endpoint", endpoint,
								sizeof(endpoint)) == OS_SUCCESS) {
						}
					}
					json_arr_leave_object(&jctx);
					char key[40];
					sprintf(key, "%s%s%s", node_id, protocol, endpoint);
					char value[256] = "";
					for (uint32_t k = 0; k < num_elem_arr; k++) {
						if (json_arr_get_object(&jctx, k) == OS_SUCCESS
								&& k != j) {
							if (json_obj_get_string(&jctx, "node_id", node_id,
									sizeof(node_id)) == OS_SUCCESS) {
							}
							if (json_obj_get_string(&jctx, "protocol", protocol,
									sizeof(protocol)) == OS_SUCCESS) {
							}
							if (json_obj_get_string(&jctx, "endpoint", endpoint,
									sizeof(endpoint)) == OS_SUCCESS) {
							}
							char ID[40];
							sprintf(ID, "%s%s%s ", node_id, protocol, endpoint);
							strcat(value, ID);
						}
						json_arr_leave_object(&jctx);
					}
					map_set_((map_base_t*) &map, key, value, strlen(value) + 1);
				}
				json_arr_leave_array(&jctx);
			}
		}
		json_obj_leave_array(&jctx);
	}
	json_parse_end(&jctx);
	return map;
}

void sendIdButton(void *arg) {
	linkButton_t item;
	while (1) {
		if (xQueueReceive(queueIdButton, &item, portMAX_DELAY) == pdPASS) {
			printf("------------------sendid\r\n");
			char *idButton = map_get(&map, item.idButton);
			if (idButton) {
				char *idButton_cpy = malloc(strlen(idButton) * sizeof(char) + 1);
				memcpy(idButton_cpy, idButton, strlen(idButton) + 1);
				printf("---idButton: %s\r\n", idButton_cpy);
				char *token = strtok(idButton_cpy, " ");
				while (token != NULL) {
					memcpy(item.idButton, token, strlen(token) + 1);
					UBaseType_t res;
					if (strstr(token, "ble") == NULL) {
						res = xQueueSend(queueCmdZigbee, (void* ) &item, (TickType_t )10);
						if (res != pdTRUE) {
							ESP_LOGE(TAG, "Failed to send item\n");
						}
					} else {
						res = xQueueSend(queueCmdBLE, (void* ) &item, (TickType_t )10);
						if (res != pdTRUE) {
							ESP_LOGE(TAG, "Failed to send item\n");
						}
					}
					token = strtok(NULL, " ");
				}
				free(idButton_cpy);
			}
		}
		vTaskDelay(50 / portTICK_RATE_MS);
	}
}

void sendCommandZigbee(void *arg) {
	linkButton_t item;
	while (1) {
		if (xQueueReceive(queueCmdZigbee, &item, portMAX_DELAY) == pdPASS) {
			char msg[100] = {'\0'};
			char node_id[20] = {'\0'};
			char endpoint;
			memset(&node_id, 0, sizeof(node_id));
			memcpy(node_id, item.idButton, 6);
			endpoint = item.idButton[strlen(item.idButton) - 1];
			sprintf(msg, "custom action-onoff %s %s %c\n", item.stateButton, node_id, endpoint);
			uart_write_bytes(UART_NUM_1, msg, strlen(msg));
			ESP_LOGV(TAG, "%s", msg);
		}
		vTaskDelay(50 / portTICK_RATE_MS);
	}
}

void sendCommandBLE(void *arg) {
	linkButton_t item;
	while (1) {
		if (xQueueReceive(queueCmdBLE, &item, portMAX_DELAY) == pdPASS) {
			printf("-----------sendCommandBLE: %s\r\n", item);
			char node_id[7] = { '\0' };
			char endpoint;
			memcpy(node_id, item.idButton, 7);
			endpoint = item.idButton[strlen(item.idButton) - 1];
			uint8_t state = (strcmp(item.stateButton, "off") == 0) ? 0 : 1;
			ble_mesh_command_set(ONOFF, (uint16_t) mParseHex(node_id, 6), (endpoint - '0') - 1, state, 1);
		}
		vTaskDelay(300 / portTICK_RATE_MS);
	}
}
