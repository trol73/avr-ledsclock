/**
 * Функции работы с датой и временем, а также таймер часов и функции корректировки времени.
 * Все функции калькуляции дат работают для года от 2000 до 2100
 */
#ifndef _TIME_H_
#define _TIME_H_

uint16_t timer1_delta_count;			// через сколько тиков таймера1 будет срабатывать прерывание
uint32_t time_correct_inc_interval;		// интервал в сотых долях секунды, через который будет корректироваться время
uint32_t time_hsec_counter;				// счетчик сотых долей секунды, сбрасывается при корректировке


// сколько дней в месяце (для високосного года в фервале 29 дней)
PROGMEM const uint8_t _time_monthDays[12] = {
	31,		// январь
	28,		// февраль
	31,		// март
	30,		// аперль
	31,		// май
	30,		// июнь
	31,		// июль
	31,		// август
	30,		// сентябрь
	31,		// октябрь
	30,		// ноябрь
	31		// декабрь
};


// этот массив используется для определения дня недели
PROGMEM const uint8_t _time_m_array[12] = {
	0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5
};



static void onHSecondsChanged(void);	// обработчик, вызывается при смене сотых долей секунды текущего времени
static void onSecondChanged(void);		// обработчик, вызывается при смене секунды текущего времени
static void onMinuteChanged(void);		// обработчик, вызывается при смене минуты текущего времени
static void onDayChanged(void);		// обработчик, вызывается при смене дня

static uint8_t time_daysPerMonth();


void time_init_correction() {
	// сотые доли секунды надо инкрементировать каждые .. тиков компаратора таймера
	if ( settings_time_correction_hs != 0 ) {
		// 24*60*60*100
		time_correct_inc_interval = 8640000/settings_time_correction_hs;
	} else {
		time_correct_inc_interval = 0xffffffff;
	}
	time_hsec_counter = 0;
}


static void time_init() {
	timer1_delta_count = F_CPU/8/100;	// прерывание будет возникать ровно 100 раз в секунду

	// конфигурируем T/C1
	TCCR1A = 0;				// ШИМ отключен, пины отключены
	TCCR1B = _BV(CS11);		// на вход таймера подается TCK/8
	OCR1A = timer1_delta_count;
	TIMSK |= _BV(OCIE1A);	// включить прерывание по совпадению для OCR1A
	TCNT1H = 0;				// сброс счетчика таймера, именно в этой последовательности
	TCNT1L = 0;

	time_init_correction();
}



// прерывание сравнение таймера, вызывается 100 раз в секунду
ISR(TIMER1_COMPA_vect) {
	OCR1A += timer1_delta_count;

	time_hsec_counter++;
	if ( time_hsec_counter >= time_correct_inc_interval ) {
		time_hsec_counter = 0;
		if ( settings_time_correction_plus )
			time_hsec += 2;
		needSaveTimeToPCF8583 = true;
	} else
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
					if ( time_day > time_daysPerMonth() ) {
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
	onHSecondsChanged();
}

/**
 * Вычисляет день недели для текущей даты
 */
static void time_calcDayOfWeek() {
	uint8_t y = time_year-1;
	uint8_t m = time_month-1;
	uint8_t t2 = pgm_read_byte(&_time_m_array[m]);

	if ( ((time_year & 3) == 0) && (m >> 1 != 0) )	// месяц после февраля високосного года
		t2++;
	time_dw = (y + (y >> 2) + t2 + time_day) % 7;
	if ( time_dw == 0 )
		time_dw = 7;
}


static uint8_t time_daysPerMonth() {
//	if ( ((time_year % 4) == 0) && (time_month == 2) )
	if ( ((time_year & 3) == 0) && (time_month == 2) )
		return 29;
	return pgm_read_byte(&_time_monthDays[time_month-1]);
}

#endif // _TIME_H_
