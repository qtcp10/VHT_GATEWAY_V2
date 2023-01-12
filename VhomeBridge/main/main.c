//#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/ringbuf.h"
#include "freertos/queue.h"
#include "driver/uart.h"

#include "./BLE/ble_mesh.h"
#include "./Button/Button.h"
#include "./Pair/QuickMode/SmartConfig.h"
#include "./Pair/HttpServer/WebServer.h"
#include "./jsonUser/json_user.h"
#include "./SPIFFS/spiffs_user.h"
#include "./WiFi/WiFi_proc.h"
#include "./Pair/CompatibleMode/AP.h"
#include "./Mqtt/mqtt.h"
#include "./webSocket/webSocketAPP.h"
#include "./LED/led.h"
#include "common.h"
#include "./Zigbee/uart.h"
#include "./Zigbee/controller.h"
#include "./Zigbee/fota.h"
#include "./linkNode/map/map.h"
#include "linkNode/linkButton.h"
#include "./manager/manager.h"
bool LINK_BUTTON = true;

map_str_t map;

static const char *TAG = "MAIN";
Device Device_Infor;
char brokerInfor[100] = "";

enum system_state_t STATE = UNKNOW;

char topic_msg[100] = {'\0'};
char topic_cmd_set[100] = {'\0'};
char topic_bledevicejoined[100] = {'\0'};
char topic_bledeviceaction[100] = {'\0'};
char topic_devicejoined[100] = {'\0'};
char topic_deviceaction[100] = {'\0'};
char topic_deviceleft[100] = {'\0'};
char topic_devicerejoin[100] = {'\0'};
char topic_battery[100] = {'\0'};
char topic_linkButton[100] = {'\0'};

char fwVersion[50] = {'\0'};
char svalue[200] = {'\0'};

__NOINIT_ATTR bool Flag_quick_pair;
__NOINIT_ATTR bool Flag_compatible_pair;

TaskHandle_t uart_rx_task_handle = NULL;

void app_main(void)
{
	esp_err_t err;

	printf("--------------%d\r\n",Flag_quick_pair);
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
	init_wifi();
	led_init();
	xTaskCreate(led_signal_task, "led_signal_task", 1024, NULL, 200, NULL);
	xTaskCreate(led_status_task, "led_status_task", 1024, NULL, 200, NULL);
	xTaskCreate(button_task, "button_task", 4096, NULL, 200, NULL);

	mountSPIFFS();
	get_device_infor(&Device_Infor, brokerInfor);
	if (strlen(brokerInfor) == 0) {
		memcpy(brokerInfor, BROKER, strlen(BROKER) + 1);
	}
	ESP_LOGI(TAG, "BROKER: %s, ID: %s, TOK: %s", brokerInfor, Device_Infor.id, Device_Infor.token);

	removeFile(ZIGBEE_FOTA_FILE);

	char *buf = malloc(1024*sizeof(char));
	if (buf != NULL) {
		if (readfromfile("link_node", buf) == ESP_OK) {
			map_init(&map);
			map = json_parsing(buf);
		}
		free(buf);
	}

	sprintf(topic_cmd_set, "ont2mqtt/%s/commands/set", Device_Infor.id);
	sprintf(topic_bledevicejoined, "ont2mqtt/%s/bledevicejoined", Device_Infor.id);
	sprintf(topic_bledeviceaction, "ont2mqtt/%s/bledeviceaction", Device_Infor.id);
	sprintf(topic_devicejoined, "ont2mqtt/%s/devicejoined", Device_Infor.id);
	sprintf(topic_deviceaction, "ont2mqtt/%s/deviceaction", Device_Infor.id);
	sprintf(topic_deviceleft, "ont2mqtt/%s/deviceleft", Device_Infor.id);
	sprintf(topic_devicerejoin, "ont2mqtt/%s/devicerejoin", Device_Infor.id);
	sprintf(topic_battery, "ont2mqtt/%s/battery", Device_Infor.id);
	sprintf(topic_msg, "messages/%s/attribute", Device_Infor.id);
	sprintf(fwVersion, "{\"fwVersion\":\"%s\"}", VERSION);
	sprintf(topic_linkButton, "ont2mqtt/%s/linkButton", Device_Infor.id);

	if( esp_reset_reason() == ESP_RST_UNKNOWN || esp_reset_reason() == ESP_RST_POWERON)
	{
		Flag_quick_pair = false;
		Flag_compatible_pair = false;

	}
	if (Flag_quick_pair)
	{
		start_smartconfig();
		STATE = QUICK_MODE;
		ESP_LOGI(TAG, "STATE = QUICK_MODE");
	}
	else if (Flag_compatible_pair)
	{
		wifi_init_softap();
		STATE = COMPATIBLE_MODE;
		ESP_LOGI(TAG, "STATE = COMPETIBLE_MODE");
	}
	else if (Flag_quick_pair == false && Flag_compatible_pair == false)
	{
		ble_init();
		uart_init();
		zigbee_controller_init();
		xTaskCreate(uart_rx_task, "uart_rx_task", 4096, NULL, 200, &uart_rx_task_handle);
		zigbee_reset();

		if (LINK_BUTTON) linkButtonInit();
		manage_init();
		char * uart_tx_data = "custom action-start-network\n";
		uart_write_bytes(UART_NUM_1, uart_tx_data, strlen(uart_tx_data));

		wifi_config_t wifi_config = {
			.sta = {
				.threshold.authmode = WIFI_AUTH_WPA2_PSK,
				.pmf_cfg = {
					.capable = true,
					.required = false
				},
			},
		};
		if (esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config) == ESP_OK)
		{
			ESP_LOGI(TAG, "Wifi configuration already stored in flash partition called NVS");
			ESP_LOGI(TAG, "%s" ,wifi_config.sta.ssid);
			ESP_LOGI(TAG, "%s" ,wifi_config.sta.password);
			wifi_init_sta(wifi_config, WIFI_MODE_STA);
			mqtt_app_start(brokerInfor, Device_Infor.id, Device_Infor.token);
			ESP_LOGI("wifi", "free Heap:%d,%d", esp_get_free_heap_size(), heap_caps_get_free_size(MALLOC_CAP_8BIT));
		}

	}
}

