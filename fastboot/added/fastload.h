/*
  fastload.h 
  
  Written by Peter Dannegger, modified by H. C. Zimmerer

   Time-stamp: <2010-01-14 21:58:08 hcz>

   You may use my modifications here and in the accompanying files of
   this project for whatever you want to do with them provided you
   don't remove this copyright notice.

*/

;*************************************************************************
#include "compat.h" // compatibility definitions
#include "protocol.h"
;-------------------------------------------------------------------------
;				Constant definitions
;-------------------------------------------------------------------------
#define  VERSION 0x0201

#define  XTAL F_CPU	// 8MHz, not critical
#define  BootDelay XTAL / 3	// 0.33s
#define  BOOTDELAY XTAL / 3

;------------------------------	select UART mode -------------------------
#if SRX == STX && SRX_PORT == STX_PORT
#define  ONEWIRE 3
#else
#define  ONEWIRE 0
#endif

#define  SRX_PIN SRX_PORT - 2
#define  STX_DDR STX_PORT - 1

;------------------------------	select bootloader size -------------------

#ifndef APICALL
#ifndef FirstBootStart
#define  APICALL 0
#else
#define  APICALL 2*12
#endif
#endif

#ifndef CRC
#define  CRC 2*15
#endif

#ifndef VERIFY
#define  VERIFY 2*14
#endif

#ifndef WDTRIGGER
#define  WDTRIGGER 2*9
#endif

#ifndef SPH
#define  MinSize 2*198
#define  MINSIZE 2*198
#else
#define  MinSize 2*203
#define  MINSIZE 2*203
#endif

#define  BootSize CRC + VERIFY + ONEWIRE + WDTRIGGER + MinSize
#define  BOOTSIZE CRC + VERIFY + ONEWIRE + WDTRIGGER + MinSize

;------------------------------	UART delay loop value --------------------

#if STX_PORT > 0x1F
#define  _memmap_delay_ 4
#else
#define  _memmap_delay_ 0
#endif

#if CRC
#define  _crc_delay_ 4
#else
#define  _crc_delay_ 0
#endif

#if FLASHEND > 0xFFFF
#define  _pc22_delay_ 2	// 22-bit PC: 1 rcall + 1 ret cycle overhead
#else
#define  _pc22_delay_ 0
#endif

#define  UartLoop 24 + _memmap_delay_ + _crc_delay_ + _pc22_delay_
#define  UARTLOOP 24 + _memmap_delay_ + _crc_delay_ + _pc22_delay_


;------------------------------	Bootloader fuse setting ------------------
#ifdef FIRSTBOOTSTART
# if (FlashEnd - FirstBootStart) >= 256 // 256 Words needed
#  define  BootStart FirstBootStart
#  define  BOOTSTART FirstBootStart
# else
#  define  BootStart SecondBootStart
#  define  BOOTSTART SecondBootStart
# endif
  ;----------------------------	max possible buffer size -----------------

  .equ  BufferSize,((SRAM_SIZE / 2) - PAGESIZE)
  .macro testpage
    .if		BootStart % BufferSize
      .set BufferSize, BufferSize - PAGESIZE
      .if	BootStart % BufferSize
        .set BufferSize, BufferSize - PAGESIZE
        testpage
      .endif
    .endif
  .endm
	testpage	; calculate BufferSize to fit into BootStart

  ;-----------------------------------------------------------------------
# define  UserFlash (2*BootStart)
# define  USERFLASH (2*BootStart)
#else  /* FirstBootStart not defined */
# ifndef FLASHEND
#  define FLASHEND FlashEnd
# endif
# define  BootStart (FLASHEND - 255)
# define  BOOTSTART (FLASHEND - 255)
# define  BufferSize PageSize
# define  BUFFERSIZE PageSize
# define  UserFlash (2 * BootStart - 2)
# define  USERFLASH (2 * BootStart - 2)
#endif
;-------------------------------------------------------------------------
;				Using register
;-------------------------------------------------------------------------
#define  zerol r2
#define  ZEROL r2
#define  zeroh r3
#define  ZEROH r3
#define  baudl r4	// baud divider
#define  BAUDL r4
#define  baudh r5
#define  BAUDH r5
#define  crcl r6
#define  CRCL r6
#define  crch r7
#define  CRCH r7

;-------------------------------------------------------------------------
#define  appl r16	// rjmp to application
#define  APPL r16
#define  apph r17
#define  APPH r17
#define  polynoml r18	// CRC polynom 0xA001
#define  POLYNOML r18
#define  polynomh r19
#define  POLYNOMH r19

#define  zx r21	// 3 byte Z pointer
#define  ZX r21
#define  a0 r22	// working registers
#define  A0 r22
#define  a1 r23
#define  A1 r23
#define  A2 r20
#define  a2 r20
#define  twl r24	// wait time
#define  TWL r24
#define  twh r25
#define  TWH r25

;-------------------------------------------------------------------------
;				Using SRAM
;-------------------------------------------------------------------------
.section .bss
.global PROGBUFF,PROGBUFFEND
PROGBUFF: .space 2*BufferSize
PROGBUFFEND:
ProgBuffEnd:
.section .text
;-------------------------------------------------------------------------
;				Macros
;-------------------------------------------------------------------------
 #if ONEWIRE
  .macro	IOPortInit
	sbi 	STX_PORT, SRX		; weak pullup on
	cbi	STX_DDR, SRX		; as input
  .endm
  .macro	TXD_0
	sbi	STX_DDR, SRX		; strong pullup = 0
  .endm
  .macro	TXD_1
	cbi	STX_DDR, SRX		; weak pullup = 1
  .endm
  .macro	SKIP_RXD_0
	sbis	SRX_PIN, SRX		; low = 1
  .endm
  .macro	SKIP_RXD_1
	sbic	SRX_PIN, SRX		; high = 0
  .endm
#else
  .macro	IOPortInit	    ; uses 'a0'
  .if	STX_PORT > 0x1F		; working on io address area > 0x1f
	ldi	a0, 1<<SRX	        ; Called from app? Com port reset is fine.
	sts	SRX_PORT, a0
	ldi	a0, 1<<STX
	sts	STX_PORT, a0
	sts	STX_DDR, a0
  .else				; do as usual
	sbi	SRX_PORT, SRX
	sbi	STX_PORT, STX
	sbi	STX_DDR, STX
  .endif
  .endm
  .macro	TXD_0           ;uses 'a2'
  .if	STX_PORT > 0x1F		; 5 cycles
	lds	a2, STX_PORT
	andi	a2, ~(1<<STX)
	sts	STX_PORT, a2
  .else
	cbi	STX_PORT, STX	; 1 cycle
  .endif
  .endm
;-------------------------------------------------------------------------
  .macro	TXD_1           ;uses 'a2'
  .if	STX_PORT > 0x1F		; 5 cycles
	lds	a2, STX_PORT
	ori	a2, 1<<STX
	sts	STX_PORT, a2
  .else				; 1 cycle
	sbi	STX_PORT, STX
  .endif
  .endm
;-------------------------------------------------------------------------
  .macro	SKIP_RXD_0      ;uses 'a2'
  .if	SRX_PORT > 0x1F
	lds	a2, SRX_PIN
	sbrc	a2, SRX
  .else
	sbic	SRX_PIN, SRX
  .endif
  .endm
;-------------------------------------------------------------------------
  .macro	SKIP_RXD_1      ;uses 'a2'
  .if	SRX_PORT > 0x1F
	lds	a2, SRX_PIN
	sbrs	a2, SRX
  .else
	sbis	SRX_PIN, SRX
  .endif
  .endm
#endif
;-------------------------------------------------------------------------
