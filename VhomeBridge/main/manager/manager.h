/*
 * manager.h
 *
 *  Created on: Sep 5, 2022
 *      Author: pc
 */

/*
 * chức năng:
 * 1. Quản lý danh sách các thiết bị đã được pair
 * 2. Quản lý danh sách các thiết bị đang trực tuyến/ngoại tuyến
 * 3. Quản lý đồng bộ trạng thái on-off của app và thiết bị
 * */
#ifndef MAIN_MANAGER_MANAGER_H_
#define MAIN_MANAGER_MANAGER_H_

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "../BLE/ble_mesh.h"
#include "../common.h"
#include "../Zigbee/uart.h"
#include "../linkNode/linkButton.h"

typedef struct {
	char protocol[10];
	char node_id[10];
	uint8_t endpoint, value;
	bool isWorking;
} device_attr;

extern QueueHandle_t queueIdButton;

void manage_init(void);
void get_device_info(void);
void app_send(void);
void device_send(void);
void manage_sync_status(void *arg);

#endif /* MAIN_MANAGER_MANAGER_H_ */
