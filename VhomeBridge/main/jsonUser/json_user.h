/*
 * json_user.h
 *
 *  Created on: Jun 14, 2022
 *      Author: ASUS
 */

#ifndef MAIN_JSONUSER_JSON_USER_H_
#define MAIN_JSONUSER_JSON_USER_H_
#include "cJSON.h"
#include "stdint.h"
#include "string.h"
#include "stdlib.h"
#include "../common.h"

void JSON_parser(char* my_json_string, char * key, void * value);
void JSON_analyze_post(char* my_json_string, char * deviceid, char * devicetoken, char * brokerInfor);
void JSON_analyze_SUB_MQTT(char* my_json_string, cmd * sub_cmd);
void JSON_analyze_SUB_ZIGBEE(char* my_json_string, action * sub_action, cmd * sub_cmd);

#endif /* MAIN_JSONUSER_JSON_USER_H_ */
