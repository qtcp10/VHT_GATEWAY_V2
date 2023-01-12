/*
 * uart.c
 *
 *  Created on: Jun 29, 2022
 *      Author: Thanh Vu
 */
#include "../Zigbee/uart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "../jsonUser/json_user.h"
#include "mqtt_client.h"
#include "../common.h"
#include "../linkNode/linkButton.h"

extern bool LINK_BUTTON;

static const char *TAG = "UART";

char uart_rx_data[RX_BUF_SIZE] = {'\0'};

extern char topic_devicejoined[100];
extern char topic_deviceaction[100];
extern char topic_deviceleft[100];
extern char topic_devicerejoin[100];
extern char topic_msg[100];
extern char fwVersion[50];

extern QueueHandle_t queueIdButton;

extern bool data_avail;
extern esp_mqtt_client_handle_t client;
extern enum system_state_t STATE;

bool xmodem_ack = false;

void uart_init(){
	const uart_config_t uart_config = {
			.baud_rate 	= 115200,
			.data_bits 	= UART_DATA_8_BITS,
			.parity 	= UART_PARITY_DISABLE,
			.stop_bits 	= UART_STOP_BITS_1,
			.flow_ctrl 	= UART_HW_FLOWCTRL_DISABLE,
			.source_clk = UART_SCLK_APB,
	};
	// We won't use a buffer for sending data.
	uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
	uart_param_config(UART_NUM_1, &uart_config);
	uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

//	ESP_LOGI(TAG, "UART initialized");
//	char data[100] = {'\0'};
//	sprintf(data, "custom action-leave-network\n");
//	uart_write_bytes(UART_NUM_1, data, strlen(data));
}

void uart_rx_task(void *arg)
{
	ESP_LOGI(TAG, "uart_rx_task");
	char data[RX_BUF_SIZE] = {'\0'};
	int i = 0, j = 0;
	action action;
	cmd command_set;
	while (1) {
		int rxBytes = uart_read_bytes(UART_NUM_1, uart_rx_data, RX_BUF_SIZE, 10/portTICK_RATE_MS);
		if (rxBytes > 0) {
			data_avail = true;
			uart_rx_data[rxBytes] = '\0';
			for(i = 0; i < rxBytes; i++){
				if(uart_rx_data[i] == 0x0A){
					data[j] = '\0';
					memset(&action, 0, sizeof(action));
					JSON_analyze_SUB_ZIGBEE(data, &action, &command_set);

					if((strcmp(action.action, "reportAttrs") == 0) || (strcmp(action.action, "readAttrsRes") == 0) || (strcmp(action.action, "writeAttrsRes") == 0) || (strcmp(action.action, "readAttrs") == 0)){
						if(STATE == NORMAL) esp_mqtt_client_publish(client, topic_deviceaction, data, strlen(data), 0, 0);
						if (LINK_BUTTON) {
							linkButton_t dataLinkButton;
							memcpy(dataLinkButton.stateButton, command_set.value, strlen(command_set.value) + 1);
							sprintf(dataLinkButton.idButton, "%szigbee%d", command_set.nodeID, command_set.endpoint);
							UBaseType_t res = xQueueSend(queueIdButton, (void *) &dataLinkButton, (TickType_t)10);
							if (res != pdTRUE) {
								ESP_LOGE(TAG, "Failed to send item\n");
							}
						}
					}
					if((strcmp(action.action, "join") == 0)){
						if(STATE == NORMAL) esp_mqtt_client_publish(client, topic_devicejoined, data, strlen(data), 0, 0);
					}
					if((strcmp(action.action, "rejoin") == 0)){
						if(STATE == NORMAL) esp_mqtt_client_publish(client, topic_devicerejoin, data, strlen(data), 0, 0);
					}
					if((strcmp(action.action, "left") == 0)){
						if(STATE == NORMAL) esp_mqtt_client_publish(client, topic_deviceleft, data, strlen(data), 0, 0);
					}
					if((strcmp(action.action, "mcuInfo") == 0)){
						sprintf(fwVersion, "{\"fwVersion\":\"%s\",\"zbFwVersion\":\"%s\"}", VERSION, action.fwVersion);
						if(STATE == NORMAL) esp_mqtt_client_publish(client, topic_msg, fwVersion, strlen(fwVersion), 0, 0);
					}
					j = 0;
				} else {
					data[j++] = uart_rx_data[i];
					if(j >= RX_BUF_SIZE) j = 0;
				}

			}
			if(rxBytes == 1){
				if(uart_rx_data[0] == 0x06)  xmodem_ack = true;
				else xmodem_ack = false;
				ESP_LOGI(TAG, "Read %d bytes: 0x%02X", rxBytes, uart_rx_data[0]);
			} else {
				ESP_LOGI(TAG, "Read %d bytes: %s", rxBytes, uart_rx_data);
			}

		}
	}
}
