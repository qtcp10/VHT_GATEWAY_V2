/*
 * fota.h
 *
 *  Created on: Jun 21, 2022
 *      Author: ASUS
 */

#ifndef MAIN_OTA_FOTA_H_
#define MAIN_OTA_FOTA_H_

#define URL_OTA "http://202.191.56.104:5551/uploads/SmartGateway.bin"

void esp_ota_task(void *pvParameter);


#endif /* MAIN_OTA_FOTA_H_ */
