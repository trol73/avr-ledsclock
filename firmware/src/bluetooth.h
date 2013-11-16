/*
 * bluetooth.h
 *
 * Unit for support bluetooth modules: HC-06
 *
 * If bluetooth reset pin is used the these two macroses should be defined:
 *
 * 		#define BLUETOOTH_RESET_PORT		D
 * 		#define BLUETOOTH_RESET_PIN			2
 *
 *
 *  Created on: Feb 15, 2013
 *      Author: trol
 */

#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "config.h"
#include "uart.h"
#include "stddef.h"




#define BLUETOOTH_BAUDRATE_1200		'1'
#define BLUETOOTH_BAUDRATE_2400		'2'
#define BLUETOOTH_BAUDRATE_4800		'3'
#define BLUETOOTH_BAUDRATE_9600		'4'
#define BLUETOOTH_BAUDRATE_19200	'5'
#define BLUETOOTH_BAUDRATE_38400	'6'
#define BLUETOOTH_BAUDRATE_57600	'7'
#define BLUETOOTH_BAUDRATE_115200	'8'
#define BLUETOOTH_BAUDRATE_230400	'9'
#define BLUETOOTH_BAUDRATE_460800	'A'
#define BLUETOOTH_BAUDRATE_921600	'B'
#define BLUETOOTH_BAUDRATE_1382400	'C'


static inline void bt_Init();
static inline void bt_Reset(bool reset);
static inline void bt_ResetPulse();
static inline uint16_t bt_cmd(const char *cmd, char *result, uint16_t waitTimeout, uint16_t readTimeout, uint8_t maxLen);
static inline bool bt_Test();
static inline char* bt_GetVersion(char *result);
static inline bool bt_SetName(PGM_P name);
static inline bool bt_SetPin();
static inline bool bt_SetBaudRate();




/**
 *
 */
static inline void bt_Init() {
#ifdef BLUETOOTH_RESET_PORT
	DDR(BLUETOOTH_RESET_PORT) |= _BV(BLUETOOTH_RESET_PIN);
#endif
	UBRRH = HI_WORD(UBRR_VALUE_230400);
	UBRRL = LO_WORD(UBRR_VALUE_230400);

	//UCSRB = _BV(TXEN) | _BV(RXEN) | _BV(TXCIE) | _BV(RXCIE); // tx/rx enable, interrupts enable

	// Set frame format: 8data, 1stop bit
    UCSRC = _BV(URSEL) | _BV(UCSZ0) | _BV(UCSZ1);
	UCSRB = _BV(TXEN) | _BV(RXEN);// | _BV(RXCIE);// | _BV(TXCIE); // tx/rx enable
}



/**
 *
 */
static inline uint16_t bt_cmd(const char *cmd, char *result, uint16_t waitTimeout, uint16_t readTimeout, uint8_t maxLen) {
	uart_puts_p(cmd);

	uint8_t posOut = 0;
	uint16_t errCnt = 0;
	for ( uint16_t i = 0; i < waitTimeout; i++ ) {

		if ( UCSRA & _BV(RXC) ) {
			result[posOut++] = UDR;
			if ( posOut >= maxLen-2 ) {
				result[posOut] = 0;
				return i;
			}
			errCnt = 0;
		} else {
			_delay_ms(1);
			errCnt++;
			if ( posOut > 0 && errCnt > readTimeout ) {
				result[posOut] = 0;
				return i;
			}
		}
	} // for
	result[posOut] = 0;
	return waitTimeout;
}

#ifdef BLUETOOTH_RESET_PORT
/**
 *
 */
static inline void bt_Reset(bool reset) {
	if (reset) {
		// set '0' on RESET pin
		PORT(BLUETOOTH_RESET_PORT) &= ~_BV(BLUETOOTH_RESET_PIN);
	} else {
		// set '1' on RESET pin
		PORT(BLUETOOTH_RESET_PORT) |= _BV(BLUETOOTH_RESET_PIN);
	}
}


/**
 *
 */
static inline void bt_ResetPulse() {
	bt_Reset(true);
	_delay_loop_1(0xff);
	bt_Reset(false);
}
#endif




/**
 *
 */
static inline bool bt_Test() {
	char result[4];
		if ( bt_cmd(PSTR("AT"), result, 500, 50, 4) >= 500 ) {
		return false;
	}
	return result[0] == 'O' && result[1] == 'K' && result[2] == 0;
}


/**
 *
 */
static inline char* bt_GetVersion(char *result) {
	if ( bt_cmd(PSTR("AT+VERSION"), result, 1500, 50, 16) >= 1500 ) {
		return 0;
	}
	return result + 2;
}


/**
 *
 */
static inline bool bt_SetName(PGM_P name) {
	char cmd[30];
	cmd[0] = 'A';
	cmd[1] = 'T';
	cmd[2] = '+';
	cmd[3] = 'N';
	cmd[4] = 'A';
	cmd[5] = 'M';
	cmd[6] = 'E';
	for ( uint8_t i = 0; ; i++) {
		uint8_t ch = pgm_read_byte_near(name++);
		if ( !ch ) {
			break;
		}
		cmd[7+i] = ch;
	}
	if ( bt_cmd(cmd, cmd, 5500, 1000, 25) >= 5500 ) {
		return false;
	}
	return cmd[0] == 'O' && cmd[1] == 'K';
}

/**
 *
 */
static inline bool bt_SetPin() {
	char result[14];
	if ( bt_cmd(PSTR("AT+PIN2013"), result, 5000, 500, 14) >= 5000 ) {
		return false;
	}
	return result[0] == 'O' && result[1] == 'K' && result[2] == 's' && result[3] == 'e' && result[4] == 't' && result[5] == 'P' && result[6] == 'I' && result[7] == 'N' && result[8] == 0;
}


static inline bool bt_SetBaudRate() {
	char result[14];
	if ( bt_cmd(PSTR("AT+BAUD9"), result, 5000, 500, 14) >= 5000 ) {
		return false;
	}
	return result[0] == 'O' && result[1] == 'K';
}



#endif // _BLUETOOTH_H_
