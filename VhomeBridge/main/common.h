/*
 * Common.h
 *
 *  Created on: Jun 15, 2022
 *      Author: ASUS
 */

#ifndef MAIN_COMMON_H_
#define MAIN_COMMON_H_

#include <stdio.h>

#include "./SPIFFS/spiffs_user.h"

#define BROKER 						"mqtt://mqtt.innoway.vn:1883"
#define VERSION 					"0.5.3"

//#define KIT_TEST

typedef struct _dvinfor
{
	char id[50];
	char token[50];
}Device;

enum system_state_t {
	UNKNOW,
	NORMAL,
	WIFI_DISCONNECTED,
	LOCAL_MODE,
	QUICK_MODE,
	COMPATIBLE_MODE
};

typedef struct _cmd
{
	char action[50];
	char nodeID[10]; 	// 10
	char EUI64[18];		// 18
	uint8_t endpoint;
	char value[20];
	char url[100];
	char protocol[10];	// 10
}cmd;

typedef struct _action
{
	char action[50];
	char fwVersion[20];
}action;

void get_device_infor(Device * _device, char * brokerInfor);

uint32_t mParseHex(char *data, size_t max_len);

#endif /* MAIN_COMMON_H_ */
