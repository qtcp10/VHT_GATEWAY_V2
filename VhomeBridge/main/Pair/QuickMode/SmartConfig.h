/*
 * SmartConfig.h
 *
 *  Created on: Jun 12, 2022
 *      Author: ASUS
 */

#ifndef MAIN_PAIR_QUICKMODE_SMARTCONFIG_H_
#define MAIN_PAIR_QUICKMODE_SMARTCONFIG_H_

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
typedef struct _SC
{
	uint8_t ssid[33];
	uint8_t password[65];
	bool smartConfigDone;
}SC;

void start_smartconfig(void);


#endif /* MAIN_PAIR_QUICKMODE_SMARTCONFIG_H_ */
