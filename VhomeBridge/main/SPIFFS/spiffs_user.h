/*
 * SPIFFS_user.h
 *
 *  Created on: Sep 28, 2021
 *      Author: HAOHV6
 */

#ifndef MYLIB_SPIFFS_USER_H_
#define MYLIB_SPIFFS_USER_H_

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs.h"
#include "nvs_flash.h"


esp_err_t writetofile(char* FileName,char* textbuffer);
esp_err_t readfromfile(char* FileName,char* textbuffer);
void mountSPIFFS();
void removeFile(char * Filename);

#endif /* MYLIB_SPIFFS_USER_H_ */
