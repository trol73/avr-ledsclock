/*
 * adc.h
 *
 * Analog-to-digital converter
 *
 *  Created on: 26.03.2013
 *  Author: Oleg Trifonov
 */

#ifndef _ADC_H_
#define _ADC_H_

#define ADC_MICROPHONE			0b00000110
#define ADC_LIGHT				0b00000111



inline void onLightMeassure(uint16_t v);
inline void onSoundMeassure(uint16_t v);


/**
 * Start ADC meassure
 */
inline void adcStart(uint8_t pin) {
	// ADMUX = pin|_BV(REFS0)|_BV(REFS1);	INT 2.56V source
	ADMUX = pin;	// AVREF
	ADCSRA = _BV(ADEN)|_BV(ADSC)|_BV(ADIE)|_BV(ADPS2)|_BV(ADPS1)|_BV(ADPS0);
}


ISR(ADC_vect) {
	uint8_t admux = ADMUX & 0b00000111;
	uint16_t v;

	v = ADCL;
	v += ADCH << 8;
	if ( admux == ADC_LIGHT ) {
		onLightMeassure(v);
	} else if ( admux == ADC_MICROPHONE ) {
		onSoundMeassure(v);
//	} else {
//		adcStart(ADC_LIGHT);
	}
}




#endif // _ADC_H_
