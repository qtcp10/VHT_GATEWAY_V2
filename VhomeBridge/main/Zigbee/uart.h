/*
 * uart.h
 *
 *  Created on: Jun 29, 2022
 *      Author: Thanh Vu
 */

#ifndef MAIN_ZIGBEE_UART_H_
#define MAIN_ZIGBEE_UART_H_

#define RX_BUF_SIZE		1024
#define TX_BUF_SIZE		256

#define TXD_PIN 		5
#define RXD_PIN 		6

void uart_init();

void uart_tx_task(void *arg);

void uart_rx_task(void *arg);

#endif /* MAIN_ZIGBEE_UART_H_ */
