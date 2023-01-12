/*
 * ble_mesh.h
 *
 *  Created on: May 26, 2022
 *      Author: TuanNQ24
 */



#ifndef MAIN_BLE_MESH_H_
#define MAIN_BLE_MESH_H_

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_sensor_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_lighting_model_api.h"
#include "esp_ble_mesh_time_scene_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "mbedtls/aes.h"

#define BD_ADDR_LENGTH 		6

#define OFF             	0x0
#define ON              	0x1

#define CID_ESP             0x02E5
#define CID_TEL				0x0211

#define PROV_OWN_ADDR       0x0001

#define MSG_SEND_TTL        255
#define MSG_SEND_REL        false
#define MSG_TIMEOUT         500
#define MSG_ROLE            ROLE_PROVISIONER

#define COMP_DATA_PAGE_0    0x00

#define APP_KEY_IDX         0x0000
#define APP_KEY_OCTET       0x12

#define COMP_DATA_1_OCTET(msg, offset)      (msg[offset])
#define COMP_DATA_2_OCTET(msg, offset)      (msg[offset + 1] << 8 | msg[offset])

#define ESP_BLE_MESH_VND_MODEL_ID_CLIENT    0x0001
#define ESP_BLE_MESH_VND_MODEL_ID_SERVER    0x0000

#define ESP_BLE_MESH_VND_MODEL_OP_SEND      ESP_BLE_MESH_MODEL_OP_3(0x00, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL_OP_STATUS    ESP_BLE_MESH_MODEL_OP_3(0x01, CID_ESP)

enum command_type
{
	ONOFF = 1,
	LEVEL,
	BRIGHTNESS,
	HSL
};

struct esp_ble_mesh_key {
    uint16_t net_idx;
    uint16_t app_idx;
    uint8_t  app_key[16];
} prov_key;

typedef struct {
	uint16_t unicast_addr;
    uint8_t  MAC[BD_ADDR_LENGTH];
    uint8_t  uuid[ESP_BLE_MESH_OCTET16_LEN];
	uint16_t element_num;
	uint16_t models[8][24];
	uint16_t cid[8][24];
} node_infor;

esp_err_t add_fast_prov_group_address(uint16_t model_id, uint16_t group_addr);
void ble_mesh_set_msg_common(esp_ble_mesh_client_common_param_t *common,
                                            esp_ble_mesh_node_t *node,
                                            esp_ble_mesh_model_t *model, uint32_t opcode);
void ble_mesh_set_msg_common_element(esp_ble_mesh_client_common_param_t *common,
											uint16_t unicast_addr, uint8_t element_addr_offset,
                                            esp_ble_mesh_model_t *model, uint32_t opcode);
esp_err_t prov_complete(uint16_t node_index, const esp_ble_mesh_octet16_t uuid,
                               uint16_t primary_addr, uint8_t element_num, uint16_t net_idx);
void prov_link_open(esp_ble_mesh_prov_bearer_t bearer);
void prov_link_close(esp_ble_mesh_prov_bearer_t bearer, uint8_t reason);
void recv_unprov_adv_pkt(uint8_t dev_uuid[16], uint8_t addr[BD_ADDR_LENGTH],
                                esp_ble_mesh_addr_type_t addr_type, uint16_t oob_info,
                                uint8_t adv_type, esp_ble_mesh_prov_bearer_t bearer);
void ble_mesh_parse_node_comp_data(const uint8_t *data, uint16_t length);
void ble_mesh_send_sensor_message(uint32_t opcode);
void ble_mesh_sensor_timeout(uint32_t opcode);

esp_err_t ble_mesh_send_vendor_message(uint32_t opcode, uint16_t unicast_addr, uint16_t model_id, unsigned char len, unsigned char *data);
void ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                             esp_ble_mesh_prov_cb_param_t *param);
void ble_mesh_config_client_cb(esp_ble_mesh_cfg_client_cb_event_t event,
                                              esp_ble_mesh_cfg_client_cb_param_t *param);
void ble_mesh_generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event,
                                               esp_ble_mesh_generic_client_cb_param_t *param);
void ble_mesh_sensor_client_cb(esp_ble_mesh_sensor_client_cb_event_t event,
                                              esp_ble_mesh_sensor_client_cb_param_t *param);
void ble_mesh_custom_model_cb(esp_ble_mesh_model_cb_event_t event,
                                             esp_ble_mesh_model_cb_param_t *param);
esp_err_t ble_mesh_init(void);


esp_err_t ble_init(void);

esp_err_t ble_deinit(void);

esp_err_t ble_mesh_command_set(uint8_t command, uint16_t unicast_addr, uint16_t element_addr, int value, uint8_t resp);
esp_err_t ble_mesh_command_get(uint8_t command, uint16_t unicast_addr, uint16_t element_addr);
esp_err_t ble_mesh_node_reset(uint16_t unicast_addr);
esp_err_t ble_mesh_delete_group(uint16_t group_addr, uint16_t unicast_addr, uint16_t element_addr_offset, uint16_t model_id);

#endif /* MAIN_BLE_MESH_H_ */
