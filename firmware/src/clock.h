/*
 * clock.h
 *
 *  Created on: Feb 24, 2013
 *      Author: trol
 */

#ifndef _CLOCK_H_
#define _CLOCK_H_

//#include <stdbool.h>

uint16_t timer1_delta_count;			// how many ticks of timer 1 will trigger an interrupt
uint32_t time_correct_inc_interval;		// interval for time correction (hundredths of a second)
uint32_t time_hsec_counter;				// hundredths of a second counter, reset on correction

bool needSaveTimeToPCF8583 = false;		// if true then we need to save time to PCF8583
bool settings_time_correction_plus;		// if true then time correction value > 0, else time correction value < 0
uint16_t settings_time_correction_hs = 0;

uint8_t time_month;
uint8_t time_day;
uint8_t time_hour;
uint8_t time_min;
uint8_t time_sec;
uint8_t time_hsec;
uint16_t time_year;
uint8_t time_dw;

//volatile bool pcf8583_readed = false;
uint8_t settings_time_year_base; // the current year (since 2000) is calculated as settings_time_year_base * 4 + <year from pcf8535>

// dayes per month (for the leap year the February has 29 days)
PROGMEM const uint8_t _clock_monthDays[12] = {
	31,		// January
	28,		// February
	31,		// Mart
	30,		// April
	31,		// May
	30,		// June
	31,		// July
	31,		// August
	30,		// September
	31,		// October
	30,		// November
	31		// December
};


// this array is used for calculation of the day of the week
PROGMEM const uint8_t _clock_m_array[12] = {
	0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5
};

static inline void onDayChanged();
static inline void onHourChanged();
static inline void onMinuteChanged();
static inline void onSecondChanged();
static inline void onHSecondChanged();

static uint8_t clock_daysPerMonth();


static inline void onTick() {
/*
	time_hsec_counter++;
	if ( time_hsec_counter >= time_correct_inc_interval ) {
		time_hsec_counter = 0;
		if ( settings_time_correction_plus ) {
			time_hsec += 2;
		}
		needSaveTimeToPCF8583 = true;
	} else {
		time_hsec++;
	}
*/
time_hsec++;

	if ( time_hsec >= 100 ) {
		time_hsec -= 100;
		time_sec++;
		if ( time_sec == 60 ) {
			time_sec = 0;
			time_min++;
			if ( time_min == 60 ) {
				time_min = 0;
				time_hour++;
				if ( time_hour == 24 ) {
					time_hour = 0;
					time_hsec_counter = 0;
					time_day++;
					if ( time_day > clock_daysPerMonth() ) {
						time_day = 1;
						time_month++;
						if ( time_month > 12 ) {
							time_month = 1;
							time_year++;
						}
					}
					onDayChanged();
				}
			}
			onMinuteChanged();
		}
		onSecondChanged();
	}
	onHSecondChanged();
}



static inline void onDayChanged() {

}

static inline void onHourChanged() {
//	clock_updateTime();
}

static inline void onMinuteChanged() {
//	clock_updateTime();
	display_OutTime(time_hour, time_min);
}

static inline void onSecondChanged() {
	if ( time_sec == 30 ) {
		task_readTimeFromPCF = true;
	}
}

static inline void onHSecondChanged() {
	display_Dots(time_hsec < 50);
	if ( time_hsec % 2 ) {
		adcStart(ADC_MICROPHONE);
	} else {
		adcStart(ADC_LIGHT);
	}
}

static void clock_init_correction() {
	// increment hundredths of a second every  .. tics of counter
	if ( settings_time_correction_hs != 0 ) {
		// 24*60*60*100
		time_correct_inc_interval = 8640000/settings_time_correction_hs;
	} else {
		time_correct_inc_interval = 0xffffffff;
	}
	time_hsec_counter = 0;
}


inline static void clock_init() {
	clock_init_correction();
}

/**
 * Calculates day of week for current date
 */
static void clock_calcDayOfWeek() {
	uint8_t y = time_year-1;
	uint8_t m = time_month-1;
	uint8_t t2 = pgm_read_byte(&_clock_m_array[m]);

	if ( ((time_year & 3) == 0) && (m >> 1 != 0) )	{ // month after February for leap year
		t2++;
	}
	time_dw = (y + (y >> 2) + t2 + time_day) % 7;
	if ( time_dw == 0 ) {
		time_dw = 7;
	}
}


static uint8_t clock_daysPerMonth() {
//	if ( ((time_year % 4) == 0) && (time_month == 2) )
	if ( ((time_year & 3) == 0) && (time_month == 2) ) {
		return 29;
	}
	return pgm_read_byte(&_clock_monthDays[time_month-1]);
}

void clock_readTimeFromPcf8583() {
	if ( pcf8583_getTime(&time_hour, &time_min, &time_sec, &time_hsec) ) {
		time_year += settings_time_year_base * 4;
		clock_calcDayOfWeek();
		onMinuteChanged();
	}
}
#endif // _CLOCK_H_
