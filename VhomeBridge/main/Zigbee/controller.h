/*
 * controller.h
 *
 *  Created on: Jul 7, 2022
 *      Author: Thanh Vu
 */

#ifndef MAIN_ZIGBEE_CONTROLLER_H_
#define MAIN_ZIGBEE_CONTROLLER_H_


#define ZIGBEE_RESET		4
#define ZIGBEE_BOOT			7
#define ZIGBEE_PIN_SEL  	((1ULL<<ZIGBEE_RESET) | (1ULL<<ZIGBEE_BOOT))

#define HIGH				1
#define LOW					0

void zigbee_controller_init(void);
void zigbee_reset();
void zigbee_boot();

#endif /* MAIN_ZIGBEE_CONTROLLER_H_ */
