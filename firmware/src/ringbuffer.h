/*
 * ringbuffer.h
 *
 *  Created on: Mar 22, 2013
 *      Author: trol
 */

#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#include <stdbool.h>

#define RINGBUFFER_SIZE		0xff

uint8_t ringbuffer_data[RINGBUFFER_SIZE];
volatile uint8_t rinbuffer_pos_read;
volatile uint8_t rinbuffer_pos_write;



/**
 *
 */
inline void rb_reset() {
	rinbuffer_pos_read = 0;
	rinbuffer_pos_write = 0;
}


/**
 * Returns number of read bytes in the buffer
 */
inline uint8_t rb_read_size() {
	if ( rinbuffer_pos_read < rinbuffer_pos_write ) {
		return rinbuffer_pos_write - rinbuffer_pos_read;
	} else {
		return rinbuffer_pos_read - rinbuffer_pos_write;
	}
}

/**
 *
 */
inline uint8_t rb_write_avail() {
	return RINGBUFFER_SIZE - 1- rb_read_size();
}



inline void rb_push(uint8_t v) {
	ringbuffer_data[rinbuffer_pos_write] = v;
	if ( ++rinbuffer_pos_write >= RINGBUFFER_SIZE ) {
		rinbuffer_pos_write = 0;
	}
}

inline uint8_t rb_pop() {
	uint8_t result = ringbuffer_data[rinbuffer_pos_read];
	if ( ++rinbuffer_pos_read >= RINGBUFFER_SIZE ) {
		rinbuffer_pos_read = 0;
	}
	return result;
}



#endif // _RINGBUFFER_H_
