// this sketch turns the Arduino into a AVRISP
// using the following pins:
// 10: target reset
// 11: MOSI
// 12: MISO
// 13: SCK

// Put an LED (with resistor) on the following pins:
// 6: Heartbeat - shows the programmer is running
// 8: Error - Lights up if something goes wrong (use red if that makes sense)
// 5: Programming - In communication with the slave
// 7: Configured for output at LOW
// The above arrangement makes it easy to attach a common-cathode RGB LED directly
// into the arduino pin header.
//
// April 2012 by Joey Morin
// - Integrated code from http://pastebin.com/Aw5BD0zy probably authored by
// smeezekitty, referenced in http://arduino.cc/forum/index.php?topic=89918.0
// to include support for slow software SPI.  This allows for programming of
// targets with clk < 500KHz, which is the lower limit for hardware SPI on a
// 16MHz Arduino (SPI clock of 125KHz).
// - Minor code cleanup.
// - Mode selection is made on powerup or reset.  Switch or short the
// MODE_BUTTON pin to GND to enable slow software SPI mode.  Current mode is
// indicated by a fast or slow heartbeat.
//
// October 2009 by David A. Mellis
// - Added support for the read signature command
//
// February 2009 by Randall Bohn
// - Added support for writing to EEPROM (what took so long?)
// Windows users should consider WinAVR's avrdude instead of the
// avrdude included with Arduino software.
//
// January 2008 by Randall Bohn
// - Thanks to Amplificar for helping me with the STK500 protocol
// - The AVRISP/STK500 (mk I) protocol is used in the arduino bootloader
// - The SPI functions herein were developed for the AVR910_ARD programmer
// - More information at http://code.google.com/p/mega-isp

// These set the correct timing delays for programming a target with clk < 500KHz.
// Be sure to specify the lowest clock frequency of any target you plan to burn.
// The rule is the programing clock (on SCK) must be less than 1/4 the clock
// speed of the target (or 1/6 for speeds above 12MHz).  For example, if using
// the 128KHz RC oscillator, with a prescaler of 8, the target's clock frequency
// would be 16KHz, and the maximum programming clock would be 4KHz, or a clock
// period of 250uS.  The algorithm uses a quarter of the clock period for sync
// purposes, so quarter_period would be set to 63uS.  Be aware that internal RC
// oscillators can be off by as much as 10%, so you might have to force a slower
// clock speed.
#define DEFAULT_MINIMUM_TARGET_CLOCK_SPEED 128000
#define DEFAULT_SCK_FREQUENCY (DEFAULT_MINIMUM_TARGET_CLOCK_SPEED/4)
#define DEFAULT_QUARTER_PERIOD ((1000000/DEFAULT_SCK_FREQUENCY/4)+1)

#define MINIMUM_SCK_FREQUENCY 50
#define QUARTER_PERIOD_EEPROM_ADDRESS 0

#define HWVER 2
#define SWMAJ 1
#define SWMIN 18

#define LED_HB 6
#define LED_ERR 8
#define LED_PMODE 5
#define LED_COMMON 7

#define MODE_BUTTON 9

#define MAX_COMMAND_LENGTH 64

// As per http://arduino.cc/en/Tutorial/ArduinoISP
#if ARDUINO >= 100
#define HB_DELAY 20
#else
#define HB_DELAY 40
#endif

#include <EEPROM.h>

#define RESET SS

// STK Definitions
#define STK_OK 0x10
#define STK_FAILED 0x11
#define STK_UNKNOWN 0x12
#define STK_INSYNC 0x14
#define STK_NOSYNC 0x15
#define CRC_EOP 0x20 //ok it is a space...

#define PTIME 30

#define HW_SPI_MODE 1
#define SW_SPI_MODE 0

uint8_t spi_mode;

// safe EEPROM write (only writes if value changed)
void safe_EEPROM_write(int address, byte value) {
  if (value != EEPROM.read(address)) {
    EEPROM.write(address, value);
  }
}

// save a 16 bit value to eeprom
void save_quarter_period_to_eeprom(unsigned int value) {
 
 safe_EEPROM_write(QUARTER_PERIOD_EEPROM_ADDRESS, (uint8_t)(value & 0x00FF)); 
 safe_EEPROM_write(QUARTER_PERIOD_EEPROM_ADDRESS+1, (uint8_t)((value & 0xFF00) >> 8)); 

}

unsigned int load_quarter_period_from_eeprom() {
  unsigned int value;
  
  value = EEPROM.read(QUARTER_PERIOD_EEPROM_ADDRESS) + (EEPROM.read(QUARTER_PERIOD_EEPROM_ADDRESS+1) << 8);
  if ((value == 0) || (value > 1000000/MINIMUM_SCK_FREQUENCY/4))
    value = DEFAULT_QUARTER_PERIOD;
  
  return value;
}

// configure programming speed
unsigned int quarter_period = load_quarter_period_from_eeprom();

// set programming clock from a terminal
void change_sck() {

  char command[MAX_COMMAND_LENGTH];
  uint8_t command_length = 0;
  int key;
  uint32_t sck_frequency;

  Serial.print("Current software SPI quarter-period delay: ");
  Serial.print(quarter_period);
  Serial.print(" microseconds,\n\r");
  Serial.print("Resulting in a programming speed of ");
  Serial.print(1000000/quarter_period/4);
  Serial.print(" Hz.\n\r");
  Serial.print("Enter a new programming clock speed (SCK) in HZ for software SPI.\n\r");
  Serial.print("(NOTE THAT THIS SHOULD BE LESS THAN 1/4 OF TARGET'S CLOCK SPEED):\n\r");

  while(true) {
    heartbeat();
    key = Serial.read();
    if (key != -1) {
      Serial.write(key);
      if (key == '\r') {
        Serial.println();
        command[command_length] = 0;
        sck_frequency = atoi(command);
        if (sck_frequency < 50) {
          Serial.print("\n\rERROR: MINIMUM FREQUENCY IS 50 HZ, TRY AGAIN.\n\r");
        }
        else {
          quarter_period = (1000000/sck_frequency/4)+1;
          Serial.print("Setting software SPI quarter-period delay to ");
          Serial.print(quarter_period);
          Serial.print(" microseconds,\n\r");
          Serial.print("Resulting in a programming speed of ");
          Serial.print(1000000/quarter_period/4);
          Serial.print(" Hz.\n\r");
          save_quarter_period_to_eeprom(quarter_period);
          Serial.print("Saved to EEPROM.\n\r");
          return;
        }
      }
      else if ((key == 8) || (key == 127)) {
        if (command_length) {
          command[command_length] = 0;
          command[--command_length] = ' ';
          Serial.print("\r");
          Serial.write(command);
          command[command_length] = 0;
          Serial.print("\r");
          Serial.write(command);
        }
      }
      else {
        command[command_length++] = key;
      }
      if (command_length >= MAX_COMMAND_LENGTH-1) {
        Serial.print("\n\rERROR: COMMAND LINE TOO LONG, START OVER.\n\r");
        command_length = 0;
      }
    }
  }
}

uint8_t hbval=12;
int8_t hbdelta;

// this provides a heartbeat on pin 9, so you can tell the software is running.
void heartbeat() {
  if ((hbval > 24) || (hbval < 2))
    hbdelta = -hbdelta;
  hbval += hbdelta;
  analogWrite(LED_HB, hbval);
  delay(HB_DELAY);
}

void pulse(int pin, int times) {
  do {
    digitalWrite(pin, HIGH);
    delay(PTIME);
    digitalWrite(pin, LOW);
    delay(PTIME);
  } 
  while (times--);
}

void (*spi_init)();
uint8_t (*spi_send)(uint8_t);

void setup() {
  Serial.begin(19200);
  pinMode(MODE_BUTTON, INPUT);
  digitalWrite(MODE_BUTTON, HIGH);
  pinMode(LED_PMODE, OUTPUT);
  pinMode(LED_ERR, OUTPUT);
  pinMode(LED_HB, OUTPUT);
  pinMode(LED_COMMON, OUTPUT);
  digitalWrite(LED_COMMON, LOW);
  pulse(LED_PMODE, 2);
  pulse(LED_ERR, 2);
  pulse(LED_HB, 2);
  pinMode(MODE_BUTTON, INPUT);
  digitalWrite(MODE_BUTTON, HIGH);
  if (digitalRead(MODE_BUTTON)){
    spi_mode = HW_SPI_MODE;
    spi_init = hw_spi_init;
    spi_send = hw_spi_send;
    hbdelta = 2;
  }
  else {
    spi_mode = SW_SPI_MODE;
    spi_init = sw_spi_init;
    spi_send = sw_spi_send;
    hbdelta = 1;
  }
}

int error=0;
int pmode=0;
// address for reading and writing, set by 'U' command
int here;
uint8_t buff[256]; // global block storage

#define beget16(addr) (*addr * 256 + *(addr+1) )

typedef struct param {
  uint8_t devicecode;
  uint8_t revision;
  uint8_t progtype;
  uint8_t parmode;
  uint8_t polling;
  uint8_t selftimed;
  uint8_t lockbytes;
  uint8_t fusebytes;
  int flashpoll;
  int eeprompoll;
  int pagesize;
  int eepromsize;
  int flashsize;
}
parameter;

parameter param;

#define LED_PWM_DUTY 10
void loop(void) {
  static uint8_t pwm;
  // is pmode active?
  if (pmode) analogWrite(LED_PMODE, (pwm++)%((LED_PWM_DUTY<<1))<<2);
  else digitalWrite(LED_PMODE, LOW);
  // is there an error?
  if (error) digitalWrite(LED_ERR, !((pwm++)%LED_PWM_DUTY));
  else digitalWrite(LED_ERR, LOW);

  // light the heartbeat LED
  heartbeat();
  if (Serial.available()) {
    avrisp();
  }
}

uint8_t getch() {
  while(!Serial.available());
  return Serial.read();
}
void readbytes(int n) {
  for (int x = 0; x < n; x++) {
    buff[x] = Serial.read();
  }
}

void hw_spi_init() {
  uint8_t x;
  SPCR = 0x53;
  x=SPSR;
  x=SPDR;
}

void sw_spi_init() {
}

void spi_wait() {
  do {
  } 
  while (!(SPSR & (1 << SPIF)));
}

uint8_t hw_spi_send(uint8_t b) {
  uint8_t reply;
  SPDR=b;
  spi_wait();
  reply = SPDR;
  return reply;
}

#define PCK() (bits[0] << 7 | bits[1] << 6 | bits[2] << 5 | bits[3] << 4 | bits[4] << 3 | bits[5] << 2 | bits[6] << 1 | bits[7])

uint8_t sw_spi_send(uint8_t b) {
  unsigned static char msk[] = { 
    0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1   };
  uint8_t reply;
  char bits[8] = { 
    0, 0, 0, 0, 0, 0, 0, 0   };
  for(uint8_t _bit = 0;_bit < 8;_bit++){
    digitalWrite(MOSI, !!(b & msk[_bit]));
    delayMicroseconds(quarter_period);
    digitalWrite(SCK, HIGH);
    delayMicroseconds(quarter_period);
    bits[_bit] = digitalRead(MISO);
    delayMicroseconds(quarter_period);
    digitalWrite(SCK, LOW);
    delayMicroseconds(quarter_period);
  }
  reply = PCK();
  return reply;
}

uint8_t spi_transaction(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  uint8_t n;
  spi_send(a);
  n=spi_send(b);
  //if (n != a) error = -1;
  n=spi_send(c);
  return spi_send(d);
}

void empty_reply() {
  if (CRC_EOP == getch()) {
    Serial.print((char)STK_INSYNC);
    Serial.print((char)STK_OK);
  }
  else {
    Serial.print((char)STK_NOSYNC);
  }
}

void breply(uint8_t b) {
  if (CRC_EOP == getch()) {
    Serial.print((char)STK_INSYNC);
    Serial.print((char)b);
    Serial.print((char)STK_OK);
  }
  else {
    Serial.print((char)STK_NOSYNC);
  }
}

void get_version(uint8_t c) {
  switch(c) {
  case 0x80:
    breply(HWVER);
    break;
  case 0x81:
    breply(SWMAJ);
    break;
  case 0x82:
    breply(SWMIN);
    break;
  case 0x93:
    breply('S'); // serial programmer
    break;
  default:
    breply(0);
  }
}

void set_parameters() {
  // call this after reading paramter packet into buff[]
  param.devicecode = buff[0];
  param.revision = buff[1];
  param.progtype = buff[2];
  param.parmode = buff[3];
  param.polling = buff[4];
  param.selftimed = buff[5];
  param.lockbytes = buff[6];
  param.fusebytes = buff[7];
  param.flashpoll = buff[8];
  // ignore buff[9] (= buff[8])
  //getch(); // discard second value

  // WARNING: not sure about the byte order of the following
  // following are 16 bits (big endian)
  param.eeprompoll = beget16(&buff[10]);
  param.pagesize = beget16(&buff[12]);
  param.eepromsize = beget16(&buff[14]);

  // 32 bits flashsize (big endian)
  param.flashsize = buff[16] * 0x01000000
    + buff[17] * 0x00010000
    + buff[18] * 0x00000100
    + buff[19];

}

void start_pmode() {
  spi_init();
  // following delays may not work on all targets...
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, HIGH);
  pinMode(SCK, OUTPUT);
  digitalWrite(SCK, LOW);
  delay(50);
  digitalWrite(RESET, LOW);
  delay(50);
  pinMode(MISO, INPUT);
  pinMode(MOSI, OUTPUT);
  spi_transaction(0xAC, 0x53, 0x00, 0x00);
  pmode = 1;
}

void end_pmode() {
  pinMode(MISO, INPUT);
  pinMode(MOSI, INPUT);
  pinMode(SCK, INPUT);
  pinMode(RESET, INPUT);
  pmode = 0;
}

void universal() {
  int w;
  uint8_t ch;

  for (w = 0; w < 4; w++) {
    buff[w] = getch();
  }
  ch = spi_transaction(buff[0], buff[1], buff[2], buff[3]);
  breply(ch);
}

void flash(uint8_t hilo, int addr, uint8_t data) {
  spi_transaction(0x40+8*hilo,
  addr>>8 & 0xFF,
  addr & 0xFF,
  data);
}
void commit(int addr) {
  spi_transaction(0x4C, (addr >> 8) & 0xFF, addr & 0xFF, 0);
}

//#define _current_page(x) (here & 0xFFFFE0)
int current_page(int addr) {
  if (param.pagesize == 32) return here & 0xFFFFFFF0;
  if (param.pagesize == 64) return here & 0xFFFFFFE0;
  if (param.pagesize == 128) return here & 0xFFFFFFC0;
  if (param.pagesize == 256) return here & 0xFFFFFF80;
  return here;
}
uint8_t write_flash(int length) {
  if (param.pagesize < 1) return STK_FAILED;
  //if (param.pagesize != 64) return STK_FAILED;
  int page = current_page(here);
  int x = 0;
  while (x < length) {
    if (page != current_page(here)) {
      commit(page);
      page = current_page(here);
    }
    flash(LOW, here, buff[x++]);
    flash(HIGH, here, buff[x++]);
    here++;
  }

  commit(page);

  return STK_OK;
}

uint8_t write_eeprom(int length) {
  // here is a word address, so we use here*2
  // this writes byte-by-byte,
  // page writing may be faster (4 bytes at a time)
  for (int x = 0; x < length; x++) {
    spi_transaction(0xC0, 0x00, here*2+x, buff[x]);
    delay(45);
  }
  return STK_OK;
}

void program_page() {
  char result = (char) STK_FAILED;
  int length = 256 * getch() + getch();
  if (length > 256) {
    Serial.print((char) STK_FAILED);
    return;
  }
  char memtype = getch();
  for (int x = 0; x < length; x++) {
    buff[x] = getch();
  }
  if (CRC_EOP == getch()) {
    Serial.print((char) STK_INSYNC);
    if (memtype == 'F') result = (char)write_flash(length);
    if (memtype == 'E') result = (char)write_eeprom(length);
    Serial.print(result);
  }
  else {
    Serial.print((char) STK_NOSYNC);
  }
}
uint8_t flash_read(uint8_t hilo, int addr) {
  return spi_transaction(0x20 + hilo * 8,
  (addr >> 8) & 0xFF,
  addr & 0xFF,
  0);
}

char flash_read_page(int length) {
  for (int x = 0; x < length; x+=2) {
    uint8_t low = flash_read(LOW, here);
    Serial.print((char) low);
    uint8_t high = flash_read(HIGH, here);
    Serial.print((char) high);
    here++;
  }
  return STK_OK;
}

char eeprom_read_page(int length) {
  // here again we have a word address
  for (int x = 0; x < length; x++) {
    uint8_t ee = spi_transaction(0xA0, 0x00, here*2+x, 0xFF);
    Serial.print((char) ee);
  }
  return STK_OK;
}

void read_page() {
  char result = (char)STK_FAILED;
  int length = 256 * getch() + getch();
  char memtype = getch();
  if (CRC_EOP != getch()) {
    Serial.print((char) STK_NOSYNC);
    return;
  }
  Serial.print((char) STK_INSYNC);
  if (memtype == 'F') result = flash_read_page(length);
  if (memtype == 'E') result = eeprom_read_page(length);
  Serial.print(result);
  return;
}

void read_signature() {
  if (CRC_EOP != getch()) {
    Serial.print((char) STK_NOSYNC);
    return;
  }
  Serial.print((char) STK_INSYNC);
  uint8_t high = spi_transaction(0x30, 0x00, 0x00, 0x00);
  Serial.print((char) high);
  uint8_t middle = spi_transaction(0x30, 0x00, 0x01, 0x00);
  Serial.print((char) middle);
  uint8_t low = spi_transaction(0x30, 0x00, 0x02, 0x00);
  Serial.print((char) low);
  Serial.print((char) STK_OK);
}
//////////////////////////////////////////
//////////////////////////////////////////


////////////////////////////////////
////////////////////////////////////
int avrisp() {
  uint8_t data, low, high;
  uint8_t ch = getch();
  uint8_t static cr = 0;

  // support for changing SCK speed via serial interface
  // issuing carriage return 5 times in a row enters SCK change mode
  if (spi_mode == SW_SPI_MODE) {
    if (ch == '\r') {
      cr++;
      if (cr >= 5) {
        cr = 0;
        change_sck();
      }
      return 0;
    }
    else {
      cr = 0;
    }
  }

  switch (ch) {
  case '0': // signon
    empty_reply();
    break;
  case '1':
    if (getch() == CRC_EOP) {
      Serial.print((char) STK_INSYNC);
      Serial.print("AVR ISP");
      Serial.print((char) STK_OK);
    }
    break;
  case 'A':
    get_version(getch());
    break;
  case 'B':
    readbytes(20);
    set_parameters();
    empty_reply();
    break;
  case 'E': // extended parameters - ignore for now
    readbytes(5);
    empty_reply();
    break;

  case 'P':
    start_pmode();
    empty_reply();
    break;
  case 'U':
    here = getch() + 256 * getch();
    empty_reply();
    break;

  case 0x60: //STK_PROG_FLASH
    low = getch();
    high = getch();
    empty_reply();
    break;
  case 0x61: //STK_PROG_DATA
    data = getch();
    empty_reply();
    break;

  case 0x64: //STK_PROG_PAGE
    program_page();
    break;

  case 0x74: //STK_READ_PAGE
    read_page();
    break;

  case 'V':
    universal();
    break;
  case 'Q':
    error=0;
    end_pmode();
    empty_reply();
    break;

  case 0x75: //STK_READ_SIGN
    read_signature();
    break;

    // expecting a command, not CRC_EOP
    // this is how we can get back in sync
  case CRC_EOP:
    Serial.print((char) STK_NOSYNC);
    break;

    // anything else we will return STK_UNKNOWN
  default:
    if (CRC_EOP == getch())
      Serial.print((char)STK_UNKNOWN);
    else
      Serial.print((char)STK_NOSYNC);
  }
}