//_____________________ fuse.c
#define F_CPU 8000000UL

#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define _REVERSE

#define _OPT_BUTTON_
/*
. document tab-stop set to 4, best viewed w/ vi set to ":set ts=4"

. description
  this project is created so that if i could revert the RESET fuse change and flash via
  SPI again.

. features
  . reads device signature and hi-low fuses for hi-voltage serial programmable attinys
  . reset hi-low fuses to factory default on target devices
  . layout to drop-on attiny13, attiny25/45/85 8 pin devices targets
  . attiny24/44/84 targets needs additional breadboard and jumper wires
  . standalone operations, fuses show on 7 segment display

. project fuse setting
  avrdude -c usbtiny -p t2313 -V -U lfuse:w:0xe4:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m 

. parts list
  . attiny2313
  . 4x7 segment LED display
  . 1k resistor x 2
  . 2n2222 NPN transistor or equivalent
  . 78L05
  . mini breadboard 170 tiepoints
  . +12V battery source

. opearation
  . place 8 pin target device on breadboard
  . for 14 pin targets, jumper wire to breadboard
  . apply 12V power
  . display shows device name upon identification
  . press and release button to cycle display content
  . displays device name, fuse hi bits, fuse low bits and fuse extended bits
  . long press and release button to reset fuse to factory default
  
. references and related projects
  . datasheets
  . avrdoper http://www.obdev.at/avrusb/avrdoper.html/
  . mightyohm http://www.mightyohm.com/blog/2008/09/arduino-based-avr-high-voltage-programmer/


                 Gd 12V
                 Gd Rt V+ Ck MI MO (tinyusb programmer)
                  +  +  +  +  +  +
   +=====================================================+
   |  .  .  .  .  .  I  .  .  .  .  .  .  .  .  .  .  .  | [I]n (78L05)
   |  .  .  .  .  C  .  .  .  .  .  .  .  .  .  .  .  .  | [C]ommon
   |  .  .  .  .  .  .  O  .  .  .  .  .  .  .  .  .  .  | [O]ut
   | (v)(d)(b)(c) +--------o (v) .  .  .  .  . (d)(c) .  |
   |  +--+--+--+  |  o  +--+-(1)-A--F-(2)(3)-B--+--+  .  |
   | |+         | 1k | |+ b7..6..5..4..3..2..1..0 d6|    |
   | |         -| |  1k|R d0..1 a1..0 d2..3..4..5  -|    |
   |  +--+--+--+  o  |  +--+--+--+--+--+--+--+--+--+  .  |
   |  .  .  .  .  .  o  -  -  E  D (.) C  G (4) -  -  .  |
   |  o--------------o  .  .  .  .  .  .  .  .  .  .  .  |
   |  . (a) .  E  B  C  . (a) .  .  .  .  .  . (b) .  .  | EBC of 2n2222
   |  .  .  .  .  .  .  .  .  .  .  .  .  .  o--B--o  .  |
   +=====================================================+
                    (p)(p)                                 join (p)(p) during programming

      SDO   SDI
    +--+--+--+
   |+    SII  |
   |R        -|
    +--+--+--+ 
      SCI
 
#define HIV	_BV(7)	//b7 .. RESET
#define VCC	_BV(6)	//b6 .. (v)
#define SDO _BV(0)	//b0 .. (d)

#define SCI _BV(0)	//d0 .. (a)
#define SII _BV(5)	//d5 .. (b)
#define SDI _BV(6)	//d6 .. (c)

*/

#define BUTTON_DDR	 	DDRD
#define BUTTON_PORT 	PORTD
#define BUTTON_PIN 		_BV(4)

#define SEG_A_PB	_BV(5)
#define SEG_B_PB	_BV(1)
#define SEG_C_PB	0x00
#define SEG_D_PB	0x00
#define SEG_E_PB	0x00
#define SEG_F_PB	_BV(4)
#define SEG_G_PB	0x00
#define SEG_d_PB	0x00
#define DIGIT_0_PB	_BV(6)
#define DIGIT_1_PB	_BV(3)
#define DIGIT_2_PB	_BV(2)
#define DIGIT_3_PB	0x00

#define SEG_A_PD	0x00
#define SEG_B_PD	0x00
#define SEG_C_PD	_BV(2)
#define SEG_D_PD	_BV(7)		// d.7 maps to a.1
#define SEG_E_PD	_BV(1)
#define SEG_F_PD	0x00
#define SEG_G_PD	_BV(3)
//#define SEG_d_PD	0x00
#define SEG_d_PD	_BV(6)		// d.6 maps to a.0
#define DIGIT_0_PD	0x00
#define DIGIT_1_PD	0x00
#define DIGIT_2_PD	0x00
#define DIGIT_3_PD	_BV(4)

#ifdef _REVERSE
	#define HOLD_DDR_VAL 0x60
	#define HOLD_VAL 	 0x40
#else
	#define HOLD_DDR_VAL 0x18
	#define HOLD_VAL 	 0x08
#endif


#define SEGS_STAY(v) \
   (((v & _BV(6)) ? 1 : 0) +\
	((v & _BV(5)) ? 1 : 0) +\
	((v & _BV(4)) ? 1 : 0) +\
	((v & _BV(3)) ? 1 : 0) +\
	((v & _BV(2)) ? 1 : 0) +\
	((v & _BV(1)) ? 1 : 0) +\
	((v & _BV(0)) ? 1 : 0)) | 0x40

#define SEGS_PORT_DET(p, v) \
   (((v & _BV(6)) ? SEG_A_P##p : 0) |	\
	((v & _BV(5)) ? SEG_B_P##p : 0) |	\
	((v & _BV(4)) ? SEG_C_P##p : 0) |	\
	((v & _BV(3)) ? SEG_D_P##p : 0) |	\
	((v & _BV(2)) ? SEG_E_P##p : 0) |	\
	((v & _BV(1)) ? SEG_F_P##p : 0) |	\
	((v & _BV(0)) ? SEG_G_P##p : 0))

#define SEGS_PORT(v)	{SEGS_STAY(v),SEGS_PORT_DET(B, v),SEGS_PORT_DET(D, v)}
#define SEGS_B (SEG_A_PB|SEG_B_PB|SEG_C_PB|SEG_D_PB|SEG_E_PB|SEG_F_PB|SEG_G_PB|SEG_d_PB)
#define SEGS_D (SEG_A_PD|SEG_B_PD|SEG_C_PD|SEG_D_PD|SEG_E_PD|SEG_F_PD|SEG_G_PD|SEG_d_PD)

#define DIGITS_B (DIGIT_0_PB|DIGIT_1_PB|DIGIT_2_PB|DIGIT_3_PB)
#define DIGITS_D (DIGIT_0_PD|DIGIT_1_PD|DIGIT_2_PD|DIGIT_3_PD)

#define USED_B (SEGS_B|DIGITS_B)
#define USED_D (SEGS_D|DIGITS_D)

/*
       ___a__
      |      |
     f|      | b
       ___g__
     e|      | c
      |      |
       ___d__
*/
//_____________________ abc defg
#define LTR_0 0x7e	// 0111 1110
#define LTR_1 0x30	// 0011 0000
#define LTR_2 0x6d	// 0110 1101
#define LTR_3 0x79	// 0111 1001
#define LTR_4 0x33	// 0011 0011
#define LTR_5 0x5b	// 0101 1011
#define LTR_6 0x5f	// 0101 1111
#define LTR_7 0x70	// 0111 0000
#define LTR_8 0x7f	// 0111 1111
#define LTR_9 0x7b	// 0111 1011
#define LTR_A 0x77	// 0111 0111
#define LTR_b 0x1f	// 0001 1111
#define LTR_C 0x4e	// 0100 1110
#define LTR_d 0x3d	// 0011 1101
#define LTR_E 0x4f	// 0100 1111
#define LTR_F 0x47	// 0100 0111

#define LTR_P 0x67	// 0110 0111
#define LTR_G 0x5e	// 0101 1110
#define LTR_n 0x15	// 0001 0101
#define LTR_r 0x05	// 0000 0101
#define LTR_t 0x0f	// 0000 1111
#define LTR__ 0x00	// 0000 0000

uint8_t EEMEM digit2ports[][3] = { 
	SEGS_PORT(LTR_0), SEGS_PORT(LTR_1), SEGS_PORT(LTR_2), SEGS_PORT(LTR_3),
	SEGS_PORT(LTR_4), SEGS_PORT(LTR_5), SEGS_PORT(LTR_6), SEGS_PORT(LTR_7),
	SEGS_PORT(LTR_8), SEGS_PORT(LTR_9), SEGS_PORT(LTR_A), SEGS_PORT(LTR_b),
	SEGS_PORT(LTR_C), SEGS_PORT(LTR_d), SEGS_PORT(LTR_E), SEGS_PORT(LTR_F),
	SEGS_PORT(LTR_r), SEGS_PORT(LTR_P), SEGS_PORT(LTR_G), SEGS_PORT(LTR_n), 
	SEGS_PORT(LTR_t), SEGS_PORT(LTR__),
};

enum {
	POS_0, POS_1, POS_2, POS_3, POS_4, POS_5, POS_6, POS_7,
	POS_8, POS_9, POS_A, POS_b, POS_C, POS_d, POS_E, POS_F,
	POS_r, POS_P, POS_G, POS_n, POS_t, POS__,
};

#define ST_HOLD		0x80
#define ST_PRESSED	0x40
#define ST_BUTTON   (ST_HOLD|ST_PRESSED)
#define ST_TICKED	0x20
#define ST_12HR 	0x10
#define ST_REFRESH	0x08
#define ST_BUZZ     0x04
#define ST_SETUP   	0x03

#define ST_SENSE ST_BUZZ

volatile uint8_t busy=0;
volatile uint8_t state=ST_REFRESH|ST_12HR;
volatile uint8_t dim=1;

uint8_t  pos=0, stays;
uint8_t  mode=0;
#ifdef _OPT_SENSE_
uint16_t charged=0;
#endif
uint16_t button=0;
uint16_t clicks=0, ticks=0;

//_______________________ ticks per second and devired values
#define TPS   (F_CPU/256)
#define TPS_2 (TPS/2)
#define TPS_4 (TPS/4)

#define LONG_HOLD (TPS/3)

#define NUM_DIGITS	4

static const uint8_t digit_mapb[] PROGMEM = { 0x40,0x08,0x04,0x00 };
static const uint8_t digit_mapd[] PROGMEM = { 0x00,0x00,0x00,0x10 };
//_________________ list of porta..c,ddra..c,stay * number_of_digits
uint8_t output[3 * NUM_DIGITS];
uint8_t *ioptr = output; 
uint8_t dot = 0x00;

//__________________________________________________
ISR(TIMER0_OVF_vect) { 

	clicks++;

	if (clicks >= TPS) {
		clicks = 0;
		state |= ST_TICKED;
		ticks++;
	}//if

	if (stays & 0x0f) { stays--; return; }
	stays >>= dim;

	DDRA  &= ~(USED_D>>6);
	DDRB  &= ~USED_B;
	DDRD  &= ~(USED_D&0x3f);
	PORTA &= ~(USED_D>>6);
	PORTB &= ~USED_B;
	PORTD &= ~(USED_D&0x3f);

	if (stays) { stays--; return; }

	if (busy) return;

#ifdef _OPT_BUTTON_
	//___________ scan button
	PORTD |= BUTTON_PIN;
	_delay_us(1);		// allow pull-up to settle
	if (PIND & BUTTON_PIN) {
		if (button) {
			if (button > 30) {
				//_________ released
				if (button > LONG_HOLD)
					state |= ST_HOLD;
				else
					state |= ST_PRESSED;
				stays = 0x14;
			}//if
		}//if
		else {
			stays = 0x10;
		}//else
		button = 0;
	}//if
	else {
		//_____________ pressed
		button++;
		if (button > LONG_HOLD) {	// show long hold
#ifdef _REVERSE
			DDRB |= HOLD_DDR_VAL; PORTB |= HOLD_VAL;
#else
			DDRD |= HOLD_DDR_VAL; PORTD |= HOLD_VAL;
#endif
		}//if
	}//else
	PORTD &= ~BUTTON_PIN;
	DDRD  &= ~(USED_D&0x3f);
	PORTD &= ~(USED_D&0x3f);
	if (state & ST_BUTTON) return;
	if (button) return;
#endif

#ifdef _OPT_SENSE_
	if (!charged && (state & ST_SENSE)) {
		state &= ~ST_SENSE;
		DDRB  = 0x00; PORTB = 0x00;
		DDRD  = 0x00; PORTD = 0x00;
		_delay_us(1);
		//_________ charge sensor led
		charged = 1;
		//SENSE_CHARGE

		DDRA  |=  SEGS_D>>6;
		PORTA &= ~(SEGS_D>>6);
		DDRB  |=  SEGS_B;
		PORTB &= ~SEGS_B;
		DDRD  |=  SEGS_D&0x3f;
		PORTD &= ~(SEGS_D&0X3f);

		DDRB  &= ~0x44;
		PORTB |=  0x44;
		//DDRB  &= ~_BV(6);
		//PORTB |= _BV(6);
		//_delay_us(1);
		//asm volatile("nop\n\tnop\n\tnop\n\tnop\n\t"::);
		//asm volatile("nop\n\tnop\n\tnop\n\tnop\n\t"::);
		//asm volatile("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"::);
		//while (!SENSE_BUTT);
		//PORTB &= ~_BV(6);
		while (!SENSE_ALL);
		PORTB &= ~0x44;


		return;
	}//if
	
#endif


	//___________ load segments
	uint8_t portd = *ioptr++;
	uint8_t portb = *ioptr++;
	uint8_t porta = portd >> 6;
	portd &= 0x3f;

	//uint8_t mapb = digit_mapb[pos];
	//uint8_t mapd = digit_mapd[pos];
	uint8_t mapb = pgm_read_byte(&digit_mapb[pos]);
	uint8_t mapd = pgm_read_byte(&digit_mapd[pos]);
	DDRA  |= porta;
	DDRB  |= portb | mapb;
	DDRD  |= portd | mapd;
#ifdef _REVERSE
	PORTA |= ~porta;
	PORTB |= ~portb & mapb;
	PORTD |= ~portd & mapd;
#else
	PORTA |= porta;
	PORTB |= portb & ~mapb;
	PORTD |= portd & ~mapd;
#endif

	stays  = *ioptr++;

	if (stays && (state & ST_SETUP) && (clicks / TPS_2) &&
		((pos == (state & ST_SETUP)) ||
		(!pos && !(state & 0x02))))
		stays = 0x08;

	pos++;
	if (pos >= 4) {
		pos = 0;
		ioptr = output;
	}//if

}


//__________________________________________________
void seg2port(uint8_t bcd, uint8_t which) {

	if (which & 0x40) {
		//_____________ bcd entry, do 2nd digit 1st, then 1st digit
		which &= 0x0f;
		seg2port(bcd&0x0f, which+1);
		bcd >>= 4;
	}//if

	uint8_t *pp = output + which * 3;
	uint8_t offset = 3;
	busy++;

	do {
		*pp++ = eeprom_read_byte(&digit2ports[bcd][--offset]);
	} while (offset);
	busy--;

}

#define HIV	_BV(7)	//b7
#define VCC	_BV(6)	//b6
#define SDO _BV(0)	//b0

#define SCI _BV(0)	//d0
#define SII _BV(5)	//d5
#define SDI _BV(6)	//d6

#define SCI_PULSE	_delay_us(1); PORTD |= SCI; _delay_us(1); PORTD &= ~SCI;
//__________________________________________________
uint8_t hv_cmd(uint8_t *dptr, uint8_t cnt) {
	// data format is like write 0_DDDD_DDDD_00
	//                      read D_DDDD_DDDx_xx
	uint8_t sdo=0x00;
	while (cnt) {
		uint8_t sdi = *dptr++;
		uint8_t sii = *dptr++;
		uint8_t cmp=0x80;

		sdo = 0x00;
		PORTD &= ~(SDI|SII);
		SCI_PULSE;
		//sdo = ((PINB&SDO) ? 1 : 0);
		//sdo = ((PINB&SDO) ? 0 : 1);

		// 0x1e92 06
		// 0x62df	b0110 0010 1101 1111
		while (cmp) {
			sdo <<= 1;
			//sdo |= ((PINB&SDO) ? 0 : 1);
			//sdo |= ((PINB&SDO) ? 1 : 0);
			if (PINB&SDO) sdo |= 0x01;
			PORTD &= ~(SDI|SII);
			if (cmp&sdi) PORTD |= SDI;
			//else         PORTD &= ~SDI;
			if (cmp&sii) PORTD |= SII;
			//else         PORTD &= ~SII;
			SCI_PULSE;
			//sdo |= (PINB&SDO) ? 1 : 0;
			cmp >>= 1;
		}//while

		PORTD &= ~(SDI|SII);
		SCI_PULSE;
		SCI_PULSE;
		_delay_us(100);
		cnt--;
	}//while
	//sdo = cnt;

	return sdo;
}

uint8_t chip_sig[] = { 0xee, 0xee, 0xee, 0xee, 0x00, 0x00 };

//__________________________________________________
void read_chip(uint8_t release_reset) {

	cli();
	busy++;

	// enter hv mode, everything go low
	DDRB  |= (VCC|HIV|SDO);
	PORTB &= ~(VCC|SDO);// Vcc and data out off
	PORTB |= HIV;		// 12v off
	DDRD  |= (SCI|SDI|SII);
	PORTD &= ~(SCI|SDI|SII);
	//_delay_ms(50);

	PORTB |= VCC;		// Vcc on
	_delay_us(40);
	PORTB &= ~HIV;		// turn on 12v
	_delay_us(15);
	DDRB  &= ~SDO;		// release SDO
	_delay_us(300);

	// should be ready to issue commands
	// read signature
	//
	// default fuse settting taken from http://www.engbedded.com/fusecalc/
	//
	// mcu - id          - factory fuse low-high-extended
	// t13 - 1e9007 (07) - 6a-ff
	// t25 - 1e9108 (18) - 62-df-ff
	// t45 - 1e9206 (26) - 62-df-ff
	// t85 - 1e930b (3b) - 62-df-ff
	// t24 - 1e910b (1b) - 62-df-ff
	// t44 - 1e9207 (27) - 6a-df-ff
	// t84 - 1e930c (3c) - 6a-df-ff

	// signature  { 0x08, 0x4c, 0xpp, 0x0c,      0x00, 0x68,      0x00, 0x6c
	// write fuse { 0x40, 0x4c, val,  0x2c,      0x00, 0x64/0x74, 0x00, 0x6c/0x7c
	// read fuse  { 0x04, 0x4c, 0x00, 0x68/0x7a, 0x00, 0x6c/0x7c

	uint8_t cmd[] = { 0x08, 0x4c, 0x00, 0x0c, 0x00, 0x68, 0x00, 0x6c, };
	uint8_t *pdata = chip_sig;
	uint8_t i;
	
	//____________________ read device signature
	for (i=0;i<3;i++) {
		cmd[2] = i;
		if (i)
			*pdata++ = hv_cmd(&cmd[2], 3);
		else
			*pdata++ = hv_cmd(cmd, 4);
	}//for

	uint8_t  id[] = { 0x07, 0x18, 0x26, 0x3b, 0x1b, 0x27, 0x3c, };
	uint8_t mcu[] = { 0x13, 0x25, 0x45, 0x85, 0x24, 0x44, 0x84, };

	if ((chip_sig[0] == 0x1e) && ((chip_sig[1]&0xf0) == 0x90) &&
		!(chip_sig[2]&0xf0)) {
		chip_sig[1] <<= 4;
		chip_sig[1] |= chip_sig[2];
		/*
		switch (chip_sig[1]) {
			case 0x07: chip_sig[1] = 0x13; break;
			case 0x18: chip_sig[1] = 0x25; break;
			case 0x26: chip_sig[1] = 0x45; break;
			case 0x3b: chip_sig[1] = 0x85; break;
			case 0x1b: chip_sig[1] = 0x24; break;
			case 0x27: chip_sig[1] = 0x44; break;
			case 0x3c: chip_sig[1] = 0x84; break;
			default: chip_sig[1] = 0xee; break;
		}//switch
		chip_sig[0] = 0x00;
		*/
		for (i=0;i<7;i++) {
			if (chip_sig[1] == id[i]) {
				chip_sig[0] = i;
				chip_sig[1] = mcu[i];
			}//if
		}//for
		pdata--;
	}//if

	//____________________ reset fuse
	if (release_reset) {
		cmd[0] = 0x40; 
		//________________ write fuse low bits
		cmd[3] = 0x2c; cmd[5] = 0x64; 
		if (chip_sig[1] == 0x13 || chip_sig[1] == 0x44 || chip_sig[1] == 0x84)
			cmd[2] = 0x6a; 
		else
			cmd[2] = 0x62; 
		hv_cmd(cmd, 4); _delay_ms(50);
		//________________ write fuse high bits
		cmd[5] = 0x74; cmd[7] = 0x7c;
		if (chip_sig[1] == 0x13)
			cmd[2] = 0xff; 
		else
			cmd[2] = 0xdf; 
		hv_cmd(cmd, 4); _delay_ms(50);
		//________________ write fuse extended bits
		if (chip_sig[1] != 0x13) {
			cmd[5] = 0x66; cmd[7] = 0x6e;
			cmd[2] = 0x01; 
			hv_cmd(cmd, 4); _delay_ms(50);
		}//if
	}//if

	//____________________ read fuse
	cmd[0] = 0x04; cmd[2] = 0x00;

	cmd[3] = 0x68; cmd[5] = 0x6c; 		// fuse low
	*pdata++ = hv_cmd(cmd, 3);

	cmd[3] = 0x7a; cmd[5] = 0x7c; 		// fuse high
	*pdata++ = hv_cmd(cmd, 3);

	if (chip_sig[1] != 0x13) {
		cmd[3] = 0x6a; cmd[5] = 0x6e; 	// fuse extended
		*pdata++ = hv_cmd(cmd, 3);
	}//if


	// done, turn things off
	PORTB |= HIV;
	PORTB &= ~(VCC|SDO);
	PORTD &= ~(SCI|SDI|SII);
	busy--;
	sei();
}

uint8_t menu=0, loc=0;

//__________________________________________________
void main(void) {

	TCCR0B |= 0x01;
	TIMSK  |= _BV(TOIE0);
	TCNT0   = 0x00;

	DDRA = PORTA = 0;
	DDRD = PORTD = 0;
	DDRB   = (VCC|HIV|SDO);
	//DDRB   = (VCC|SDO);
	PORTB  = HIV;		// 12v off
	_delay_ms(50);
	read_chip(0);
	sei();


	while (1) { 
		//_____________ "state" bits LBTSRZPP
		//              L - long hold, B - button, T - ticked, S - setup mode, 
		//              R - refresh,   Z - buzzer  PP - digit edit position
		//
		//_____________ "menu" bits TxxOxMMM
		//              T   - in toggle mode
		//              O   - option used
		//              MMM - menu id (1-4)

		if (state & ST_TICKED) {
			state &= ~ST_TICKED;
		}//if

		if (state & ST_REFRESH) {
			state &= ~ST_REFRESH;
			dot = _BV(mode);
			if (mode) {
				seg2port(chip_sig[mode*2], 0x40);
			}//if
			else {
				seg2port(POS__, 0);
				seg2port(POS_t, 1);
			}//else
			seg2port(chip_sig[mode*2+1], 0x42);
		}//if

		//_____________________________________ check input
		if (state & ST_BUTTON) {	// needs attention
			loc = 0;
			if (state & ST_PRESSED) {
				// normal pressed, rotate display mode
				mode++;
				if (mode >= 3) mode = 0;
			}//if
			else {
				// pressed and held, reset fuse
				mode = 0;
				read_chip(1);
				_delay_us(100);
				read_chip(0);
			}//else
			state &= ~ST_BUTTON;
			state |= ST_REFRESH;
		}//if
	}//while

}

