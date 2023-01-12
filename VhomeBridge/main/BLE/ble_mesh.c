/*
 * ble_mesh.c
 *
 *  Created on: May 26, 2022
 *      Author: TuanNQ24
 */

#include <stdio.h>
#include <string.h>

//#include "freertos/queue.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "ble_mesh.h"
#include "esp_bt.h"
#include "mqtt_client.h"

#include "../linkNode/linkButton.h"
#include "../common.h"

extern bool LINK_BUTTON;

static const char *TAG = "BLE";

uint8_t provision_status = 0;
extern esp_mqtt_client_handle_t client;
extern char topic_bledevicejoined[100];
extern char topic_bledeviceaction[100];
extern char svalue[200];

uint8_t stateButtonBLE = 0;

extern QueueHandle_t queueIdButton;
extern QueueHandle_t queueGetStateOnoffBLE;

uint8_t dev_uuid[16];
uint16_t server_address = ESP_BLE_MESH_ADDR_UNASSIGNED;
uint8_t  dev_uuid[ESP_BLE_MESH_OCTET16_LEN];
uint16_t sensor_prop_id;
const unsigned char aes128_key[16] = {0x56, 0x49, 0x45, 0x54, 0x54, 0x45, 0x4C, 0x5F, 0x52, 0x26, 0x44, 0x28, 0x56, 0x48, 0x54, 0x29};
const unsigned char aes128_data_header[8] = {0x24, 0x02, 0x28, 0x04, 0x28, 0x11, 0x20, 0x20};

//static esp_ble_mesh_cfg_srv_t config_server = {
//		.beacon = ESP_BLE_MESH_BEACON_DISABLED,
//		.friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
//		.default_ttl = 7,
//		/* 3 transmissions with 20ms interval */
//		.net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
//		.relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
//};

static esp_ble_mesh_cfg_srv_t config_server = {
    .beacon = ESP_BLE_MESH_BEACON_DISABLED,
#if defined(CONFIG_BLE_MESH_FRIEND)
    .friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
    .default_ttl = 7,
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
};

static const esp_ble_mesh_client_op_pair_t vnd_op_pair[] = {
    { 0xE00211, ESP_BLE_MESH_VND_MODEL_OP_SEND, ESP_BLE_MESH_VND_MODEL_OP_STATUS },
};

static esp_ble_mesh_client_t vendor_client = {
    .op_pair_size = ARRAY_SIZE(vnd_op_pair),
    .op_pair = vnd_op_pair,
};

static esp_ble_mesh_model_op_t vnd_op[] = {
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_STATUS, 2),
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_client_t config_client;
static esp_ble_mesh_client_t onoff_client;
static esp_ble_mesh_client_t level_client;
static esp_ble_mesh_client_t power_onoff_client;
static esp_ble_mesh_client_t power_level_client;
static esp_ble_mesh_client_t battery_client;
static esp_ble_mesh_client_t sensor_client;
static esp_ble_mesh_client_t light_lightness_client;
static esp_ble_mesh_client_t light_clt_client;
static esp_ble_mesh_client_t light_hsl_client;
static esp_ble_mesh_client_t light_xyl_client;

static esp_ble_mesh_model_t root_models[] = {
		ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
		ESP_BLE_MESH_MODEL_CFG_CLI(&config_client),
		ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(NULL, &onoff_client),
		ESP_BLE_MESH_MODEL_GEN_LEVEL_CLI(NULL, &level_client),
		ESP_BLE_MESH_MODEL_GEN_POWER_ONOFF_CLI(NULL, &power_onoff_client),
		ESP_BLE_MESH_MODEL_GEN_POWER_LEVEL_CLI(NULL, &power_level_client),
		ESP_BLE_MESH_MODEL_GEN_BATTERY_CLI(NULL, &battery_client),
		ESP_BLE_MESH_MODEL_SENSOR_CLI(NULL, &sensor_client),
		ESP_BLE_MESH_MODEL_LIGHT_LIGHTNESS_CLI(NULL, &light_lightness_client),
		ESP_BLE_MESH_MODEL_LIGHT_CTL_CLI(NULL, &light_clt_client),
		ESP_BLE_MESH_MODEL_LIGHT_HSL_CLI(NULL, &light_hsl_client),
		ESP_BLE_MESH_MODEL_LIGHT_XYL_CLI(NULL, &light_xyl_client),
};

static esp_ble_mesh_model_t vnd_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_CLIENT,
    vnd_op, NULL, &vendor_client),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, vnd_models),
};

static esp_ble_mesh_comp_t composition = {
		.cid = CID_ESP,
		.elements = elements,
		.element_count = ARRAY_SIZE(elements),
};

static esp_ble_mesh_prov_t provision = {
		.prov_uuid           = dev_uuid,
		.prov_unicast_addr   = PROV_OWN_ADDR,
		.prov_start_address  = 0x0005,
		.prov_attention      = 0x00,
		.prov_algorithm      = 0x00,
		.prov_pub_key_oob    = 0x00,
		.prov_static_oob_val = NULL,
		.prov_static_oob_len = 0x00,
		.flags               = 0x00,
		.iv_index            = 0x00,
};

node_infor global_node = {0};

void ble_mesh_set_msg_common(esp_ble_mesh_client_common_param_t *common,
		esp_ble_mesh_node_t *node,
		esp_ble_mesh_model_t *model, uint32_t opcode)
{
	common->opcode = opcode;
	common->model = model;
	common->ctx.net_idx = prov_key.net_idx;
	common->ctx.app_idx = prov_key.app_idx;
	common->ctx.addr = node->unicast_addr;
	common->ctx.send_ttl = MSG_SEND_TTL;
	common->ctx.send_rel = MSG_SEND_REL;
	common->msg_timeout = 2000;
	common->msg_role = MSG_ROLE;
}

void ble_mesh_set_msg_common_element(esp_ble_mesh_client_common_param_t *common,
		uint16_t unicast_addr, uint8_t element_addr_offset,
		esp_ble_mesh_model_t *model, uint32_t opcode)
{
	common->opcode = opcode;
	common->model = model;
	common->ctx.net_idx = prov_key.net_idx;
	common->ctx.app_idx = prov_key.app_idx;
	common->ctx.addr = unicast_addr + element_addr_offset;
	common->ctx.send_ttl = MSG_SEND_TTL;
	common->ctx.send_rel = MSG_SEND_REL;
	common->msg_timeout = MSG_TIMEOUT;
	common->msg_role = MSG_ROLE;
}

esp_err_t prov_complete(uint16_t node_index, const esp_ble_mesh_octet16_t uuid,
		uint16_t primary_addr, uint8_t element_num, uint16_t net_idx)
{
	provision_status = 0;
	esp_ble_mesh_client_common_param_t common = {0};
	esp_ble_mesh_cfg_client_get_state_t get = {0};
	esp_ble_mesh_node_t *node = NULL;

	esp_err_t err = ESP_OK;

	ESP_LOGI(TAG, "NODEID = 0x%04X, PRIMARY_ADDR = 0x%04X ELEMENT_NUM = %u, NET_ID = 0x%04X",
			node_index, primary_addr, element_num, net_idx);
	ESP_LOG_BUFFER_HEX("UUID", uuid, ESP_BLE_MESH_OCTET16_LEN);

	server_address = primary_addr;
	global_node.unicast_addr = primary_addr;
	memcpy(global_node.uuid, uuid, sizeof(global_node.uuid));


	char name[11] = {'\0'};
	sprintf(name, "%s%02X", "NODE", node_index);
	err = esp_ble_mesh_provisioner_set_node_name(node_index, name);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to set node name");
		return ESP_FAIL;
	}

	node = esp_ble_mesh_provisioner_get_node_with_addr(primary_addr);
	if (node == NULL) {
		ESP_LOGE(TAG, "Failed to get node 0x%04x info", primary_addr);
		return ESP_FAIL;
	}

	ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
	get.comp_data_get.page = COMP_DATA_PAGE_0;
	err = esp_ble_mesh_config_client_get_state(&common, &get);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to send Config Composition Data Get");
		return ESP_FAIL;
	}

	return ESP_OK;
}

void recv_unprov_adv_pkt(uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN], uint8_t addr[BD_ADDR_LENGTH],
		esp_ble_mesh_addr_type_t addr_type, uint16_t oob_info,
		uint8_t adv_type, esp_ble_mesh_prov_bearer_t bearer)
{
	esp_ble_mesh_unprov_dev_add_t add_dev = {0};
	esp_err_t err = ESP_OK;

	/* Due to the API esp_ble_mesh_provisioner_set_dev_uuid_match, Provisioner will only
	 * use this callback to report the devices, whose device UUID starts with 0xdd & 0xdd,
	 * to the application layer.
	 */

	ESP_LOG_BUFFER_HEX("MAC", addr, BD_ADDR_LENGTH);
	ESP_LOGI(TAG, "Address type 0x%02x, adv type 0x%02x", addr_type, adv_type);
	ESP_LOG_BUFFER_HEX("UUID", dev_uuid, ESP_BLE_MESH_OCTET16_LEN);
	ESP_LOGI(TAG, "oob info 0x%04x, bearer %s", oob_info, (bearer & ESP_BLE_MESH_PROV_ADV) ? "PB-ADV" : "PB-GATT");

	memcpy(add_dev.addr, addr, BD_ADDR_LENGTH);
	memcpy(global_node.MAC, addr, BD_ADDR_LENGTH);
	add_dev.addr_type = (uint8_t)addr_type;
	memcpy(add_dev.uuid, dev_uuid, ESP_BLE_MESH_OCTET16_LEN);
	add_dev.oob_info = oob_info;
	add_dev.bearer = (uint8_t)bearer;
	/* Note: If unprovisioned device adv packets have not been received, we should not add
             device with ADD_DEV_START_PROV_NOW_FLAG set. */
	err = esp_ble_mesh_provisioner_add_unprov_dev(&add_dev,
			ADD_DEV_RM_AFTER_PROV_FLAG | ADD_DEV_START_PROV_NOW_FLAG | ADD_DEV_FLUSHABLE_DEV_FLAG);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to start provisioning device");
	}
}

void ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
		esp_ble_mesh_prov_cb_param_t *param)
{
	switch (event) {
	case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
		ESP_LOGI(TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
		break;
	case ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT:
		ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT, err_code %d", param->provisioner_prov_enable_comp.err_code);
		break;
	case ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT:
		ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT, err_code %d", param->provisioner_prov_disable_comp.err_code);
		break;
	case ESP_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT:
		if(provision_status == 1){
			ESP_LOGW(TAG, "ESP_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT");
			recv_unprov_adv_pkt(param->provisioner_recv_unprov_adv_pkt.dev_uuid, param->provisioner_recv_unprov_adv_pkt.addr,
					param->provisioner_recv_unprov_adv_pkt.addr_type, param->provisioner_recv_unprov_adv_pkt.oob_info,
					param->provisioner_recv_unprov_adv_pkt.adv_type, param->provisioner_recv_unprov_adv_pkt.bearer);
		}
		break;
	case ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT:
		ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT, bearer %s",
				param->provisioner_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
		break;
	case ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT:
		ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT, bearer %s, reason 0x%02x",
				param->provisioner_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT", param->provisioner_prov_link_close.reason);
		break;
	case ESP_BLE_MESH_PROVISIONER_PROV_COMPLETE_EVT:
		ESP_LOGW(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_COMPLETE_EVT");
		prov_complete(param->provisioner_prov_complete.node_idx, param->provisioner_prov_complete.device_uuid,
				param->provisioner_prov_complete.unicast_addr, param->provisioner_prov_complete.element_num,
				param->provisioner_prov_complete.netkey_idx);
		break;
	case ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT:
		ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT, err_code %d", param->provisioner_add_unprov_dev_comp.err_code);
		break;
	case ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT:
		ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT, err_code %d", param->provisioner_set_dev_uuid_match_comp.err_code);
		break;
	case ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT:
		ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT, err_code %d", param->provisioner_set_node_name_comp.err_code);
		ESP_LOGI(TAG, "%s", esp_ble_mesh_provisioner_get_node_name(param->provisioner_set_node_name_comp.node_index));
		break;
	case ESP_BLE_MESH_PROVISIONER_UPDATE_LOCAL_NET_KEY_COMP_EVT:
	{
		const uint8_t *net_key = esp_ble_mesh_provisioner_get_local_net_key(prov_key.net_idx);
		ESP_LOG_BUFFER_HEX("NET_KEY", net_key, ESP_BLE_MESH_OCTET16_LEN);
		break;
	}
	case ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT:
	{
		const uint8_t * app_key = esp_ble_mesh_provisioner_get_local_app_key(prov_key.net_idx ,prov_key.app_idx);
		ESP_LOG_BUFFER_HEX("APP_KEY", app_key, ESP_BLE_MESH_OCTET16_LEN);
		ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT, err_code %d", param->provisioner_add_app_key_comp.err_code);
		if (param->provisioner_add_app_key_comp.err_code == 0) {
			prov_key.app_idx = param->provisioner_add_app_key_comp.app_idx;

			esp_err_t err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_CLI, ESP_BLE_MESH_CID_NVAL);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Bind AppKey to ONOFF client");
			}

			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_CONFIG_CLI, ESP_BLE_MESH_CID_NVAL);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Bind AppKey to Config client");
			}

			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_GEN_LEVEL_CLI, ESP_BLE_MESH_CID_NVAL);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Bind AppKey to LEVEL client");
			}

			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_GEN_POWER_ONOFF_CLI, ESP_BLE_MESH_CID_NVAL);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Bind AppKey to POWER_ONOFF client");
			}

			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI, ESP_BLE_MESH_CID_NVAL);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Bind AppKey to POWER_LEVEL client");
			}

			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_GEN_BATTERY_CLI, ESP_BLE_MESH_CID_NVAL);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Bind AppKey to BATTERY client");
			}

			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_SENSOR_CLI, ESP_BLE_MESH_CID_NVAL);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Bind AppKey to SENSOR client");
			}

			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI, ESP_BLE_MESH_CID_NVAL);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Bind AppKey to LIGHT_LIGHTNESS client");
			}

			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_CLI, ESP_BLE_MESH_CID_NVAL);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Bind AppKey to LIGHT_CTL client");
			}

			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_LIGHT_HSL_CLI, ESP_BLE_MESH_CID_NVAL);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Bind AppKey to LIGHT_HSL client");
			}

			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_LIGHT_XYL_CLI, ESP_BLE_MESH_CID_NVAL);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Bind AppKey to LIGHT_XYL client");
			}

			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_VND_MODEL_ID_CLIENT, CID_ESP);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Failed to bind AppKey to vendor client");
			}
		}
		break;
	}
	case ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT:
		ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT, err_code %x", param->provisioner_bind_app_key_to_model_comp.err_code);
		break;
	case ESP_BLE_MESH_PROVISIONER_STORE_NODE_COMP_DATA_COMP_EVT:
		ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_STORE_NODE_COMP_DATA_COMP_EVT, err_code %d", param->provisioner_store_node_comp_data_comp.err_code);
		break;
	default:
		break;
	}
}

void ble_mesh_parse_node_comp_data(const uint8_t *data, uint16_t length)
{
	uint16_t cid, pid, vid, crpl, feat;
	uint16_t loc, model_id, company_id;
	uint8_t nums, numv;
	uint16_t offset;
	uint8_t num_element = 0;
	int i, j = 0;

	cid = COMP_DATA_2_OCTET(data, 0);
	pid = COMP_DATA_2_OCTET(data, 2);
	vid = COMP_DATA_2_OCTET(data, 4);
	crpl = COMP_DATA_2_OCTET(data, 6);
	feat = COMP_DATA_2_OCTET(data, 8);
	offset = 10;
	ESP_LOGI(TAG, "********************** Composition Data Start **********************");
	ESP_LOGI(TAG, "* CID = 0x%04X, PID = 0x%04X, VID = 0x%04X, CRPL = 0x%04X, Features = 0x%04X, EleNum = 0x%02X Length = %d*", cid, pid, vid, crpl, feat, num_element, length);
	for (; offset < length; ) {
		loc = COMP_DATA_2_OCTET(data, offset);
		nums = COMP_DATA_1_OCTET(data, offset + 2);
		numv = COMP_DATA_1_OCTET(data, offset + 3);
		offset += 4;
		ESP_LOGI(TAG, "* Loc 0x%04X, NumS 0x%02X, NumV 0x%02X *", loc, nums, numv);
		num_element++;
		j = 0;
		for (i = 0; i < nums; i++) {
			model_id = COMP_DATA_2_OCTET(data, offset);
			offset += 2;
			global_node.models[num_element - 1][j] = model_id;
			global_node.cid[num_element - 1][j] = 0xffff;
			j++;
			ESP_LOGI(TAG, "* SIG Model ID 0x%04X *", model_id);
		}
		for (i = 0; i < numv; i++) {
			company_id = COMP_DATA_2_OCTET(data, offset);
			model_id = COMP_DATA_2_OCTET(data, offset + 2);
			offset += 4;
			global_node.models[num_element - 1][j] = model_id;
			global_node.cid[num_element - 1][j] = company_id;
			j++;
			ESP_LOGI(TAG, "* Vendor Model ID 0x%04X, CID = 0x%04X*", model_id, company_id);
		}
		global_node.models[num_element - 1][j] = 0xffff;
	}
	global_node.element_num = num_element;
	ESP_LOGI(TAG, "*********************** Composition Data End ***********************");
}

void ble_mesh_config_client_cb(esp_ble_mesh_cfg_client_cb_event_t event,
		esp_ble_mesh_cfg_client_cb_param_t *param)
{
	esp_ble_mesh_client_common_param_t common = {0};
	esp_ble_mesh_cfg_client_set_state_t set = {0};
	static uint16_t wait_model_id, wait_cid;
	esp_ble_mesh_node_t *node = NULL;
	esp_err_t err = ESP_OK;
	static uint8_t i, j = 0;
	static uint16_t temp = 0;
	static uint8_t k = 0;

	if (param->error_code) {
		ESP_LOGE(TAG, "Send config client message failed (err %d) %x", param->error_code, param->params->opcode);
		return;
	}

	node = esp_ble_mesh_provisioner_get_node_with_addr(param->params->ctx.addr);
	if (!node) {
		ESP_LOGE(TAG, "Node 0x%04x not exists", param->params->ctx.addr);
		return;
	}

	switch (event) {
	case ESP_BLE_MESH_CFG_CLIENT_GET_STATE_EVT:
		switch(param->params->opcode){
		case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET: {
			ESP_LOG_BUFFER_HEX("Composition data", param->status_cb.comp_data_status.composition_data->data,
					param->status_cb.comp_data_status.composition_data->len);
			ble_mesh_parse_node_comp_data(param->status_cb.comp_data_status.composition_data->data,
					param->status_cb.comp_data_status.composition_data->len);
			err = esp_ble_mesh_provisioner_store_node_comp_data(param->params->ctx.addr,
					param->status_cb.comp_data_status.composition_data->data,
					param->status_cb.comp_data_status.composition_data->len);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Failed to store node composition data");
				break;
			}

			ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
			set.app_key_add.net_idx = prov_key.net_idx;
			set.app_key_add.app_idx = prov_key.app_idx;
			memcpy(set.app_key_add.app_key, prov_key.app_key, ESP_BLE_MESH_OCTET16_LEN);
			err = esp_ble_mesh_config_client_set_state(&common, &set);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Failed to send Config AppKey Add");
			}
			break;
		}
		default:
			break;
		}
		break;
	case ESP_BLE_MESH_CFG_CLIENT_SET_STATE_EVT:
		switch (param->params->opcode) {
		case ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD:
			ESP_LOGE(TAG, "Start Subscription Add CallBack param ElementID %x ModelID %x ComID %x", param->status_cb.model_app_status.element_addr, param->status_cb.model_app_status.model_id, param->status_cb.model_app_status.company_id);
			if (param->status_cb.model_app_status.company_id == 0xFFFF &&
								param->status_cb.model_app_status.element_addr == (node->unicast_addr) + k)
			{
				k++;
				if(k >= global_node.element_num)
				{
					k = 0;
					ESP_LOGI(TAG, "ADD Group success!!!");
					ESP_LOGE(TAG, "Start Group Control ON");
					static esp_ble_mesh_generic_client_set_state_t set_state = {0};
					ble_mesh_set_msg_common_element(&common, 0xC000, 0, onoff_client.model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET);
					set_state.onoff_set.op_en = true;
					set_state.onoff_set.onoff = 0;
					set_state.onoff_set.trans_time = 0;
					set_state.onoff_set.delay = 0;
					set_state.onoff_set.tid = set_state.onoff_set.tid + 1;
					err = esp_ble_mesh_generic_client_set_state(&common, &set_state);
					if (err) {
						ESP_LOGE(TAG, "ble_mesh_command_set failed %d", err);

					}
				}
				else
				{
					ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD);
					set.model_sub_add.element_addr = node->unicast_addr + k;
					set.model_sub_add.sub_addr = 0xC000;						//0xC000 is example address, BackEnd must send group address to GW
					set.model_sub_add.model_id = ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV;
					set.model_sub_add.company_id = ESP_BLE_MESH_CID_NVAL;
					err = esp_ble_mesh_config_client_set_state(&common, &set);
					if (err != ESP_OK) {
						ESP_LOGE(TAG, "Failed to send Config Model Subscription Add");
					}
				}
			}
			break;
		case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
			i = 0;
			j = 0;
			temp = global_node.models[j][i];
			ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
			set.model_app_bind.element_addr = (node->unicast_addr);
			set.model_app_bind.model_app_idx = prov_key.app_idx;
			set.model_app_bind.model_id = temp;
			set.model_app_bind.company_id = global_node.cid[j][i];
			err = esp_ble_mesh_config_client_set_state(&common, &set);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Failed to send Config Model App Bind");
				return;
			}
			wait_model_id = temp;
			wait_cid = global_node.cid[j][i];
			i++;
			break;
		case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
			if (param->status_cb.model_app_status.model_id == temp &&
					param->status_cb.model_app_status.company_id == wait_cid &&
					param->status_cb.model_app_status.element_addr == (node->unicast_addr) + j) {
				ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
				ESP_LOGI(TAG, "Model to BindKey ModelID = 0x%04X, CID = 0x%04X", wait_model_id, wait_cid);
				temp = global_node.models[j][i];
				if(temp==0xffff)
				{
					i = 0;
					j++;
					if(j >= global_node.element_num)
					{
						j = 0;
						ESP_LOGI(TAG, "Provision and config success!!!");
						unsigned char save_gw_addr_data[8] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
						unsigned char aes_data[16] = {0};
						ble_mesh_send_vendor_message(0xE00211, node->unicast_addr, 0, 8, save_gw_addr_data);
						memcpy(aes_data, aes128_data_header, 8);
						memcpy(&(aes_data[8]), &global_node.uuid, 6);
						aes_data[14] = global_node.unicast_addr;
						aes_data[15] = global_node.unicast_addr>>8;
//						for(int count = 0; count<16; count++)
//						{
//							ESP_LOGE(TAG, "AES128 data %x", aes_data[count]);
//						}
					    unsigned char aes128_out[16];
					    mbedtls_aes_context aes;
					    mbedtls_aes_init(&aes);
					    mbedtls_aes_setkey_enc(&aes, aes128_key, 128);
					    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, aes_data, aes128_out);
					    unsigned char check_type_data[8] = {0};
					    check_type_data[0] = 0x03;
					    check_type_data[1] = 0x00;
//					    for(int count = 0; count<16; count++)
//						{
//							ESP_LOGE(TAG, "AES128 data after encrypted %x", aes128_out[count]);
//						}
					    memcpy(&(check_type_data[2]), &aes128_out[10], 6);
//					    for(int count = 0; count<8; count++)
//						{
//							ESP_LOGE(TAG, "Check_type_data %x", check_type_data[count]);
//						}
					    ble_mesh_send_vendor_message(0xE00211, node->unicast_addr, 0, 8, check_type_data);

						sprintf(svalue, "{\"nodeId\":\"0x%04X\",\"eui64\":\"%02X%02X%02X%02X%02X%02X\",\"endpointCount\":%d,\"deviceType\":\"%02X%02X\"}",
								global_node.unicast_addr, global_node.MAC[0], global_node.MAC[1], global_node.MAC[2], global_node.MAC[3], global_node.MAC[4], global_node.MAC[5], global_node.element_num, global_node.uuid[6], global_node.uuid[7]);
						esp_mqtt_client_publish(client, topic_bledevicejoined, svalue, strlen(svalue), 0, 0);

						/* Publish state of BLE device after is joined*/
						ESP_LOGV(TAG, "Publish state of BLE device after is joined\r\n");
						uint8_t onoff_status = -1;
						for (uint8_t i = 1; i <= global_node.element_num; i++) {
							esp_err_t err = ble_mesh_command_get(ONOFF, (uint16_t)global_node.unicast_addr, (uint16_t)i-1);
							if(xQueueReceive(queueGetStateOnoffBLE, &onoff_status, (TickType_t) 10) == pdPASS) {
								char *svalue = malloc(100*sizeof(char));
								sprintf(svalue,"{\"nodeId\":\"0x%04X\",\"endpoint\":%d,\"data\":[{\"opcode\":\"%02X\",\"param\":\"%02X\"}]}", (uint16_t)global_node.unicast_addr, i, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, onoff_status);
								esp_mqtt_client_publish(client, topic_bledeviceaction, svalue, strlen(svalue), 0, 0);
								ESP_LOGI(TAG, "CMD ON-OFF, NodeId = 0x%04X, Element = %d, Value = %d", (uint16_t)global_node.unicast_addr, i, onoff_status);
								free(svalue);
							}
							vTaskDelay(300/portTICK_RATE_MS);
						}

						if((global_node.uuid[6]==0x02) && (global_node.uuid[7]==0x01))				//If device is wall switch, add all botton to a group
						{
							ESP_LOGE(TAG, "Start Subscription Add");
							ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD);
							set.model_sub_add.element_addr = node->unicast_addr;
							set.model_sub_add.sub_addr = 0xC000;								//0xC000 is example address, BackEnd must send group address to GW
							set.model_sub_add.model_id = ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV;
							set.model_sub_add.company_id = ESP_BLE_MESH_CID_NVAL;
							err = esp_ble_mesh_config_client_set_state(&common, &set);
							if (err != ESP_OK) {
								ESP_LOGE(TAG, "Failed to send Config Model Subscription Add");
							}
						}
						break;
					} else {
						temp = global_node.models[j][i];
					}

				}

				set.model_app_bind.model_app_idx = prov_key.app_idx;
				set.model_app_bind.model_id = temp;
				set.model_app_bind.element_addr = (node->unicast_addr) + j;
				set.model_app_bind.company_id = global_node.cid[j][i];
				err = esp_ble_mesh_config_client_set_state(&common, &set);
				if (err) {
					ESP_LOGE(TAG, "Failed to send Config Model App Bind");
					return;
				}
				wait_model_id = temp;
				wait_cid = global_node.cid[j][i];
				i++;

			}
			break;
		default:
			break;
		}
		break;
	case ESP_BLE_MESH_CFG_CLIENT_PUBLISH_EVT:
		if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_STATUS) {
			ESP_LOG_BUFFER_HEX("Composition data", param->status_cb.comp_data_status.composition_data->data,
					param->status_cb.comp_data_status.composition_data->len);
		}
		break;
	case ESP_BLE_MESH_CFG_CLIENT_TIMEOUT_EVT:
		ESP_LOGE(TAG, "ESP_BLE_MESH_CFG_CLIENT_TIMEOUT_EVT");
		switch (param->params->opcode) {
		case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET: {
			ESP_LOGE(TAG, "ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET");
			esp_ble_mesh_cfg_client_get_state_t get = {0};
			ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
			get.comp_data_get.page = COMP_DATA_PAGE_0;
			err = esp_ble_mesh_config_client_get_state(&common, &get);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Failed to send Config Composition Data Get");
			}
			break;
		}
		case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
			ESP_LOGE(TAG, "ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD");
			ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
			set.app_key_add.net_idx = prov_key.net_idx;
			set.app_key_add.app_idx = prov_key.app_idx;
			memcpy(set.app_key_add.app_key, prov_key.app_key, ESP_BLE_MESH_OCTET16_LEN);
			err = esp_ble_mesh_config_client_set_state(&common, &set);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Failed to send Config AppKey Add");
			}
			break;
		case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
			ESP_LOGE(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
			ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
			set.model_app_bind.element_addr = node->unicast_addr + j;
			set.model_app_bind.model_app_idx = prov_key.app_idx;
			set.model_app_bind.model_id = wait_model_id;
			set.model_app_bind.company_id = wait_cid;
			ESP_LOGE(TAG, "BindKey ModelID = 0x%04X, CID = 0x%04X", wait_model_id, wait_cid);
			err = esp_ble_mesh_config_client_set_state(&common, &set);
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Failed to send Config Model App Bind");
			}
			break;
		default:
			break;
		}
		break;
	default:
		ESP_LOGE(TAG, "Invalid config client event %u", event);
		break;
	}
	return;
}

void ble_mesh_send_sensor_message(uint32_t opcode)
{
	esp_ble_mesh_sensor_client_get_state_t get = {0};
	esp_ble_mesh_client_common_param_t common = {0};
	esp_ble_mesh_node_t *node = NULL;
	esp_err_t err = ESP_OK;

	node = esp_ble_mesh_provisioner_get_node_with_addr(server_address);
	if (node == NULL) {
		ESP_LOGE(TAG, "Node 0x%04x not exists", server_address);
		return;
	}

	ble_mesh_set_msg_common(&common, node, sensor_client.model, opcode);
	switch (opcode) {
	case ESP_BLE_MESH_MODEL_OP_SENSOR_CADENCE_GET:
		get.cadence_get.property_id = sensor_prop_id;
		break;
	case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTINGS_GET:
		get.settings_get.sensor_property_id = sensor_prop_id;
		break;
	case ESP_BLE_MESH_MODEL_OP_SENSOR_SERIES_GET:
		get.series_get.property_id = sensor_prop_id;
		break;
	default:
		break;
	}

	err = esp_ble_mesh_sensor_client_get_state(&common, &get);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to send sensor message 0x%04x", opcode);
	}
}

void ble_mesh_sensor_timeout(uint32_t opcode)
{
	switch (opcode) {
	case ESP_BLE_MESH_MODEL_OP_SENSOR_DESCRIPTOR_GET:
		ESP_LOGW(TAG, "Sensor Descriptor Get timeout, opcode 0x%04x", opcode);
		break;
	case ESP_BLE_MESH_MODEL_OP_SENSOR_CADENCE_GET:
		ESP_LOGW(TAG, "Sensor Cadence Get timeout, opcode 0x%04x", opcode);
		break;
	case ESP_BLE_MESH_MODEL_OP_SENSOR_CADENCE_SET:
		ESP_LOGW(TAG, "Sensor Cadence Set timeout, opcode 0x%04x", opcode);
		break;
	case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTINGS_GET:
		ESP_LOGW(TAG, "Sensor Settings Get timeout, opcode 0x%04x", opcode);
		break;
	case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTING_GET:
		ESP_LOGW(TAG, "Sensor Setting Get timeout, opcode 0x%04x", opcode);
		break;
	case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTING_SET:
		ESP_LOGW(TAG, "Sensor Setting Set timeout, opcode 0x%04x", opcode);
		break;
	case ESP_BLE_MESH_MODEL_OP_SENSOR_GET:
		ESP_LOGW(TAG, "Sensor Get timeout 0x%04x", opcode);
		break;
	case ESP_BLE_MESH_MODEL_OP_SENSOR_COLUMN_GET:
		ESP_LOGW(TAG, "Sensor Column Get timeout, opcode 0x%04x", opcode);
		break;
	case ESP_BLE_MESH_MODEL_OP_SENSOR_SERIES_GET:
		ESP_LOGW(TAG, "Sensor Series Get timeout, opcode 0x%04x", opcode);
		break;
	default:
		ESP_LOGE(TAG, "Unknown Sensor Get/Set opcode 0x%04x", opcode);
		return;
	}

	ble_mesh_send_sensor_message(opcode);
}

void ble_mesh_generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event,
		esp_ble_mesh_generic_client_cb_param_t *param)
{
	esp_ble_mesh_client_common_param_t common = {0};
	esp_ble_mesh_node_t *node = NULL;
	uint32_t opcode;
	uint16_t addr;

	short onoff_status = 0, level_status = 0;

	opcode = param->params->opcode;
	addr = param->params->ctx.addr;

	ESP_LOGI(TAG, "%s, error_code = 0x%02x, event = 0x%02x, addr: 0x%04x, opcode: 0x%04x, dest: 0x%04d, rssi: %d",
			__func__, param->error_code, event, param->params->ctx.addr, opcode, param->params->ctx.recv_dst, param->params->ctx.recv_rssi);

	if (param->error_code) {
		ESP_LOGE(TAG, "Send generic client message failed, opcode 0x%04x", opcode);
		return;
	}

	node = esp_ble_mesh_provisioner_get_node_with_addr(addr);
	if (!node) {
		ESP_LOGE(TAG, "%s: Get node info failed", __func__);
		return;
	}

	switch (event) {
	case ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT:
		switch (opcode) {
		case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET: {
			printf("ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET\r\n");
			onoff_status = param->status_cb.onoff_status.present_onoff;
			ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET onoff: 0x%02x", onoff_status);
			if (LINK_BUTTON) xQueueSend(queueGetStateOnoffBLE, &onoff_status, (TickType_t) 10);
			/* After Generic OnOff Status for Generic OnOff Get is received, Generic OnOff Set will be sent */
			ble_mesh_set_msg_common(&common, node, onoff_client.model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET);
			break;
		}

		case ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_GET: {
			level_status = param->status_cb.level_status.present_level;
			ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_GET onoff: 0x%02x", level_status);
			/* After Generic OnOff Status for Generic OnOff Get is received, Generic OnOff Set will be sent */
			break;
		}

		default:
			break;
		}
		break;
	case ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT:
		switch (opcode) {
		case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET: {
			printf("ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET\r\n");
			onoff_status = param->status_cb.onoff_status.present_onoff;
			esp_ble_mesh_node_t * node_tmp = esp_ble_mesh_provisioner_get_node_with_addr(param->params->ctx.addr);
			if(node_tmp){
				uint8_t endpoint = param->params->ctx.addr - node_tmp->unicast_addr + 1;
				sprintf(svalue,"{\"nodeId\":\"0x%04X\",\"endpoint\":%d,\"data\":[{\"opcode\":\"%02X\",\"param\":\"%02X\"}]}", node_tmp->unicast_addr, endpoint, param->params->ctx.recv_op, onoff_status);
				esp_mqtt_client_publish(client, topic_bledeviceaction, svalue, strlen(svalue), 0, 0);
			}
			else printf("node_tmp null\r\n");
			ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET onoff: 0x%02x", onoff_status);
			break;
		}

		case ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_SET: {
			level_status = param->status_cb.level_status.present_level;
			ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_SET onoff: 0x%02x", level_status);
			break;
		}

		default:
			break;
		}
		break;
	case ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT:
		printf("ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT\r\n");
		onoff_status = param->status_cb.onoff_status.present_onoff;
		esp_ble_mesh_node_t * node_tmp = esp_ble_mesh_provisioner_get_node_with_addr(param->params->ctx.addr);
		if(node_tmp){
			uint8_t endpoint = param->params->ctx.addr - node_tmp->unicast_addr + 1;
			if (LINK_BUTTON) {
				linkButton_t dataLinkButton;
				sprintf(dataLinkButton.idButton, "0x%04Xble%d", node_tmp->unicast_addr, endpoint);
				if (onoff_status == 0) {
					memcpy(dataLinkButton.stateButton, "off", strlen("off") + 1);
				} else {
					memcpy(dataLinkButton.stateButton, "on", strlen("on") + 1);
				}
				xQueueSend(queueIdButton, (void *) &dataLinkButton, (TickType_t)100);
			}
			sprintf(svalue,"{\"nodeId\":\"0x%04X\",\"endpoint\":%d,\"data\":[{\"opcode\":\"%02X\",\"param\":\"%02X\"}]}", node_tmp->unicast_addr, endpoint, param->params->ctx.recv_op, onoff_status);
			esp_mqtt_client_publish(client, topic_bledeviceaction, svalue, strlen(svalue), 0, 0);
		}
		ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET onoff: 0x%02x", onoff_status);
		break;
	case ESP_BLE_MESH_GENERIC_CLIENT_TIMEOUT_EVT:
		ESP_LOGE(TAG, "ble_mesh_generic_client_cb ESP_BLE_MESH_GENERIC_CLIENT_TIMEOUT_EVT");
		switch (opcode) {
		case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET: {
			ESP_LOGI(TAG, "--------timeout ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET onoff: 0x%02x", onoff_status);
			esp_ble_mesh_node_t * node_tmp = esp_ble_mesh_provisioner_get_node_with_addr(param->params->ctx.addr);
			if(node_tmp){
				uint8_t endpoint = param->params->ctx.addr - node_tmp->unicast_addr;
				ble_mesh_command_set(ONOFF, node_tmp->unicast_addr, endpoint, stateButtonBLE, 1);
			}
			else printf("node_tmp null\r\n");
			break;
		}
		case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET: {
			printf("------timeout ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET\r\n");
			ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET onoff: 0x%02x", onoff_status);
			/* After Generic OnOff Status for Generic OnOff Get is received, Generic OnOff Set will be sent */
			ble_mesh_set_msg_common(&common, node, onoff_client.model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET);
			break;
		}

		default:
			break;
		}
		break;
	default:
		ESP_LOGE(TAG, "Not a generic client status message event");
		break;
	}
}

void ble_mesh_sensor_client_cb(esp_ble_mesh_sensor_client_cb_event_t event,
		esp_ble_mesh_sensor_client_cb_param_t *param)
{
	esp_ble_mesh_node_t *node = NULL;

	ESP_LOGI(TAG, "Sensor client, event %u, addr 0x%04x Opcode %x ", event, param->params->ctx.addr, param->params->opcode);

	if (param->error_code) {
		ESP_LOGE(TAG, "Send sensor client message failed (err %d)", param->error_code);
		return;
	}

	node = esp_ble_mesh_provisioner_get_node_with_addr(param->params->ctx.addr);
	if (!node) {
		ESP_LOGE(TAG, "Node 0x%04x not exists", param->params->ctx.addr);
		return;
	}

	switch (event) {
	case ESP_BLE_MESH_SENSOR_CLIENT_GET_STATE_EVT:
		switch (param->params->opcode) {
		case ESP_BLE_MESH_MODEL_OP_SENSOR_DESCRIPTOR_GET:
			ESP_LOGI(TAG, "Sensor Descriptor Status, opcode 0x%04x", param->params->ctx.recv_op);
			if (param->status_cb.descriptor_status.descriptor->len != ESP_BLE_MESH_SENSOR_SETTING_PROPERTY_ID_LEN &&
					param->status_cb.descriptor_status.descriptor->len % ESP_BLE_MESH_SENSOR_DESCRIPTOR_LEN) {
				ESP_LOGE(TAG, "Invalid Sensor Descriptor Status length %d", param->status_cb.descriptor_status.descriptor->len);
				return;
			}
			if (param->status_cb.descriptor_status.descriptor->len) {
				ESP_LOG_BUFFER_HEX("Sensor Descriptor", param->status_cb.descriptor_status.descriptor->data,
						param->status_cb.descriptor_status.descriptor->len);
				/* If running with sensor server example, sensor client can get two Sensor Property IDs.
				 * Currently we use the first Sensor Property ID for the following demonstration.
				 */
				sensor_prop_id = param->status_cb.descriptor_status.descriptor->data[1] << 8 |
						param->status_cb.descriptor_status.descriptor->data[0];
			}
			break;
		case ESP_BLE_MESH_MODEL_OP_SENSOR_CADENCE_GET:
			ESP_LOGI(TAG, "Sensor Cadence Status, opcode 0x%04x, Sensor Property ID 0x%04x",
					param->params->ctx.recv_op, param->status_cb.cadence_status.property_id);
			ESP_LOG_BUFFER_HEX("Sensor Cadence", param->status_cb.cadence_status.sensor_cadence_value->data,
					param->status_cb.cadence_status.sensor_cadence_value->len);
			break;
		case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTINGS_GET:
			ESP_LOGI(TAG, "Sensor Settings Status, opcode 0x%04x, Sensor Property ID 0x%04x",
					param->params->ctx.recv_op, param->status_cb.settings_status.sensor_property_id);
			ESP_LOG_BUFFER_HEX("Sensor Settings", param->status_cb.settings_status.sensor_setting_property_ids->data,
					param->status_cb.settings_status.sensor_setting_property_ids->len);
			break;
		case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTING_GET:
			ESP_LOGI(TAG, "Sensor Setting Status, opcode 0x%04x, Sensor Property ID 0x%04x, Sensor Setting Property ID 0x%04x",
					param->params->ctx.recv_op, param->status_cb.setting_status.sensor_property_id,
					param->status_cb.setting_status.sensor_setting_property_id);
			if (param->status_cb.setting_status.op_en) {
				ESP_LOGI(TAG, "Sensor Setting Access 0x%02x", param->status_cb.setting_status.sensor_setting_access);
				ESP_LOG_BUFFER_HEX("Sensor Setting Raw", param->status_cb.setting_status.sensor_setting_raw->data,
						param->status_cb.setting_status.sensor_setting_raw->len);
			}
			break;
		case ESP_BLE_MESH_MODEL_OP_SENSOR_GET:
			ESP_LOGI(TAG, "Sensor Status, opcode 0x%04x", param->params->ctx.recv_op);
			if (param->status_cb.sensor_status.marshalled_sensor_data->len) {
				ESP_LOG_BUFFER_HEX("Sensor Data", param->status_cb.sensor_status.marshalled_sensor_data->data,
						param->status_cb.sensor_status.marshalled_sensor_data->len);
				uint8_t *data = param->status_cb.sensor_status.marshalled_sensor_data->data;
				uint16_t length = 0;
				for (; length < param->status_cb.sensor_status.marshalled_sensor_data->len; ) {
					uint8_t fmt = ESP_BLE_MESH_GET_SENSOR_DATA_FORMAT(data);
					uint8_t data_len = ESP_BLE_MESH_GET_SENSOR_DATA_LENGTH(data, fmt);
					uint16_t prop_id = ESP_BLE_MESH_GET_SENSOR_DATA_PROPERTY_ID(data, fmt);
					uint8_t mpid_len = (fmt == ESP_BLE_MESH_SENSOR_DATA_FORMAT_A ?
							ESP_BLE_MESH_SENSOR_DATA_FORMAT_A_MPID_LEN : ESP_BLE_MESH_SENSOR_DATA_FORMAT_B_MPID_LEN);
					ESP_LOGI(TAG, "Format %s, length 0x%02x, Sensor Property ID 0x%04x",
							fmt == ESP_BLE_MESH_SENSOR_DATA_FORMAT_A ? "A" : "B", data_len, prop_id);
					if (data_len != ESP_BLE_MESH_SENSOR_DATA_ZERO_LEN) {
						ESP_LOG_BUFFER_HEX("Sensor Data", data + mpid_len, data_len + 1);
						length += mpid_len + data_len + 1;
						data += mpid_len + data_len + 1;
					} else {
						length += mpid_len;
						data += mpid_len;
					}
				}
			}
			break;
		case ESP_BLE_MESH_MODEL_OP_SENSOR_COLUMN_GET:
			ESP_LOGI(TAG, "Sensor Column Status, opcode 0x%04x, Sensor Property ID 0x%04x",
					param->params->ctx.recv_op, param->status_cb.column_status.property_id);
			ESP_LOG_BUFFER_HEX("Sensor Column", param->status_cb.column_status.sensor_column_value->data,
					param->status_cb.column_status.sensor_column_value->len);
			break;
		case ESP_BLE_MESH_MODEL_OP_SENSOR_SERIES_GET:
			ESP_LOGI(TAG, "Sensor Series Status, opcode 0x%04x, Sensor Property ID 0x%04x",
					param->params->ctx.recv_op, param->status_cb.series_status.property_id);
			ESP_LOG_BUFFER_HEX("Sensor Series", param->status_cb.series_status.sensor_series_value->data,
					param->status_cb.series_status.sensor_series_value->len);
			break;
		default:
			ESP_LOGI(TAG, "Unknown Sensor Get opcode 0x%04x", param->params->ctx.recv_op);
			break;
		}
		break;
		case ESP_BLE_MESH_SENSOR_CLIENT_SET_STATE_EVT:
			switch (param->params->opcode) {
			case ESP_BLE_MESH_MODEL_OP_SENSOR_CADENCE_SET:
				ESP_LOGI(TAG, "Sensor Cadence Status, opcode 0x%04x, Sensor Property ID 0x%04x",
						param->params->ctx.recv_op, param->status_cb.cadence_status.property_id);
				ESP_LOG_BUFFER_HEX("Sensor Cadence", param->status_cb.cadence_status.sensor_cadence_value->data,
						param->status_cb.cadence_status.sensor_cadence_value->len);
				break;
			case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTING_SET:
				ESP_LOGI(TAG, "Sensor Setting Status, opcode 0x%04x, Sensor Property ID 0x%04x, Sensor Setting Property ID 0x%04x",
						param->params->ctx.recv_op, param->status_cb.setting_status.sensor_property_id,
						param->status_cb.setting_status.sensor_setting_property_id);
				if (param->status_cb.setting_status.op_en) {
					ESP_LOGI(TAG, "Sensor Setting Access 0x%02x", param->status_cb.setting_status.sensor_setting_access);
					ESP_LOG_BUFFER_HEX("Sensor Setting Raw", param->status_cb.setting_status.sensor_setting_raw->data,
							param->status_cb.setting_status.sensor_setting_raw->len);
				}
				break;
			default:
				ESP_LOGI(TAG, "Unknown Sensor Set opcode 0x%04x", param->params->ctx.recv_op);
				break;
			}
			break;
			case ESP_BLE_MESH_SENSOR_CLIENT_PUBLISH_EVT:
				ESP_LOGI(TAG, "ESP_BLE_MESH_SENSOR_CLIENT_PUBLISH_EVT");
				const char *buff = bt_hex(param->status_cb.sensor_status.marshalled_sensor_data->data, param->status_cb.sensor_status.marshalled_sensor_data->len);
				sprintf(svalue,"{\"nodeId\":\"0x%04X\",\"data\":[{\"opcode\":\"%02X\",\"param\":\"%s\"}]}", node->unicast_addr, param->params->ctx.recv_op, buff);
				esp_mqtt_client_publish(client, topic_bledeviceaction, svalue, strlen(svalue), 0, 0);
				break;
			case ESP_BLE_MESH_SENSOR_CLIENT_TIMEOUT_EVT:
				ESP_LOGI(TAG, "ESP_BLE_MESH_SENSOR_CLIENT_TIMEOUT_EVT");
				ble_mesh_sensor_timeout(param->params->opcode);
				break;
			default:
				break;
	}
}

void ble_mesh_custom_model_cb(esp_ble_mesh_model_cb_event_t event,
                                             esp_ble_mesh_model_cb_param_t *param)
{
    switch (event) {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT:
        if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL_OP_STATUS) {
//            ESP_LOGI(TAG, "Recv 0x%06x, tid 0x%04x, time %lldus",
//                param->model_operation.opcode, store.vnd_tid, end_time - start_time);
        }
        break;
    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
        if (param->model_send_comp.err_code) {
            ESP_LOGE(TAG, "Failed to send message 0x%06x", param->model_send_comp.opcode);
            break;
        }
        ESP_LOGI(TAG, "Send 0x%06x", param->model_send_comp.opcode);
        break;
    case ESP_BLE_MESH_CLIENT_MODEL_RECV_PUBLISH_MSG_EVT:
        ESP_LOGI(TAG, "Receive publish message 0x%06x", param->client_recv_publish_msg.opcode);
        break;
    case ESP_BLE_MESH_CLIENT_MODEL_SEND_TIMEOUT_EVT:
        ESP_LOGW(TAG, "Client message 0x%06x timeout", param->client_send_timeout.opcode);
 //       example_ble_mesh_send_vendor_message(true, 0x009C);
        break;
    default:
        break;
    }
}

esp_err_t ble_mesh_init(void)
{
	esp_err_t err = ESP_OK;

	uint8_t app_key[16] = {0x89, 0xC7, 0x62, 0xA4, 0xDD, 0xA5, 0x0F, 0x97, 0xA7, 0xE5, 0x7D, 0x2F, 0xCA, 0xEF, 0x1E, 0x85};

	prov_key.net_idx = ESP_BLE_MESH_KEY_PRIMARY;
	prov_key.app_idx = APP_KEY_IDX;
	for(int i = 0; i < 16 ; i++){
		prov_key.app_key[i] = app_key[i];
	}

	esp_ble_mesh_register_prov_callback(ble_mesh_provisioning_cb);
	esp_ble_mesh_register_config_client_callback(ble_mesh_config_client_cb);
	esp_ble_mesh_register_sensor_client_callback(ble_mesh_sensor_client_cb);
	esp_ble_mesh_register_generic_client_callback(ble_mesh_generic_client_cb);
	esp_ble_mesh_register_custom_model_callback(ble_mesh_custom_model_cb);

	err = esp_ble_mesh_init(&provision, &composition);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize mesh stack (err %d)", err);
		return err;
	}

    err = esp_ble_mesh_client_model_init(&vnd_models[0]);
    if (err) {
        ESP_LOGE(TAG, "Failed to initialize vendor client");
        return err;
    }

	err = esp_ble_mesh_provisioner_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to enable mesh provisioner (err %d)", err);
		return err;
	}

	err = esp_ble_mesh_provisioner_add_local_app_key(prov_key.app_key, prov_key.net_idx, prov_key.app_idx);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to add local AppKey (err %d)", err);
		return err;
	}

	ESP_LOGI(TAG, "BLE Mesh Provisioner initialized");
	return err;
}

static void ble_mesh_get_dev_uuid(uint8_t *dev_uuid)
{
	if (dev_uuid == (void *)0) { // NULL
		ESP_LOGE(TAG, "%s, Invalid device uuid", __func__);
		return;
	}
	memcpy(dev_uuid + 2, esp_bt_dev_get_address(), BD_ADDR_LENGTH);
}

static esp_err_t bluetooth_init(void)
{
	esp_err_t ret;

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)); // @suppress("Symbol is not resolved")

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ret = esp_bt_controller_init(&bt_cfg);
	if (ret) {
		ESP_LOGE(TAG, "%s initialize controller failed", __func__);
		return ret;
	}

	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE); // @suppress("Symbol is not resolved")
	if (ret) {
		ESP_LOGE(TAG, "%s enable controller failed", __func__);
		return ret;
	}
	ret = esp_bluedroid_init();
	if (ret) {
		ESP_LOGE(TAG, "%s init bluetooth failed", __func__);
		return ret;
	}
	ret = esp_bluedroid_enable();
	if (ret) {
		ESP_LOGE(TAG, "%s enable bluetooth failed", __func__);
		return ret;
	}

	return ret;
}

esp_err_t ble_init(void){
	esp_err_t err = ESP_OK;
	err = bluetooth_init();
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
		return err;
	} else {
		ESP_LOGI(TAG, "esp32_bluetooth_init success (err %d)", err);
	}

	ble_mesh_get_dev_uuid(dev_uuid);

	/* Initialize the Bluetooth Mesh Subsystem */
	err = ble_mesh_init();
	if (err) {
		ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
		return err;
	}

	ESP_LOGI(TAG, "esp32_bluetooth_mesh_init success (err %d)", err);
	return err;
}
esp_err_t ble_deinit(void) {
	esp_err_t err = ESP_OK;
	esp_ble_mesh_deinit_param_t param1;
	param1.erase_flash = false;
	err = esp_ble_mesh_deinit(&param1);
	if (err) {
		ESP_LOGE(TAG, "Bluetooth mesh deinit failed (err %d)", err);
	} else {
		ESP_LOGI(TAG, "esp_ble_mesh_deinit success (err %d)", err);
	}
	esp_bluedroid_disable();
	esp_bluedroid_deinit();
	return err;
}

esp_err_t ble_mesh_command_set(uint8_t command, uint16_t unicast_addr, uint16_t element_addr, int value, uint8_t resp)
{
	uint32_t opcode = 0;
	static esp_ble_mesh_generic_client_set_state_t set_state = {0};
	static esp_ble_mesh_client_common_param_t common = {0};
	esp_ble_mesh_client_t generic_client;
	esp_err_t err = ESP_OK;

	switch (command)
	{
	case ONOFF:
		generic_client = onoff_client;
		stateButtonBLE = value;
		if(resp)
			opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET;
		else
			opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK;
		break;
	case LEVEL:
		generic_client = level_client;
		if(resp)
			opcode = ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_SET;
		else
			opcode = ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_SET_UNACK;
		break;
	default:
		break;
	}
	ble_mesh_set_msg_common_element(&common, unicast_addr, element_addr, generic_client.model, opcode);
	set_state.onoff_set.op_en = true;
	set_state.onoff_set.onoff = value;
	set_state.onoff_set.trans_time = 0;
	set_state.onoff_set.delay = 0;
	set_state.onoff_set.tid = set_state.onoff_set.tid + 1;
	err = esp_ble_mesh_generic_client_set_state(&common, &set_state);
	if (err) {
		ESP_LOGE(TAG, "ble_mesh_command_set failed %d", err);
		return err;
	}
	return err;
}

esp_err_t ble_mesh_command_get(uint8_t command, uint16_t unicast_addr, uint16_t element_addr)
{
	uint32_t opcode = 0;
	static esp_ble_mesh_generic_client_get_state_t get_state = {0};
	static esp_ble_mesh_client_common_param_t common = {0};
	esp_ble_mesh_client_t generic_client;
	esp_err_t err = ESP_OK;

	switch (command)
	{
	case ONOFF:
		generic_client = onoff_client;
		opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET;
		break;
	case LEVEL:
		generic_client = level_client;
		opcode = ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_GET;
		break;
	default:
		break;
	}
	ble_mesh_set_msg_common_element(&common, unicast_addr, element_addr, generic_client.model, opcode);
	err = esp_ble_mesh_generic_client_get_state(&common, &get_state);
	if (err) {
		ESP_LOGE(TAG, "ble_mesh_command_get failed %d", err);
		return err;
	}
	return err;
}

esp_err_t ble_mesh_node_reset(uint16_t unicast_addr)
{
	esp_ble_mesh_client_common_param_t common = {0};

	ble_mesh_set_msg_common_element(&common, unicast_addr, 0, config_client.model, ESP_BLE_MESH_MODEL_OP_NODE_RESET);
	return esp_ble_mesh_config_client_set_state(&common, NULL);
}

esp_err_t add_fast_prov_group_address(uint16_t model_id, uint16_t group_addr)
{
    const esp_ble_mesh_comp_t *comp = NULL;
    esp_ble_mesh_elem_t *element = NULL;
    esp_ble_mesh_model_t *model = NULL;
    int i, j;

    if (!ESP_BLE_MESH_ADDR_IS_GROUP(group_addr)) {
        return ESP_ERR_INVALID_ARG;
    }

    comp = esp_ble_mesh_get_composition_data();
    if (!comp) {
        return ESP_FAIL;
    }

    for (i = 0; i < comp->element_count; i++) {
        element = &comp->elements[i];
        model = esp_ble_mesh_find_sig_model(element, model_id);
        if (!model) {
            continue;
        }
        for (j = 0; j < ARRAY_SIZE(model->groups); j++) {
            if (model->groups[j] == group_addr) {
                break;
            }
        }
        if (j != ARRAY_SIZE(model->groups)) {
            ESP_LOGW(TAG, "%s: Group address already exists, element index: %d", __func__, i);
            continue;
        }
        for (j = 0; j < ARRAY_SIZE(model->groups); j++) {
            if (model->groups[j] == ESP_BLE_MESH_ADDR_UNASSIGNED) {
                model->groups[j] = group_addr;
                break;
            }
        }
        if (j == ARRAY_SIZE(model->groups)) {
            ESP_LOGE(TAG, "%s: Model is full of group addresses, element index: %d", __func__, i);
        }
    }

    return ESP_OK;
}

esp_err_t ble_mesh_send_vendor_message(uint32_t opcode, uint16_t unicast_addr, uint16_t model_id, unsigned char len, unsigned char *data)
{
    esp_ble_mesh_msg_ctx_t ctx = {0};
    esp_err_t err;
//    unsigned char data[20] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    ctx.net_idx = prov_key.net_idx;
    ctx.app_idx = prov_key.app_idx;
    ctx.addr = unicast_addr + model_id;
    ctx.send_ttl = MSG_SEND_TTL;
    ctx.send_rel = MSG_SEND_REL;
//    opcode = ESP_BLE_MESH_VND_MODEL_OP_SEND;
//    opcode = 0xC0 | 0xE0;
//    opcode = 0xE00211;

//    if (resend == false) {
//        store.vnd_tid++;
//    }

    err = esp_ble_mesh_client_model_send_msg(vendor_client.model, &ctx, opcode,
            len, data, MSG_TIMEOUT, false, MSG_ROLE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send vendor message 0x%06x %d", opcode, err);
        return err;
    }
    return err;
}

esp_err_t ble_mesh_delete_group(uint16_t group_addr, uint16_t unicast_addr, uint16_t element_addr_offset, uint16_t model_id)
{
	static esp_ble_mesh_cfg_client_set_state_t set = {0};
	static esp_ble_mesh_client_common_param_t common = {0};
	esp_err_t err;

	ble_mesh_set_msg_common_element(&common, unicast_addr, element_addr_offset, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_SUB_DELETE);
	set.model_sub_delete.element_addr = unicast_addr + element_addr_offset;
	set.model_sub_add.sub_addr = group_addr;						//0xC000 is example address, BackEnd must send group address to GW
	set.model_sub_add.model_id = model_id;
	set.model_sub_add.company_id = ESP_BLE_MESH_CID_NVAL;
	err = esp_ble_mesh_config_client_set_state(&common, &set);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to send Config Model Subscription Delete");
	}
	return err;
}
