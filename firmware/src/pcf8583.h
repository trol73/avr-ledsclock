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

#ifndef _PCF8583_H_
#define _PCF8583_H_

#include "debug.h"
#include "i2c.h"

#define PCF8583_ADDRESS		0b10100000

// addresses of memory cells for PCF8583
#define PCF8583_REG_CONTROL				0
#define PCF8583_REG_HSECONDS			1
#define PCF8583_REG_SECONDS				2
#define PCF8583_REG_MINUTES				3
#define PCF8583_REG_HOURS				4
#define PCF8583_REG_YEAR_DAYS			5
#define PCF8583_REG_MONTHS_WEEK_DAYS	6

/*
#define PCF8583_REG_MONTHS				7
#define PCF8583_REG_YEARS				8
#define PCF8583_REG_ALARM_MIN			9
#define PCF8583_REG_ALARM_HOUR			10
#define PCF8583_REG_ALARM_DAY			11
#define PCF8583_REG_ALARM_WEEK_DAY		12
*/

// configurate bits (for the cell PCF8583_REG_CONTROL)
//#define PCF8583_CFG_TIMER_FLAG			0	// 50% duty factor seconds flag if alarm enable bit is 0)
//#define PCF8583_CFG_ALARM_FLAG			1	// 50% duty factor minutes flag if alarm enable bit is 0)
#define PCF8583_CFG_ALARM_ENABLE_BIT	2	// установка этого флага включает будильник


bool pcf8583_getTime(uint8_t *hour, uint8_t *min, uint8_t *sec, uint8_t *hsec);

bool pcf8583_setTime(uint8_t hour, uint8_t min, uint8_t sec, uint8_t hsec);

bool pcf8583_getDate(uint8_t *year, uint8_t *month, uint8_t *day);

bool pcf8583_setDate(uint8_t year, uint8_t month, uint8_t day);



#endif // _PCF8583_H_
