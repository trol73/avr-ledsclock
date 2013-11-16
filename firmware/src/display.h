/*
 * display.h
 *
 *  Created on: Feb 17, 2013
 *      Author: trol
 */

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "bresenham.h"

#define EMPTY	10

#define BRIGHTNESS_MAX	100

volatile uint8_t videoMemorySegments[4];
volatile uint8_t videoMemoryPoints[4];
volatile uint8_t videoMemoryAnodes[4+7];
volatile bool videoMemoryDotEnabled;

volatile uint8_t display_timer0deltaCount;
volatile uint8_t display_timer2deltaCount;
volatile uint8_t display_katode;
volatile uint8_t display_brightness = BRIGHTNESS_MAX;
volatile bool display_autoBrightness = true;

static inline void display_OutTime(uint8_t hour, uint8_t min);
static inline void display_Dots(bool on);

bresenham_struct bresenhamBrightness;


PROGMEM const uint8_t SEGMENT_ANODE_CODES[11] = {
	0b01110111,		// 0
	0b00100100,		// 1
	0b01011101,		// 2
	0b01101101,		// 3
	0b00101110,		// 4
	0b01101011,		// 5
	0b01111011,		// 6
	0b00100101,		// 7
	0b01111111,		// 8
	0b01101111,		// 9
	0b00000000		// EMPTY
};


PROGMEM const uint8_t POINT_ANODE_CODES[11] = {
	0b00001100,		// 0
	0b00101010,		// 1
	0b00000000,		// 2
	0b00000000,		// 3
	0b00101111,		// 4
	0b00000000,		// 5
	0b00000100,		// 6
	0b00101010,		// 7
	0b00000000,		// 8
	0b00001000,		// 9
	0b00000000		// EMPTY
};




static inline void display_Init() {
	DDRA = 0b00011111;
	DDRB = 0b00001111;
	DDRC = 0b11111111;
	DDRD = 0b01110110;

	// set up TC0, 100 Hz
	display_timer0deltaCount = F_CPU/1024/100;	// interrupt will be called 100 times per second
	TCCR0 = _BV(WGM01)|_BV(CS02)|_BV(CS00);		// normal mode, divider = 1024
	OCR0 = display_timer0deltaCount;
	TCNT0 = 0;
	TIMSK |= _BV(OCIE0);						// enable compare interrupt for OCR0

	// set up TC2, ~51 KHz
//	display_timer2deltaCount = F_CPU/8/51200;	// interrupt will be called 51 200 times per second
	display_timer2deltaCount = F_CPU/8/43200;	// interrupt will be called 43 200 times per second

	TCCR2 = _BV(WGM21)|_BV(CS21);				// normal mode, divider = 8
	OCR2 = display_timer2deltaCount;
	TCNT2 = 0;
	TIMSK |= _BV(OCIE2);						// enable compare interrupt for OCR2

	bresenham_init(&bresenhamBrightness, BRIGHTNESS_MAX);
}



/**
 * Select cathode:
 * 		0..6   - for segments
 * 		7..10  - for points
 */
static inline void selectCathode(uint8_t val) {
	if ( val < 5 ) {
		PORTA |= _BV(val);
	} else if ( val < 7 ) {
		PORTD |= _BV(val);
	} else {
		PORTB |= _BV(val-7);
	}
}

static inline void disableCathodes() {
	PORTA &= 0b11100000;
	PORTB &= 0b11110000;
	PORTD &= 0b10011111;
}


static inline void setAnodeLevels(uint8_t val) {
	PORTC &= 0b00000011;
	PORTC |= val;
//	PORTC |= val << 2;
}




static inline uint8_t calcAnodesValue(uint8_t kathode) {
	if ( kathode < 7 ) {
		uint8_t anodes = 0xff;
		for ( uint8_t bit = 0; bit < 4; bit++ ) {
			if ( videoMemorySegments[bit] & _BV(kathode) ) {
				anodes &= ~_BV(bit);
			}
		}
		if ( videoMemoryDotEnabled ) {
			anodes &= ~_BV(4);
			anodes &= ~_BV(5);
		}
		return anodes;
	}
	return ~videoMemoryPoints[kathode-7];
}


static inline void displaySet(uint8_t index, uint8_t val) {
	videoMemorySegments[index] = pgm_read_byte(&SEGMENT_ANODE_CODES[val]);
	videoMemoryPoints[index] = pgm_read_byte(&POINT_ANODE_CODES[val]);
}

static inline void display_Dots(bool on) {
	if ( on != videoMemoryDotEnabled ) {
		videoMemoryDotEnabled = on;
		for ( uint8_t k = 0; k < 7; k++ ) {
			videoMemoryAnodes[k] = calcAnodesValue(k) << 2;
		}
	}
}


static inline void display_OutTime(uint8_t hour, uint8_t min) {
	displaySet(0, hour < 10 ? EMPTY : hour / 10);
	displaySet(1, hour % 10);
	displaySet(2, min / 10);
	displaySet(3, min % 10);
	for ( uint8_t k = 0; k < 7+4; k++ ) {
		videoMemoryAnodes[k] = calcAnodesValue(k) << 2;
	}
}


static inline void display_Update() {
	static uint8_t tick = 0;
	static bool visible = true;
	if ( display_katode == 0 ) {
		if ( ++tick >= BRIGHTNESS_MAX) {
			tick = 0;
		}
		visible = bresenham_getNext(&bresenhamBrightness);
		if ( !visible ) {
			disableCathodes();
		}
	}
	if ( ++display_katode >= 7+4 ) {
		display_katode = 0;
	}
	if ( visible ) {
		disableCathodes();
		setAnodeLevels(videoMemoryAnodes[display_katode]);
		selectCathode(display_katode);
	}
}


void display_setBrightness(uint8_t brightness) {
	if ( brightness > BRIGHTNESS_MAX ) {
		brightness = BRIGHTNESS_MAX;
	}
	display_brightness = brightness;
	bresenham_setValue(&bresenhamBrightness, display_brightness);
}

#endif // _DISPLAY_H_
