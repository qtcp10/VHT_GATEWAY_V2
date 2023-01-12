#ifndef _LINK_NODE_H_
#define _LINK_NODE_H_

#include "./map/map.h"

typedef struct {
	char stateButton[5]; 	/*State of button is "on" or "off"*/
	char idButton[40];	/*Key id button in map*/
} linkButton_t;

void linkButtonInit(void);

/**
 * @brief: Parsing json and store in map
 * @param textBuf is json file
 *
 * @return map structure
 */
map_str_t json_parsing(char * textBuf);

void sendIdButton(void *arg);
void sendCommandZigbee(void *arg);
void sendCommandBLE(void *arg);

#endif // _LINK_NODE_H_
