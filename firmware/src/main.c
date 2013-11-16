#include <stddef.h>
#include <avr/io.h>
#include <stdlib.h>
#include <stdbool.h>

#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>

#include <util/delay.h>


uint16_t lightValue;
uint16_t soundValue;

volatile bool task_readTimeFromPCF;
volatile bool task_updateBrightness;

#include "fat.h"
#include "fat_config.h"
#include "partition.h"
#include "sd_raw.h"
#include "sd_raw_config.h"



#include "config.h"
#include "uart.h"
#include "debug.h"
#include "pcf8583.h"
//#include "i2c.h"

#include "display.h"
#include "adc.h"
#include "clock.h"
#include "bluetooth.h"

#include "sd_card_reader.h"

#include "light_sensor.h"

#include "audio.h"
#include "control.h"



#include <math.h>


// -ffunction-sections -fdata-sections
// ${COMMAND}  ${FLAGS} ${OUTPUT_FLAG} ${OUTPUT_PREFIX}${OUTPUT} ${INPUTS}
// COMPILER -ffunction-sections
// LINKER -Wl,-gc-sections

//You may recall the Makefile flags were:
//Code:
//CFLAGS += -ffunction-sections
//LDFLAGS += -Wl,-gc-sections



static inline void init() {
	display_Init();
	bt_Init();
	bt_ResetPulse();
	audio_disable();
	adcStart(ADC_LIGHT);
	adcStart(ADC_MICROPHONE);

	sei();

	clock_readTimeFromPcf8583();


}


ISR(TIMER0_COMP_vect) {
//	OCR0 += display_timer0deltaCount;
	OCR0 = display_timer0deltaCount;
	onTick();


//	if ( time_sec < 30 ) {
//		static bool c = false;
//		audio_out( c ? 1023 : 0);
//		c = !c;
//	}
//	audio_out(0);
}



ISR(TIMER2_COMP_vect) {
	OCR2 = display_timer2deltaCount;
	display_Update();

	/*
	if ( time_sec < 30 ) {
		static bool c = false;
		static uint8_t cnt = 0;
		if (++cnt > 44) {
			audio_out( c ? 1023 : 0);
			c = !c;
			cnt = 0;
		}
	}
	*/

	//static uint8_t cccnt = 0;
	//if (++cccnt == 3 && sdcard_fd) {
	//	cccnt = 0;
		uint8_t v = sdrdr_nextByte();
		uint16_t v16 = v;
		//v16 *= 4;
		audio_out(v16);
//	} else if (time_sec == 0) {
//		if ( sdrdr_init() ) {
//			sdrdr_openFile();
//			audio_enable();
//		}
	//}

/*
	static uint16_t indx = 0;
	audio_out(audio_nextSin(indx++));
	if ( indx >= 87 ) {
		indx = 0;
	}
*/
}


inline void onIdle() {
	if ( task_readNextBlock ) {
		sdrdr_readNext();
		//sdrdr_read_file(sdcard_fd, 512*10);
	} else if ( task_readTimeFromPCF ) {
		clock_readTimeFromPcf8583();
		task_readTimeFromPCF = false;
	} else if ( task_updateBrightness ) {
		setAutoBright();
		task_updateBrightness = false;
	}
}






inline void onSoundMeassure(uint16_t v) {
	soundValue = v;
}


int main(void) {
	init();

	display_setBrightness(100);

	char uartBuffer[64];
	uint8_t uartPos = 0;
	while ( true ) {
		if ( !(UCSRA & (1 << RXC)) ) {
			onIdle();
			continue;
		}
		uint8_t ch = UDR;//uart_getc();
		if ( ch == 0 || ch == '\n' ) {
			uartBuffer[uartPos] = 0;
			onCommand(uartBuffer);
			uartPos = 0;
		} else {
			uartBuffer[uartPos++] = ch;
		}
	}


}


/*

//		displayOutTime(x1, x2);
		uint8_t b1 = uart_getc();
		uint8_t b2 = uart_getc();
		//uart_putc(b1);
//		buf[bufPos++] = b1;
//		buf[bufPos++] = b2;
		if (b1 == 'r' && b2 == 's') {
			bt_Reset(true);
			_delay_ms(500);
			bt_Reset(false);
		}
		if (b1 == 'h' && b2 == 's') {
			for (uint8_t i =0; i < bufPos; i++) {
				uart_putc(buf[i]);
			}
		}
		if (b1 == 'n' && b2 == 'm') {

			//bluetoothResetPulse();
			bluetoothReset(true);			_delay_ms(500);			bluetoothReset(false);

			_delay_ms(40000);
			bluetoothSetName(PSTR(BLUETOOTH_NAME));
			_delay_ms(1500);

			//bluetoothResetPulse();
			bluetoothReset(true);			_delay_ms(500);			bluetoothReset(false);

		}

//		displayOutTime(b1, b2);
*/
