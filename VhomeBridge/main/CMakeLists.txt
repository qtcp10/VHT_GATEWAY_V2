# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c"
					"../main/Pair/QuickMode/SmartConfig.c"
					"../main/Button/Button.c"
					"../main/Pair/HttpServer/WebServer.c"
					"../main/jsonUser/json_user.c"
					"../main/SPIFFS/spiffs_user.c"
					"../main/WiFi/WiFi_proc.c"
					"../main/Pair/CompatibleMode/AP.c"
					"../main/Mqtt/mqtt.c"
					"../main/BLE/ble_mesh.c"
					"../main/OTA/fota.c"
					"../main/webSocket/websocket_server.c"
					"../main/webSocket/websocket.c"
					"../main/webSocket/websocketAPP.c"
					"../main/LED/led.c"
					"../main/Zigbee/uart.c"
					"../main/Zigbee/controller.c"
					"../main/Zigbee/fota.c"
					"../main/linkNode/json_parser/json_parser.c" 
		            "../main/linkNode/map/map.c"
		            "../main/linkNode/linkButton.c"
		            "../main/manager/manager.c"
					"common.c")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
