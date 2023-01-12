/*
 * fota.h
 *
 *  Created on: Jul 7, 2022
 *      Author: Thanh Vu
 */

#ifndef MAIN_ZIGBEE_FOTA_H_
#define MAIN_ZIGBEE_FOTA_H_

/* 128 for XModem + 3 head chars + 2 crc */
#define XMODEM_SIZE 			128
#define XBUF_SIZE 				(XMODEM_SIZE + 3 + 2)

#define ZIGBEE_OTA_BUF_SIZE 	16384
#define ZIGBEE_FOTA_FILE 		"/spiffs/zigbee.ota"

#define SOH   0x01
#define STX   0x02
#define EOT   0x04
#define ACK   0x06
#define NAK   0x15
#define CAN   0x18
#define CTRLZ 0x1A

void zigbee_fota_task(void *arg);

#endif /* MAIN_ZIGBEE_FOTA_H_ */
