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

#ifndef _UART_H_
#define _UART_H_


#include <avr/pgmspace.h>
#include <stdbool.h>

#define UBRR_VALUE_9600			(F_CPU/16/9600 - 1)
#define UBRR_VALUE_115200		(F_CPU/16/115200 - 1)
#define UBRR_VALUE_230400		(F_CPU/16/230400 - 1)




inline void uart_putc(char c) {
	// waiting for sending buffer free
	while ( !(UCSRA & (1 << UDRE)) );
	UDR = c;
}

inline uint8_t uart_getc() {
	while ( !(UCSRA & (1 << RXC)) );
	return UDR;
}

//void uart_puts(const char* str);
//void uart_puts_p(PGM_P str);




void uart_puts_p(PGM_P str);
void uart_puts(const char* str);


void uart_putb_dec(uint8_t b);
void uart_putw_dec(uint16_t w);
void uart_putdw_dec(uint32_t dw);


void uart_putc_hex(uint8_t b);
void uart_putw_hex(uint16_t w);
void uart_putdw_hex(uint32_t dw);





#endif // _UART_H_
