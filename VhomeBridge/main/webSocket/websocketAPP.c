

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "lwip/api.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/ledc.h"
#include "string.h"
#include "websocket_server.h"
#include "websocketAPP.h"
#include "freertos/ringbuf.h"
#define AP_SSID "esp32"
#define AP_PSSWD "12345678"
struct netconn *conn;
static QueueHandle_t client_queue;
const static int client_queue_size = 10;

RingbufHandle_t socket_buf_rec;
RingbufHandle_t socket_buf_send;
// handles websocket events
void websocket_callback(uint8_t num,WEBSOCKET_TYPE_t type,char* msg,uint64_t len) {
	const static char* TAG = "websocket_callback";

	switch(type) {
	case WEBSOCKET_CONNECT:
		ESP_LOGI(TAG,"client %i connected!",num);
		break;
	case WEBSOCKET_DISCONNECT_EXTERNAL:
		ESP_LOGI(TAG,"client %i sent a disconnect message",num);
		break;
	case WEBSOCKET_DISCONNECT_INTERNAL:
		ESP_LOGI(TAG,"client %i was disconnected",num);
		break;
	case WEBSOCKET_DISCONNECT_ERROR:
		ESP_LOGI(TAG,"client %i was disconnected due to an error",num);
		break;
	case WEBSOCKET_TEXT:
		if(len) { // if the message length was greater than zero
			UBaseType_t res =  xRingbufferSend(socket_buf_rec, msg, len, pdMS_TO_TICKS(1000));
			if (res != pdTRUE) {
				printf("Failed to send item\n");
			}
		}
		break;
	case WEBSOCKET_BIN:
		ESP_LOGI(TAG,"client %i sent binary message of size %i:\n%s",num,(uint32_t)len,msg);
		break;
	case WEBSOCKET_PING:
		ESP_LOGI(TAG,"client %i pinged us with message of size %i:\n%s",num,(uint32_t)len,msg);
		break;
	case WEBSOCKET_PONG:
		ESP_LOGI(TAG,"client %i responded to the ping",num);
		break;
	}
}

// serves any clients
static void http_serve(struct netconn *conn) {
	const static char* TAG = "http_server";
	struct netbuf* inbuf;
	static char* buf;
	static uint16_t buflen;
	static err_t err;


	netconn_set_recvtimeout(conn,1000); // allow a connection timeout of 1 second
	ESP_LOGI(TAG,"reading from client...");
	err = netconn_recv(conn, &inbuf);
	ESP_LOGI(TAG,"read from client");
	if(err==ERR_OK) {
		netbuf_data(inbuf, (void**)&buf, &buflen);
		if(buf) {
			// default page websocket
			if(strstr(buf,"GET / ")
					&& strstr(buf,"Upgrade: websocket")) {
				ESP_LOGI(TAG,"Requesting websocket on /");
				ws_server_add_client(conn,buf,buflen,"/",websocket_callback);
				netbuf_delete(inbuf);
			}
			else {
				ESP_LOGI(TAG,"Unknown request");
				netconn_close(conn);
				netconn_delete(conn);
				netbuf_delete(inbuf);
			}
		}
		else {
			ESP_LOGI(TAG,"Unknown request (empty?...)");
			netconn_close(conn);
			netconn_delete(conn);
			netbuf_delete(inbuf);
		}
	}
	else { // if err==ERR_OK
		ESP_LOGI(TAG,"error on read, closing connection");
		netconn_close(conn);
		netconn_delete(conn);
		netbuf_delete(inbuf);
	}
}

// handles clients when they first connect. passes to a queue
static void server_task(void* pvParameters) {
	const static char* TAG = "server_task";
	struct netconn *newconn;
	static err_t err;
	client_queue = xQueueCreate(client_queue_size,sizeof(struct netconn*));

	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn,NULL,8000);
	netconn_listen(conn);
	ESP_LOGI(TAG,"server listening");
	do {
		err = netconn_accept(conn, &newconn);
		ESP_LOGI(TAG,"new client");
		if(err == ERR_OK) {
			xQueueSendToBack(client_queue,&newconn,portMAX_DELAY);
			//http_serve(newconn);
		}
	} while(err == ERR_OK);
	//  netconn_close(conn);
	//  netconn_delete(conn);
	ESP_LOGE(TAG,"task ending");
	//  esp_restart();
	vTaskDelete(NULL);
}

// receives clients from queue, handles them
static void server_handle_task(void* pvParameters) {
	const static char* TAG = "server_handle_task";
	struct netconn* conn;
	ESP_LOGI(TAG,"task starting");
	for(;;) {
		xQueueReceive(client_queue,&conn,portMAX_DELAY);
		if(!conn) continue;
		http_serve(conn);
	}
	vTaskDelete(NULL);
}

// handle your incoming msg from socket here
void in_msg(void *arg)
{
	char * item = NULL;
	size_t item_size;
	while(1)
	{
		item = (char *)xRingbufferReceive(socket_buf_rec, &item_size, pdMS_TO_TICKS(1000));
		if(item)
		{
			item[item_size] = '\0';
			printf("SOCKET_IN_MSG: %s\r\n", item);
			//.....process incoming data here
			vRingbufferReturnItem(socket_buf_rec, (void *)item);
		}
	}
}

// send out msg socket
void out_msg(void *arg)
{
	char * item = NULL;
	size_t item_size;
	while(1)
	{
		item = (char *)xRingbufferReceive(socket_buf_send, &item_size, pdMS_TO_TICKS(1000));
		if(item)
		{
			item[item_size] = '\0';
			printf("SOCKET_OUT_MSG: %s\r\n", item);
			ws_server_send_text_all(item,item_size);
			vRingbufferReturnItem(socket_buf_send, (void *)item);
		}
	}
}
void websocket_stop()
{
	ws_server_stop();
	netconn_close(conn);
	netconn_delete(conn);

}
void websocket_start()
{
	socket_buf_rec = xRingbufferCreate(1028, RINGBUF_TYPE_NOSPLIT);
	if (socket_buf_rec == NULL) {
		printf("Failed to create ring buffer\n");
	}
	socket_buf_send = xRingbufferCreate(1028, RINGBUF_TYPE_NOSPLIT);
	if (socket_buf_send == NULL) {
		printf("Failed to create ring buffer\n");
	}
	ws_server_start();
	xTaskCreate(&out_msg,"in_msg",3000,NULL,9,NULL);
	xTaskCreate(&in_msg,"in_msg",3000,NULL,9,NULL);
	xTaskCreate(&server_task,"server_task",3000,NULL,9,NULL);
	xTaskCreate(&server_handle_task,"server_handle_task",4000,NULL,6,NULL);
}
