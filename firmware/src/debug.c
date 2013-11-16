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

#include "debug.h"
#include "uart.h"

void MSG_(PGM_P str) {
#if DEBUG
	uart_puts_p(str);
	uart_putc('\n');
#endif
}

void MSG_STR(char *str) {
#if DEBUG
	uart_puts(str);
	uart_putc('\n');
#endif
}

void MSG_HEX_(PGM_P str, uint32_t val, uint8_t bytes) {
#if DEBUG
	uart_puts_p(str);
	if ( bytes == 1 )
		uart_putc_hex(val);
	else if ( bytes == 2 )
		uart_putw_hex(val);
	else if ( bytes == 4 )
		uart_putdw_hex(val);
	uart_putc('\n');
#endif
}


void MSG_DEC_(PGM_P str, uint32_t val) {
#if DEBUG
	uart_puts_p(str);
	uart_putdw_dec(val);
	uart_putc('\n');
#endif
}

