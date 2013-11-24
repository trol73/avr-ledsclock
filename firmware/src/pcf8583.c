/*
 * Copyright (c) 2010-2013 by Oleg Trifonov <otrifonow@gmail.com>
 *
 * http://trolsoft.ru
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include "pcf8583.h"

static uint8_t bcd2int(uint8_t val) {
	return ((val >> 4) & 0x0F)*10 + (val & 0x0F);
}

static uint8_t int2bcd(uint8_t val) {
	return ((val / 10) << 4) + val % 10;
}

// получить значение времени из PCF 8583
bool pcf8583_getTime(uint8_t *hour, uint8_t *min, uint8_t *sec, uint8_t *hsec) {
	i2c_init();
	uint8_t b1, b2, b3, b4;

	if ( !i2c_start() ||
		!i2c_sendAddress(PCF8583_ADDRESS | I2C_WRITE) ||
		!i2c_sendByte(PCF8583_REG_HSECONDS) ||
		!i2c_stop() ||

		!i2c_start() ||
		!i2c_sendAddress(PCF8583_ADDRESS | I2C_READ) ||
		!i2c_receiveByteACK(&b1) ||
		!i2c_receiveByteACK(&b2) ||
		!i2c_receiveByteACK(&b3) ||
		!i2c_receiveByteNACK(&b4) ||
		!i2c_stop()	)
	{
		MSG("ERR getTime()");
		i2c_stop();
		return false;
	}

	*hsec = bcd2int(b1);
	*sec = bcd2int(b2);
	*min = bcd2int(b3);
	*hour = bcd2int(b4);

	return true;
}

bool pcf8583_setTime(uint8_t hour, uint8_t min, uint8_t sec, uint8_t hsec) {
	i2c_init();

	if ( !i2c_start() ||
		!i2c_sendAddress(PCF8583_ADDRESS | I2C_WRITE) ||
		!i2c_sendByte(PCF8583_REG_HSECONDS) ||

		!i2c_sendByte(int2bcd(hsec)) ||
		!i2c_sendByte(int2bcd(sec)) ||
		!i2c_sendByte(int2bcd(min)) ||
		!i2c_sendByte(int2bcd(hour)) ||
		!i2c_stop()	)
	{
		MSG("ERR setTime()");
		i2c_stop();
		return false;
	}

	return true;
}



bool pcf8583_getDate(uint8_t *year, uint8_t *month, uint8_t *day) {
	uint8_t b1, b2;

	i2c_init();

	if ( !i2c_start() ||
		!i2c_sendAddress(PCF8583_ADDRESS | I2C_WRITE) ||
		!i2c_sendByte(PCF8583_REG_YEAR_DAYS) ||
		!i2c_stop() ||

		!i2c_start() ||
		!i2c_sendAddress(PCF8583_ADDRESS | I2C_READ) ||
		!i2c_receiveByteACK(&b1) ||
		!i2c_receiveByteNACK(&b2) ||
		!i2c_stop()	)
	{
		MSG("ERR getDate()");
		i2c_stop();
		return false;
	}

	*year = (b1 >> 6) & 3;
	*month = bcd2int(b2 & 0b00011111);
	*day = bcd2int(b1 & 0b00111111);
	MSG_HEX("get b1 ", b1, 1);
	MSG_HEX("get b2 ", b2, 1);
	MSG_DEC("get year ", *year);
	MSG_DEC("get month ", *month);
	MSG_DEC("get day ", *day);

	return true;
}



bool pcf8583_setDate(uint8_t year, uint8_t month, uint8_t day) {
	i2c_init();
	uint8_t b1 = int2bcd(day) | (year << 6);
	uint8_t b2 = int2bcd(month);

	if ( !i2c_start() ||
		!i2c_sendAddress(PCF8583_ADDRESS | I2C_WRITE) ||
		!i2c_sendByte(PCF8583_REG_YEAR_DAYS) ||

		!i2c_sendByte(b1) ||
		!i2c_sendByte(b2) ||
		!i2c_stop()	)
	{
		MSG("ERR setDate()");
		i2c_stop();
		return false;
	}
	MSG_HEX("set b1 ", b1, 1);
	MSG_HEX("set b2 ", b2, 1);
	MSG_DEC("set year ", year);
	MSG_DEC("set month ", month);
	MSG_DEC("set day ", day);

	return true;
}

