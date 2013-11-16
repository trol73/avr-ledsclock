/*
 * Copyright (c) 2008-2013 by Oleg Trifonov <otrifonow@gmail.com>
 *
 * http://trolsoft.ru
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */


#include "uart.h"

#include <avr/io.h>

#include "stddef.h"
#include "config.h"


/**
 *
 */
void uart_puts(const char* str) {
	while ( *str ) {
		uart_putc(*str++);
	}
}

/**
 *
 */
void uart_puts_p(PGM_P str) {
	while ( true ) {
		uint8_t b = pgm_read_byte_near(str++);
		if ( !b ) {
			break;
		}
		uart_putc(b);
	}
}





void uart_putc_hex(uint8_t b) {
	// upper nibble
	if ( (b >> 4) < 0x0a ) {
		uart_putc((b >> 4) + '0');
	} else {
		uart_putc((b >> 4) - 0x0a + 'a');
	}

	// lower nibble
	if ( (b & 0x0f) < 0x0a ) {
		uart_putc((b & 0x0f) + '0');
	} else {
		uart_putc((b & 0x0f) - 0x0a + 'a');
	}
}


inline void uart_putw_hex(uint16_t w) {
	uart_putc_hex((uint8_t) (w >> 8));
	uart_putc_hex((uint8_t) (w & 0xff));
}


inline void uart_putdw_hex(uint32_t dw) {
	uart_putw_hex((uint16_t) (dw >> 16));
	uart_putw_hex((uint16_t) (dw & 0xffff));
}

void uart_putb_dec(uint8_t b) {
	uart_putdw_dec(b);
}

void uart_putw_dec(uint16_t w) {
	uint16_t num = 10000;
	uint8_t started = 0;

	while ( num > 0 ) {
		uint8_t b = w / num;
		if ( b > 0 || started || num == 1 ) {
			uart_putc('0' + b);
			started = 1;
		}
		w -= b * num;
		num /= 10;
	}
}


void uart_putdw_dec(uint32_t dw) {
	uint32_t num = 1000000000;
	uint8_t started = 0;

	while ( num > 0 ) {
		uint8_t b = dw / num;
		if( b > 0 || started || num == 1 ) {
			uart_putc('0' + b);
			started = 1;
		}
		dw -= b * num;
		num /= 10;
	}
}



#ifdef _NVER___
void uart_init() {
//	UBRRH = HI_WORD(UART_RATE_VALUE);
//	UBRRL = LO_WORD(UART_RATE_VALUE);
	//UCSRB = _BV(TXEN) | _BV(RXEN) | _BV(TXCIE) | _BV(RXCIE); /* tx/rx enable, interrupts enable */
	UCSRB = _BV(TXEN) | _BV(RXEN); // tx/rx enable
}


#endif
