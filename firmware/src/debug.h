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


#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "config.h"

#include <avr/pgmspace.h>


void MSG_(PGM_P str);

void MSG_STR(char *str);
void MSG_HEX_(PGM_P str, uint32_t val, uint8_t bytes);
void MSG_DEC_(PGM_P str, uint32_t val);


#define MSG(str)							MSG_(PSTR(str))
#define MSG_HEX(str, val, bytes)			MSG_HEX_(PSTR(str), (val), bytes)
#define MSG_DEC(str, val)					MSG_DEC_(PSTR(str), (val))





#endif // _DEBUG_H_
