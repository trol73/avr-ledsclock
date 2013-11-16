/*
 * light_sensor.h
 *
 *  Created on: Mar 30, 2013
 *  Author: trol
 */

/*
var light = 1023 - x;
var res;
if (light <= 50) res = 2 + 3*(light)/50
else if (light <= 100) res = 6 + 9*(light-50)/50
else if (light <= 200) res = 16 + 14*(light-100)/100
else res = 31+(67)*(light-200)/(1023-229);
res

0 1024 40
 */

#ifndef _LIGHT_SENSOR_H_
#define _LIGHT_SENSOR_H_

static PROGMEM const uint8_t table_light2brightness[27] = {
	100, 97,  94,  90,  87,  84,  80,  77,  73,  70,  67,  63,  60,  57,  53,  50, 46,  43,  40,  36,  33,  28,  22,  16,  8,   3,   1
};

uint8_t calculateLight2brightness(uint16_t x) {
	if ( x <= 0 ) {
		return pgm_read_byte(&table_light2brightness[0]);
	}
	uint8_t index = x/40;
	if ( index >= 26 ) {
		return pgm_read_byte(&table_light2brightness[26]);
	}
	uint8_t y1 = pgm_read_byte(&table_light2brightness[index]);
	uint8_t y2 = pgm_read_byte(&table_light2brightness[index+1]);
	int16_t dy = (y2 - y1);
	dy *= (x - index*40);
	dy /= 40;
	return y1 + dy;
}

inline void onLightMeassure(uint16_t v) {
	lightValue = v;
	task_updateBrightness = true;
}


void setAutoBright() {
	if ( display_autoBrightness ) {
		static uint8_t averageBrightness = 100;

		uint8_t br = calculateLight2brightness(lightValue);

		uint16_t newBr = br + 3*averageBrightness;
		newBr /= 4;
		uint8_t brightness = newBr;
		averageBrightness = brightness;

		if ( brightness != display_brightness && time_hsec < 2 && time_sec % 5 == 0 ) {
			display_setBrightness(brightness);
		}
	}
}



#endif // _LIGHT_SENSOR_H_
