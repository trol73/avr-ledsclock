/*
 * audio.h
 *
 *  Created on: Mar 10, 2013
 *  Author: trol
 */

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include "stddef.h"


inline void audio_disable() {
	TCCR1A = 0;
	DDRD &= ~_BV(4);
}


inline void audio_enable() {
	DDRD |= _BV(4);			// PD4 - audio output
//	TCCR1A = _BV(WGM11)|_BV(WGM10)|_BV(COM1B1);
	TCCR1A = _BV(WGM10)|_BV(COM1B1);
	TCCR1B = _BV(CS10)|_BV(WGM12);

	OCR1B = 0;
}


inline void audio_out(uint16_t val) {
	OCR1B = val;
}


#endif // _AUDIO_H_
