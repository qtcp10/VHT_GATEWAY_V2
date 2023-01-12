/*
 * led.c
 *
 *  Created on: Jun 29, 2022
 *      Author: Thanh Vu
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "led.h"
#include "../common.h"

extern enum system_state_t STATE;
bool data_avail = false;
static const char * TAG = "LED";

void led_init(void){
	gpio_config_t io_conf = {};
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set,e.g.GPIO18/19
	io_conf.pin_bit_mask = LED_PIN_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

	gpio_set_level(LED_SIGNAL, LED_OFF);
	ESP_LOGI(TAG, "LED initialized");

}

void led_signal_task(void *arg){
	while(1){
		if(data_avail == true){
			data_avail = false;
			gpio_set_level(LED_SIGNAL, LED_ON);
			vTaskDelay(100/portTICK_RATE_MS);
			gpio_set_level(LED_SIGNAL, LED_OFF);
		} else {
			vTaskDelay(100/portTICK_RATE_MS);
		}
	}
}

void led_status_task(void *arg){

	while(1){
		switch (STATE) {
		case UNKNOW:
			gpio_set_level(LED_STATUS, LED_OFF);
			vTaskDelay(1000/portTICK_RATE_MS);
			break;
		case NORMAL:
			gpio_set_level(LED_STATUS, LED_ON);
			vTaskDelay(1000/portTICK_RATE_MS);
			break;
		case WIFI_DISCONNECTED:
			gpio_set_level(LED_STATUS, LED_ON);
			vTaskDelay(100/portTICK_RATE_MS);
			gpio_set_level(LED_STATUS, LED_OFF);
			vTaskDelay(900/portTICK_RATE_MS);
			break;
		case LOCAL_MODE:
			gpio_set_level(LED_STATUS, LED_ON);
			vTaskDelay(100/portTICK_RATE_MS);
			gpio_set_level(LED_STATUS, LED_OFF);
			vTaskDelay(100/portTICK_RATE_MS);
			gpio_set_level(LED_STATUS, LED_ON);
			vTaskDelay(100/portTICK_RATE_MS);
			gpio_set_level(LED_STATUS, LED_OFF);
			vTaskDelay(900/portTICK_RATE_MS);
			break;
		case QUICK_MODE:
			gpio_set_level(LED_STATUS, LED_ON);
			vTaskDelay(100/portTICK_RATE_MS);
			gpio_set_level(LED_STATUS, LED_OFF);
			vTaskDelay(100/portTICK_RATE_MS);
			break;
		case COMPATIBLE_MODE:
			gpio_set_level(LED_STATUS, LED_ON);
			vTaskDelay(1000/portTICK_RATE_MS);
			gpio_set_level(LED_STATUS, LED_OFF);
			vTaskDelay(1000/portTICK_RATE_MS);
			break;
		default:
			break;
		}
	}
}

