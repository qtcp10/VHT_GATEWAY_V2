/*
 * manager.c
 *
 *  Created on: Sep 5, 2022
 *      Author: pc
 */

#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

#include "../linkNode/linkButton.h"
#include "manager.h"

static const char *TAG = "MANAGER";

extern bool LINK_BUTTON;

QueueHandle_t queueStatus;
QueueHandle_t queue_mqtt_handle;

void manage_init(void) {
	queue_mqtt_handle = xQueueCreate(50, sizeof(char));

}

void manage_sync_status(void *arg) {

	while (1) {

	}
}
