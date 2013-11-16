;;; tboot loader version 1.0
;;;
;;; Based on Wim Lewis SSBL bootloader http://www.hhhh.org/wiml/proj/avr/ssbl.html
;;;

#define __SFR_OFFSET 0

#include <avr/io.h>


#define GLUE(a, b)     a##b
#define PORT(x)        GLUE(PORT, x)
#define PIN(x)         GLUE(PIN, x)
#define DDR(x)         GLUE(DDR, x)


#define FULL_ECHO_MODE		1


;FOSC = 1000000			; factory-default internal osc setting
;FOSC = 8000000			; (8MHz internal osc)
FOSC = 11059200			; 

;BAUDRATE = 115200		; set this to whatever you want
BAUDRATE = 230400			; set this to whatever you want





UBR = (FOSC / 16 / BAUDRATE) - 1

.section .init0
	
.global normal_start
normal_start:
	; exit from the bootloader and run main firmware
		
.section .boot
bootloader_start:
	; enable bluetooth
	ldi		R16, 4
	out		DDRD, r16
	out		PORTD, r16
	
	;; Set up the UART
	ldi		R24, lo8(UBR)
	out		UBRRL, R24
	ldi		R24, hi8(UBR)
	out		UBRRH, R24


	ldi		R24, (1<<RXEN) | (1<<TXEN)
	out		UCSRB, R24

	;; Initialize the stack pointer
	;; To save a few bytes, we also jump back here on errors, instead
	;; of unwinding the stack. The T flag is set to indicate that we've
	;; done that (so we emit the error response instead of the
	;; initial ready response).
	clt
reset_stack_and_putch:	

	ldi		R24, lo8(RAMEND)	;  SP <-- RAMEND
	ldi		R25, hi8(RAMEND)
	out		SPL, R24
	out		SPH, R25

	brts		putch_and_continue				; jump if T == 1
	;; greeting msg - "ok"
	ldi 		r16, 'o'
	rcall 	put_ch
	ldi 		r16, 'k'
	rcall 	put_ch
	
put_ok_and_continue:
	ldi 		r16, 13			; the symbol of the loader response, is sent after each executed command

	;; wait for completion before sending ack byte
putch_and_continue:
	in 		r17, SPMCR
	andi 		r17, (1<<SPMEN)
	brne 		putch_and_continue		; jump if the result is not zerro

	rcall 	put_ch
	;; fall through to command loop

	;; command loop
command_loop:	
	rcall 	get_ch

;; 'Z' command
	cpi		r16, 'Z'						; load Z register
	brne 		not_ldz
	rcall		get_xbyte
	mov 		ZH, r16
	rcall 	get_xbyte
	mov 		ZL, r16
	rjmp		put_ok_and_continue
not_ldz:
	
;; 'R' command
	cpi 		r16, 'R'						; read program memory
	brne		not_lpm
	rcall		get_xbyte					; loading bytes count
	mov		r17, r16
2:	lpm		r16, Z+						; reading memory
	rcall		put_xbyte
	subi		r17, 1
	brne		2b
	rjmp		put_ok_and_continue
not_lpm:

;; 'W' command
	cpi		r16, 'W'						; write program memory
	brne		not_wpm
	rcall		get_xbyte					; loading words count
	mov		r17, r16
	ldi		r18, (1<<SPMEN)
2:	rcall		get_r0r1
	out		SPMCR, r18
	spm
	adiw		ZL, 2
	subi		r17, 1						; words counter decrement
	brne		2b
	rjmp		put_ok_and_continue
not_wpm:

;; 'P' command
	cpi		r16, 'P'						; load SPM control register
	brne		not_spmcr
	rcall		get_r0r1
	rcall		get_xbyte
	out		SPMCR, r16
	spm
	rjmp		put_ok_and_continue
not_spmcr:

;; 'Q' command
	cpi		r16, 'Q'						; query parameters
	brne		not_query_parms
	mov		r16, ZH  						;; -> ZH
	rcall		put_xbyte
	mov		r16, ZL							;; -> ZL
	rcall		put_xbyte
	ldi		r16, hi8(bootloader_start) ;; loader offset hi
	rcall		put_xbyte
	ldi		r16, lo8(bootloader_start)	;; loader offset low
	rcall		put_xbyte
	rjmp		put_ok_and_continue
not_query_parms:

;; '@' command
	cpi		r16, '@'						; jump to address from [ZH:ZL]
	brne		not_icall
	ijmp
not_icall:
	
	;; no more command letters
	;; fall through to error case

	
err:
	ldi		r16, '!'
	set
	rjmp		reset_stack_and_putch


;;;;;;;;;;;;;;;;;;;;;
;;; UART utility
;;;;;;;;;;;;;;;;;;;;;

;; Gets a hex word from UART to [R0:R1] 
get_r0r1:
	rcall		get_xbyte
	mov		r0, r16
	rcall		get_xbyte
	mov		r1, r16
	ret

;; Gets a byte from UART to [R16]
get_ch:
	sbis		UCSRA, RXC 			; check for data ready
	rjmp		get_ch				; if not, keep looping
	in			r16, UDR				; else, retrieve data byte
#if FULL_ECHO_MODE
	rcall		put_ch
#endif
	ret

;; Gets a nybble from UART to [R16]
get_xdigit:
	rcall		get_ch				; retrieve a character in r16
	subi		r16, '0'
	brlt		err					; if ( ch < 0 ) goto error
	cpi		r16, 10				; if ( ch in [0..9] )
	brlt		ok_xdigit			;    return ch
	subi		r16, ('a' - '0')
	brlt		err         		; if ( ch not in [A..F] ) goto error
	cpi		r16, 6				; if ( ch in [0..5] )
	brlt		ok_xletter  		; 		return ch + 10
	rjmp     err
ok_xletter:
	subi		r16, -10
ok_xdigit:
	ret

;; Gets a hex byte from UART to [R16]
get_xbyte:
	rcall		get_xdigit			; high nybble
	swap		r16
	mov		r15, r16
	rcall		get_xdigit        ; low nybble
	or			r16, r15
	ret

;; Sendns a hex byte [R16] to UART
put_xbyte:
	mov		r15, r16
	swap		r16               
	rcall		put_xdigit			; put high nybble
	mov		r16, r15          ; put low nybble

put_xdigit:
	andi		r16, 0x0F			; mask off high byte; 0 ... +15
	subi		r16, 10				; -10 ... +5
	brlt		1f
	subi		r16, (- ('a' - '0' - 10)) ; 
1:	subi		r16, (- (10 + '0'))

;; Sends a byte [R16] to UART
put_ch:
	sbis		UCSRA, UDRE			; check for space in tx fifo
	rjmp		put_ch				; if not, keep looping
	out		UDR, r16				; enqueue data byte
	ret

