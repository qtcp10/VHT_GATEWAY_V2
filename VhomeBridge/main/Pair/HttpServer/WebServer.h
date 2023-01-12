/*
 * WebServer.h
 *
 *  Created on: Jun 12, 2022
 *      Author: ASUS
 */

#ifndef MAIN_PAIR_HTTPSERVER_WEBSERVER_H_
#define MAIN_PAIR_HTTPSERVER_WEBSERVER_H_

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_tls_crypto.h"
#include <esp_http_server.h>

httpd_handle_t start_webserver(void);
void parse_wifi_uri(char * buf, char * s, char * p);
void urldecode2(char *dst, const char *src);
#endif /* MAIN_PAIR_HTTPSERVER_WEBSERVER_H_ */
