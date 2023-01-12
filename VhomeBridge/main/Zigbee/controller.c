/*
 * controller.c
 *
 *  Created on: Jul 7, 2022
 *      Author: Thanh Vu
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "controller.h"

static const char * TAG = "ZB_CTL";

void zigbee_controller_init(void){
	gpio_config_t io_conf = {};
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set,e.g.GPIO18/19
	io_conf.pin_bit_mask = ZIGBEE_PIN_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

	ESP_LOGI(TAG, "ZB_CTL initialized");
}

void zigbee_reset(){
	ESP_LOGI(TAG, "zigbee_reset");
	gpio_set_level(ZIGBEE_RESET, LOW);
	gpio_set_level(ZIGBEE_BOOT, HIGH);
	vTaskDelay(100/portTICK_RATE_MS);
	gpio_set_level(ZIGBEE_RESET, HIGH);
}

void zigbee_boot(){
	ESP_LOGI(TAG, "zigbee_boot");
	gpio_set_level(ZIGBEE_RESET, LOW);
	gpio_set_level(ZIGBEE_BOOT, LOW);
	vTaskDelay(100/portTICK_RATE_MS);
	gpio_set_level(ZIGBEE_RESET, HIGH);
}



