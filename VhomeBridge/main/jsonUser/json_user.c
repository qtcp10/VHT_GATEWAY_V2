
#include "json_user.h"
#include "../common.h"

void JSON_analyze_post(char* my_json_string, char * deviceid, char * devicetoken, char * brokerInfor)
{
	cJSON *root = cJSON_Parse(my_json_string);
	cJSON *current_element = NULL;
	cJSON_ArrayForEach(current_element, root)
	{
		if (current_element->string)
		{
			const char* string = current_element->string;
			if(strcmp(string, "broker") == 0)
			{
				memcpy(brokerInfor, current_element->valuestring, strlen(current_element->valuestring) + 1);
			}
			if(strcmp(string, "deviceid") == 0)
			{
				memcpy(deviceid, current_element->valuestring, strlen(current_element->valuestring) + 1);
			}
			if(strcmp(string, "devicetoken") == 0)
			{
				memcpy(devicetoken, current_element->valuestring, strlen(current_element->valuestring) + 1);
			}
		}
	}
	cJSON_Delete(root);
}

void JSON_analyze_SUB_MQTT(char* my_json_string, cmd * sub_cmd)
{
	cJSON *root = cJSON_Parse(my_json_string);
	cJSON *current_element = NULL;
	cJSON_ArrayForEach(current_element, root)
	{
		if (current_element->string)
		{
			const char* string = current_element->string;
			if(strcmp(string, "action") == 0)
			{
				memcpy(sub_cmd->action, current_element->valuestring, strlen(current_element->valuestring)+ 1);
			}
			if(strcmp(string, "nodeId") == 0)
			{
				memcpy(sub_cmd->nodeID, current_element->valuestring, strlen(current_element->valuestring) + 1);
			}
			if(strcmp(string, "eui64") == 0)
			{
				memcpy(sub_cmd->EUI64, current_element->valuestring, strlen(current_element->valuestring) + 1);
			}
			if(strcmp(string, "endpoint") == 0)
			{
				sub_cmd->endpoint = current_element->valueint;
			}
			if(strcmp(string, "value") == 0)
			{
				memcpy(sub_cmd->value, current_element->valuestring, strlen(current_element->valuestring) + 1);
			}
			if(strcmp(string,"url") == 0)
			{
				memcpy(sub_cmd->url, current_element->valuestring, strlen(current_element->valuestring) + 1);
			}
			if(strcmp(string,"protocol") == 0)
			{
				memcpy(sub_cmd->protocol, current_element->valuestring, strlen(current_element->valuestring) + 1);
			}
		}
	}
	cJSON_Delete(root);
}

void JSON_analyze_SUB_ZIGBEE(char* my_json_string, action * sub_action, cmd * sub_cmd)
{
	cJSON *root = cJSON_Parse(my_json_string);
	cJSON *current_element = NULL;
	cJSON_ArrayForEach(current_element, root)
	{
		if (current_element->string)
		{
			const char* string = current_element->string;
			if(strcmp(string, "action") == 0)
			{
				memcpy(sub_action->action, current_element->valuestring, strlen(current_element->valuestring)+ 1);
			}
			if(strcmp(string, "fwVersion") == 0)
			{
				memcpy(sub_action->fwVersion, current_element->valuestring, strlen(current_element->valuestring)+ 1);
			}
			if(strcmp(string, "nodeId") == 0)
			{
				memcpy(sub_cmd->nodeID, current_element->valuestring, strlen(current_element->valuestring) + 1);
			}
			if(strcmp(string, "endpoint") == 0)
			{
				sub_cmd->endpoint = current_element->valueint;
			}
			if(strcmp(string, "data") == 0)
			{
				cJSON *item = cJSON_GetObjectItem(root, "data");
				cJSON *subitem = cJSON_GetArrayItem(item, 0);
				cJSON *attrBuf = cJSON_GetObjectItem(subitem, "attributeBuffer");
				if(attrBuf != NULL) {
					if (strcmp(attrBuf->valuestring, "00") == 0) {
						memcpy(sub_cmd->value, "off", strlen("off") + 1);
					} else {
						memcpy(sub_cmd->value, "on", strlen("on") + 1);
					}
				}
			}
		}
	}
	cJSON_Delete(root);
}
