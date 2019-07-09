;-------------------------------------------------------------------------
;			automatic baud rate detection
;-------------------------------------------------------------------------
;
; recognize any byte,
; which contain 1 * bit time low, followed by 4 * bit times low
;                      ____    __    __ __             ____
;e.g. recognize 0x0D:      |__|  |__|  |  |__|__|__|__|
;                          0  1  2  3     5           9
;                                1*T               4*T
;-------------------------------------------------------------------------
; in par:	twh:twl
; out par:	baudh:baudl
; wregs:	r0, y, z; a0, twl:twh, zeroh:zerol(ro)
; const:	BootDelay, UartLoop
; calls:	timeout
; macros:	SKIP_RXD_0, SKIP_RXD_1
; extension:	uses x register when SKIP_RXD_1 uses memory mapped i/o.
;-------------------------------------------------------------------------
; 2013-07-09    <michael.hoffmann AT fh-dortmund.de>
;	* Improvements on bitrate recognition and timing
;
; 2013-07-08    <michael.hoffmann AT fh-dortmund.de>
;	* New versions of SKIP_RXD_? were added which are able to handle
;	i/o addresses outside 0x00-0x1F by switching to memory mapped i/o.
;	The new version of SKIP_RXD_1 makes the _aba4 loop slower, so
;	the original measurement of cycles taking place in the 4*T bit
;	low state fails.  To avoid measurement of alternative bit
;	patterns (you would have to change the pc loader software passwd)
;	or larger calculations (bloats up uC firmware) a new counter of
;	cycles 'x' has been added.
;-------------------------------------------------------------------------
#define  TOLERANCE 3
#define  MINTIME 90
;
;-------------------------------------------------------------------------
#if STX_PORT <= 0x1F // this has been done ever before
;-------------------------------------------------------------------------
abaud:
	ldi	a0, byte3(BootDelay / 6)
_aba1:
	movw	zl, zerol	; cause first try invalid
_aba2:
	movw	yl, zl
	movw	zl, zerol	; z = 0x0000
_aba3:
	sbiw	twl, 1		;2
	sbci	a0, 0			;1
	SKIP_RXD_0			;1	wait until RXD = 0
	brne	_aba3			;2 = 6
	breq	timeout
_aba4:
	sbiw	yl, 1		;2
	adiw	zl, 4		;2	count bit time
	brcs	timeout			;1	time to long
	SKIP_RXD_1			;1 	wait until RXD = 1
	rjmp	_aba4			;2 = 8
;------------------------------ correction for USB dongle !!! ------------
	mov	r0, zh
_aba5:
	asr	yl			; shift signed !
	lsr	r0
	brne	_aba5
;-------------------------------------------------------------------------
	sbiw	yl, TOLERANCE
	adiw	yl, 2 * TOLERANCE
	brcc	_aba2			; outside tolerance
	cpi	zl, MINTIME
	cpc	zh, zerol
	brcs	_aba2			; time to short
	sbiw	zl, 2*UartLoop-0x05	; rounding, -loop time
	lsr	zh			; /2
	ror	zl
	movw	baudl, zl
;-------------------------------------------------------------------------
#else // TX_PORT > 0x1F -> memory mapped i/o slows down SKIP_RXD_[0|1]
;-------------------------------------------------------------------------
abaud:
	ldi	a0, byte3(BootDelay / 6)
_aba1:
	movw	zl, zerol	; cause first try invalid
_aba2:
	movw	xl, zerol 	; cycle counter in x
	movw	yl, zl		; previous bit measurement in y
	movw	zl, zerol	; current bit measurement in z
_aba3:
	sbiw	twl, 1		;2
	sbci	a0, 0			;1
	SKIP_RXD_0			;3	wait until RXD = 0
	brne	_aba3			;2 => 8
	breq	_timeout
_aba4:
	adiw	xl, 6	;2	count cycles while bit low state
	sbiw	yl, 1	;2
	adiw	zl, 4	;2	count bit time
	brcs	_timeout	;1	time to long
	SKIP_RXD_1		;3 	wait until RXD = 1
	rjmp	_aba4		;2 => 12 cycles per _aba4-loop
;------------------------------ correction for USB dongle !!! ------------
	mov	r0, zh
_aba5:
	asr	yl			; shift signed !
	lsr	r0
	brne	_aba5
;-------------------------------------------------------------------------
	sbiw	yl, TOLERANCE
	adiw	yl, 2 * TOLERANCE
	brcc	_aba2			; outside tolerance
	cpi	zl, MINTIME
	cpc	zh, zerol
	brcs	_aba2			; time to short
	sbiw	xl, 2*UartLoop-0x07
	lsr	xh			; /4 because 4 bits were measured
	ror	xl
	movw	baudl, xl
;-------------------------------------------------------------------------
#endif
;-------------------------------------------------------------------------
;inlined	ret
;-------------------------------------------------------------------------
