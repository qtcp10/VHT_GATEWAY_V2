

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include "websocket.h"

#define WEBSOCKET_SERVER_MAX_CLIENTS		10
#define WEBSOCKET_SERVER_QUEUE_SIZE 		1024
#define WEBSOCKET_SERVER_QUEUE_TIMEOUT 		2000
#define WEBSOCKET_SERVER_TASK_STACK_DEPTH 	8096
#define WEBSOCKET_SERVER_TASK_PRIORITY 		3
#define WEBSOCKET_SERVER_PINNED 			0
#if WEBSOCKET_SERVER_PINNED
#define WEBSOCKET_SERVER_PINNED_CORE CONFIG_WEBSOCKET_SERVER_PINNED_CORE
#endif

// starts the server
int ws_server_start();

// ends the server
int ws_server_stop();

// adds a client, returns the client's number in the server
int ws_server_add_client(struct netconn* conn,
		char* msg,
		uint16_t len,
		char* url,
		void (*callback)(uint8_t num,
				WEBSOCKET_TYPE_t type,
				char* msg,
				uint64_t len));

int ws_server_add_client_protocol(struct netconn* conn,
		char* msg,
		uint16_t len,
		char* url,
		char* protocol,
		void (*callback)(uint8_t num,
				WEBSOCKET_TYPE_t type,
				char* msg,
				uint64_t len));

int ws_server_len_url(char* url); // returns the number of connected clients to url
int ws_server_len_all(); // returns the total number of connected clients

int ws_server_remove_client(int num); // removes the client with the set number
int ws_server_remove_clients(char* url); // removes all clients connected to the specified url
int ws_server_remove_all(); // removes all clients from the server

int ws_server_send_text_client(int num,char* msg,uint64_t len); // send text to client with the set number
int ws_server_send_text_clients(char* url,char* msg,uint64_t len); // sends text to all clients with the set number
int ws_server_send_text_all(char* msg,uint64_t len); // sends text to all clients

int ws_server_send_bin_client(int num,char* msg,uint64_t len);
int ws_server_send_bin_clients(char* url,char* msg,uint64_t len);
int ws_server_send_bin_all(char* msg,uint64_t len);

// these versions can be sent from the callback ONLY

int ws_server_send_text_client_from_callback(int num,char* msg,uint64_t len); // send text to client with the set number
int ws_server_send_text_clients_from_callback(char* url,char* msg,uint64_t len); // sends text to all clients with the set number
int ws_server_send_text_all_from_callback(char* msg,uint64_t len); // sends text to all clients

int ws_server_send_bin_client_from_callback(int num,char* msg,uint64_t len); //sends binary to client with the set number
int ws_server_send_bin_clients_from_callback(char* url,char* msg,uint64_t len); // sends binary to all clients with the set number
int ws_server_send_bin_all_from_callback(char* msg,uint64_t len); // sends binary to all clients

int ws_server_ping(); // sends a ping to all connected clients

#endif

#ifdef __cplusplus
}
#endif
