/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */

#include "WebServer.h"
#include "../CompatibleMode/AP.h"
#include "../../jsonUser/json_user.h"
#include "../../SPIFFS/spiffs_user.h"
#include "../../common.h"
#include "../../WiFi/WiFi_proc.h"
#include "esp_mac.h"

static const char *TAG = "WEB_SERVER";
extern Device Device_Infor;
extern char brokerInfor[100];
httpd_handle_t server = NULL;
bool Flag_AP_resp = false;
bool Flag_wifi = false;
char resp_str[513] = {0};
/* An HTTP GET handler */
extern __NOINIT_ATTR bool Flag_quick_pair;
extern __NOINIT_ATTR bool Flag_compatible_pair;

static esp_err_t GET_rst_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    char host[16];
    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
            memcpy(host, buf, sizeof(host));
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Content-Type") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Content-Type", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Content-Type: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "User-Agent") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "User-Agent", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => User-Agent: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
            }
            if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param);
            }
            if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param);
            }
        }
        free(buf);
    }

    /* Set some custom headers */
    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    char resp_str[513] = "{\"error\":\"0\"}";
    if(httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN))
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}
static const httpd_uri_t AP_rest = {
    .uri       = "/restart",
    .method    = HTTP_GET,
    .handler   = GET_rst_handler,
    .user_ctx  = "Hello World!"
};
static esp_err_t GET_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    char host[16];
    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
            memcpy(host, buf, sizeof(host));
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Content-Type") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Content-Type", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Content-Type: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "User-Agent") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "User-Agent", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => User-Agent: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
            }
            if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param);
            }
            if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param);
            }
        }
        free(buf);
    }

    /* Set some custom headers */
    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    uint8_t chipid[6] = {0};
    esp_read_mac(chipid, ESP_MAC_BT);
	char mac[50];
	sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", chipid[0], chipid[1], chipid[2], chipid[3], chipid[4], chipid[5]);
	printf("----------mac: %s\r\n", mac);
    char resp_str[513] = {0};
    sprintf(resp_str, "{\"ipdevice\":\"%s\",\"type\":\"2\",\"devicename\":\"SmartGateway\",\"mac\":\"%s\"}", host, mac);
    ESP_LOGD(TAG, "\r\n /device resp: %s\r\n", resp_str);
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}

static const httpd_uri_t GET_device = {
    .uri       = "/device",
    .method    = HTTP_GET,
    .handler   = GET_handler,
    .user_ctx  = "Hello World!"
};


static esp_err_t POST_handler(httpd_req_t *req)
{
	bool Flag_encode = false;
    char buf[200];
    char buff_decode[200];
    char *buf_header;
    int ret, remaining = req->content_len;
    printf("remaining: %d\r\n", remaining);
    size_t buf_len;
    buf_len = httpd_req_get_hdr_value_len(req, "Content-Type") + 1;
	if (buf_len > 1) {
		buf_header = malloc(buf_len);
		if (httpd_req_get_hdr_value_str(req, "Content-Type", buf_header, buf_len) == ESP_OK) {
			ESP_LOGI(TAG, "Found header => Content-Type: %s", buf_header);
			if(strstr(buf_header, "urlencoded")) Flag_encode = true;
		}
		free(buf_header);
	}
    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Send back the same data */
//        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        /* Log data received */
    }
	if (Flag_encode)
	{
		urldecode2(buff_decode, buf);
		ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
		ESP_LOGI(TAG, "%s", buff_decode);
		ESP_LOGI(TAG, "====================================");
		writetofile("deviceinfor", buff_decode);
		get_device_infor(&Device_Infor, brokerInfor);
		ESP_LOGI(TAG, "BROKER: %s, ID: %s, TOK: %s", brokerInfor, Device_Infor.id, Device_Infor.token);
	}
	else
	{
		ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
		ESP_LOGI(TAG, "%s", buf);
		ESP_LOGI(TAG, "====================================");
		writetofile("deviceinfor", buf);
		get_device_infor(&Device_Infor, brokerInfor);
		ESP_LOGI(TAG, "BROKER: %s, ID: %s, TOK: %s", brokerInfor, Device_Infor.id, Device_Infor.token);
	}
    // End response
//    httpd_resp_send_chunk(req, NULL, 0);
    char resp_str[20] = "{\"error\":\"0\"}";
	httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
	printf("Send respond to STA\r\n");
    Flag_quick_pair = Flag_compatible_pair = false;
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    esp_restart();
    return ESP_OK;
}

static const httpd_uri_t POST_ap = {
    .uri       = "/ap",
    .method    = HTTP_POST,
    .handler   = POST_handler,
    .user_ctx  = NULL
};

static esp_err_t AP_POST_handler(httpd_req_t *req)
{
	bool Flag_encode = false;
	char buf[100];
	char buff_decode[100];
	char * buf_header;
	int ret, remaining = req->content_len;
	printf("remaining: %d\r\n", remaining);
	wifi_config_t wifi_config = {
			.sta = {
			 .threshold.authmode = WIFI_AUTH_WPA2_PSK,
				.pmf_cfg = {
					.capable = true,
					.required = false
				},
			},
	};
	size_t buf_len;
	buf_len = httpd_req_get_hdr_value_len(req, "Content-Type") + 1;
	if (buf_len > 1) {
		buf_header = malloc(buf_len);
		if (httpd_req_get_hdr_value_str(req, "Content-Type", buf_header, buf_len) == ESP_OK) {
			ESP_LOGI(TAG, "Found header => Content-Type: %s", buf_header);
			if(strstr(buf_header, "urlencoded")) Flag_encode = true;
		}
		free(buf_header);
	}
	if(remaining > 0)
	{
		while (remaining > 0) {
			/* Read the data for the request */
			if ((ret = httpd_req_recv(req, buf,
							MIN(remaining, sizeof(buf)))) <= 0) {
				if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
					/* Retry receiving if timeout occurred */
					continue;
				}
				return ESP_FAIL;
			}
			remaining -= ret;
		}
		buf[req->content_len] = '\0';
	}
	else
	{
		printf("strlen(req->uri): %d\r\n", strlen(req->uri));
		memcpy(buf, req->uri, strlen(req->uri));
		buf[strlen(req->uri)] = '\0';
	}
	if (Flag_encode)
	{
		urldecode2(buff_decode, buf);
		ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
		ESP_LOGI(TAG, "%s", buff_decode);
		ESP_LOGI(TAG, "====================================");
		parse_wifi_uri(buff_decode,(char*) wifi_config.sta.ssid,(char*) wifi_config.sta.password);
	}
	else
	{
		ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
		ESP_LOGI(TAG, "%s", buf);
		ESP_LOGI(TAG, "====================================");
		parse_wifi_uri(buf,(char*) wifi_config.sta.ssid,(char*) wifi_config.sta.password);
	}

	esp_wifi_disconnect();
	bool res;
	res = wifi_init_sta(wifi_config, WIFI_MODE_APSTA);
	if (res)
	{
		Flag_wifi = true;
		tcpip_adapter_ip_info_t ipInfo;
		char str[256];
		tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
		sprintf(str, "" IPSTR "", IP2STR(&ipInfo.ip));
		sprintf(resp_str, "{\"ipdevice\":\"%s\",\"type\":\"2\",\"devicename\":\"Gateway\"}", str);
		printf("\r\n /device resp: %s\r\n", resp_str);
		printf("Send respond to STA\r\n");
		httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
	}
	else
	{
		char resp_str[513] = "{\"error\":\"1\"}";
		httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
	}
	return ESP_OK;
}

static const httpd_uri_t AP_POST_ap = {
    .uri       = "/wi",
    .method    = HTTP_POST,
    .handler   = AP_POST_handler,
    .user_ctx  = NULL
};
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/device", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/device URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    } else if (strcmp("/ap", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/ap URI is not available");
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

httpd_handle_t start_webserver(void)
{
	esp_err_t err;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    httpd_stop(server);
    err = httpd_start(&server, &config);
    if (err == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &GET_device);
        httpd_register_uri_handler(server, &POST_ap);
        httpd_register_uri_handler(server, &AP_POST_ap);
        httpd_register_uri_handler(server, &AP_rest);
        return server;
    }
    printf("\r\nerr: %d\r\n", err);
    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

void disconnect_handler(void* arg, esp_event_base_t event_base, // @suppress("Unused static function")
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

void connect_handler(void* arg, esp_event_base_t event_base, // @suppress("Unused static function")
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

void urldecode2(char *dst, const char *src)
{
        char a, b;
        while (*src) {
                if ((*src == '%') &&
                    ((a = src[1]) && (b = src[2])) &&
                    (isxdigit(a) && isxdigit(b))) {
                        if (a >= 'a')
                                a -= 'a'-'A';
                        if (a >= 'A')
                                a -= ('A' - 10);
                        else
                                a -= '0';
                        if (b >= 'a')
                                b -= 'a'-'A';
                        if (b >= 'A')
                                b -= ('A' - 10);
                        else
                                b -= '0';
                        *dst++ = 16*a+b;
                        src+=3;
                } else if (*src == '+') {
                        *dst++ = ' ';
                        src++;
                } else {
                        *dst++ = *src++;
                }
        }
        *dst++ = '\0';
}

void parse_wifi_uri(char * buf, char * s, char * p)
{
	char * sub_p = strstr(buf, "p1=");
	char * sub_s = strstr(buf, "s1=");
	char * sub_save = strstr(buf, "save=");
	char * index;
	if((sub_p != NULL) && (sub_s != NULL) && (sub_save != NULL))
	{
	    index = strstr(sub_p,"&");
	    if (index != NULL) {
	        int len = strlen(sub_p) - strlen(index) - 3;
	        strncpy(p, sub_p+3,len);
	        p[len] = 0;
	        printf("pass = %s\n", p);
	    }
	    index = strstr(sub_s,"&");
	    if (index != NULL) {
	        int len = strlen(sub_s) - strlen(index) - 3;
	        strncpy(s, sub_s+3, len);
	        s[len] = 0;
	        printf("ssid = %s\n", s);
	    }

	}
}
