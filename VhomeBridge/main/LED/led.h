/*
 * led.h
 *
 *  Created on: Jun 29, 2022
 *      Author: Thanh Vu
 */
#include "../common.h"

#ifndef MAIN_LED_LED_H_
#define MAIN_LED_LED_H_

#ifdef KIT_TEST

#define LED_STATUS    		18
#define LED_SIGNAL    		19
#define LED_PIN_SEL  		((1ULL<<LED_STATUS) | (1ULL<<LED_SIGNAL))


#define LED_ON				1
#define LED_OFF				0

#else

#define LED_STATUS    		10
#define LED_SIGNAL    		3
#define LED_PIN_SEL  		((1ULL<<LED_STATUS) | (1ULL<<LED_SIGNAL))

#define LED_ON				0
#define LED_OFF				1

#endif

void led_init(void);
void led_status_task(void *arg);
void led_signal_task(void *arg);


#endif /* MAIN_LED_LED_H_ */
