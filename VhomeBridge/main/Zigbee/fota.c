/*
 * fota.c
 *
 *  Created on: Jul 7, 2022
 *      Author: Thanh Vu
 */

#ifndef MAIN_ZIGBEE_FOTA_C_
#define MAIN_ZIGBEE_FOTA_C_

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "driver/uart.h"
#include "fota.h"
#include "controller.h"

static const char *TAG = "ZIGBEE_FOTA";
static const char *xmodem_url = "http://hubzigbee.s3.amazonaws.com/ZigbeeMinimalSocV0.0.2.gbl";
static char upgrade_data_buf[ZIGBEE_OTA_BUF_SIZE + 1];

//extern RingbufHandle_t uart_tx_buf_handle;
extern bool xmodem_ack;


unsigned short crc16_ccitt(
		/* Pointer to the byte buffer */
		const unsigned char *buffer,
		/* length of the byte buffer */
		int length)
{
	unsigned short crc16 = 0;
	while(length != 0) {
		crc16  = (unsigned char)(crc16 >> 8) | (crc16 << 8);
		crc16 ^= *buffer;
		crc16 ^= (unsigned char)(crc16 & 0xff) >> 4;
		crc16 ^= (crc16 << 8) << 4;
		crc16 ^= ((crc16 & 0xff) << 4) << 1;
		buffer++;
		length--;
	}

	return crc16;
}

void XmodemTransmit(void *ctx, int srcsz, char header){
	unsigned char xbuff[XBUF_SIZE];
	static unsigned char packetno = 1;
	int c = srcsz;

	xbuff[0] = header;
	xbuff[1] = packetno;
	xbuff[2] = ~packetno;
	packetno++;
	if (c > XMODEM_SIZE) c = XMODEM_SIZE;
	if (c >= 0) {
		memset (&xbuff[3], 0, XMODEM_SIZE);
		if(header == SOH) memcpy (&xbuff[3], &((unsigned char *)ctx)[0], c);
		// gen crc
		unsigned short ccrc = crc16_ccitt(&xbuff[3], XMODEM_SIZE);
		xbuff[XMODEM_SIZE+3] = (ccrc>>8) & 0xFF;
		xbuff[XMODEM_SIZE+4] = ccrc & 0xFF;
		// send
		uart_write_bytes(UART_NUM_1, xbuff, XMODEM_SIZE + 5);
		//xRingbufferSendFromISR(uart_tx_buf_handle, xbuff, XMODEM_SIZE + 5, (BaseType_t *)10);
	}
}

void zigbee_fota_task(void *arg){
	esp_err_t err;
	esp_http_client_config_t config = {
			.url = xmodem_url,
			.timeout_ms = 5000,
			.keep_alive_enable = true,
			config.skip_cert_common_name_check = true,
	};
	ESP_LOGI(TAG, "zigbee_fota_task");
	esp_http_client_handle_t http_client = esp_http_client_init(&config);
	if (http_client == NULL) {
		ESP_LOGE(TAG, "Failed to initialise HTTP connection");
		return;
	}
	err = esp_http_client_open(http_client, 0);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
		esp_http_client_cleanup(http_client);
		return;
	}
	int content_length = esp_http_client_fetch_headers(http_client);

	if (content_length <= 0) {
		ESP_LOGE(TAG, "No content found in the image");
		goto END;
	}
	int image_data_remaining = content_length;
	FILE* file = fopen(ZIGBEE_FOTA_FILE, "wb");
	if(file == NULL) {
		ESP_LOGE(TAG, "Cannot open file %s", ZIGBEE_FOTA_FILE);
		goto END;
	}

	// download file from server
	while (image_data_remaining != 0) {
		int data_max_len;
		if (image_data_remaining < ZIGBEE_OTA_BUF_SIZE) {
			data_max_len = image_data_remaining;
		} else {
			data_max_len = ZIGBEE_OTA_BUF_SIZE;
		}
		int data_read = esp_http_client_read(http_client, upgrade_data_buf, data_max_len);
		if (data_read == 0) {
			if (errno == ECONNRESET || errno == ENOTCONN) {
				ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
				goto END;
			}
		} else if (data_read < 0) {
			ESP_LOGE(TAG, "Error: SSL data read error");
			goto END;
		} else if (data_read > 0) {
			ESP_LOGI(TAG, "esp_http_client_read length = %d", data_read);
			fwrite(upgrade_data_buf, 1, data_read, file );
			image_data_remaining -= data_read;
			ESP_LOGI(TAG, "image_data_remaining = %d", image_data_remaining);
		}
		vTaskDelay(100/portTICK_RATE_MS);
	}

	if(file != NULL) fclose(file);
	file = fopen(ZIGBEE_FOTA_FILE, "r");
	if(file == NULL) {
		ESP_LOGE(TAG, "Cannot open file %s", ZIGBEE_FOTA_FILE);
		goto END;
	}

	// boot_mode
//	xRingbufferSendFromISR(uart_tx_buf_handle, "1\n", 2, (BaseType_t *)10);
	uart_write_bytes(UART_NUM_1, "1\n", 2);
	vTaskDelay(500/portTICK_RATE_MS);

	// send data via xmodem protocol
	image_data_remaining = content_length;
	while(image_data_remaining != 0){
		int data_read = fread(upgrade_data_buf, 1, ZIGBEE_OTA_BUF_SIZE, file);
		if(data_read <= 0) break;
		int data_send = 0;
		while(1){
			if(data_send < data_read - XMODEM_SIZE){
				XmodemTransmit(&upgrade_data_buf[data_send], XMODEM_SIZE, SOH);
				while(!xmodem_ack);
				data_send += XMODEM_SIZE;
				xmodem_ack = false;
			} else {
				XmodemTransmit(&upgrade_data_buf[data_send], data_read - data_send, 0x01);
				while(!xmodem_ack);
				xmodem_ack = false;
				break;
			}
		}
		image_data_remaining -= data_read;
		ESP_LOGI(TAG, "image_data_remaining = %d", image_data_remaining);
	}
	XmodemTransmit(upgrade_data_buf, 0, EOT);
	vTaskDelay(100/portTICK_RATE_MS);
	if(file != NULL) fclose(file);
	END:
	if(http_client != NULL){
		esp_http_client_close(http_client);
		esp_http_client_cleanup(http_client);
	}
	zigbee_reset();
	esp_restart();
//	vTaskDelete(NULL);
}
#endif /* MAIN_ZIGBEE_FOTA_C_ */
