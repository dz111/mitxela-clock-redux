#define MOSI    _BV(1)  // Pin 17 (PA1)
#define SCLK    _BV(3)  // Pin 19 (PA3)
#define LD_DATE _BV(5)  // Pin 3  (PA5)
#define LD_TIME _BV(6)  // Pin 4  (PA6)

#include "spi.h"

#include <avr/cpufunc.h>
#include <avr/io.h>
#include <util/delay.h>

void init_display() {
  Spi.write(0x09, 0xFF, LD_DATE | LD_TIME);  // Set decode mode
  Spi.write(0x0A, 0x0F, LD_DATE | LD_TIME);  // Set intensity
  Spi.write(0x0B, 0x07, LD_DATE | LD_TIME);  // Set scan limit
  Spi.write(0x0F, 0x00, LD_DATE | LD_TIME);  // Set test mode
  Spi.write(0x0C, 0x01, LD_DATE | LD_TIME);  // Exit shutdown mode
}

void blink() {
  PORTA.OUTTGL = 0b1000'0000;
}

ISR(TCA0_OVF_vect) {
  blink();
#if 1
  static uint8_t w = 0;
  if (w == 19) w = 0;
  else w++;

  uint8_t v = (w % 2) ? w / 2 : w / 2 | 0x80;

  for (int i = 0; i < 8; i++) {
    Spi.write(0x01 + i, v, LD_TIME);
  }

  // Set digits on date
  for (int i = 0; i < 8; i++) {
    Spi.write(0x01 + i, v, LD_DATE);
  }

  TCA0.SINGLE.INTFLAGS = 1;  // Clear interrupt
#endif
}

int main() {
  cli();
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);

  Spi.initialise();

  TCA0.SINGLE.PER = 9765;  // 19531 * 1024 = 19,999,744 ~1Hz
  TCA0.SINGLE.CTRLA = (0x7 << 1);  // Set prescaler to 1/1024
  TCA0.SINGLE.CTRLB = 0x0;  // Count from 0 to PER then overflow
  TCA0.SINGLE.INTCTRL = 1;  // Enable interrupt
  TCA0.SINGLE.CTRLA |= 1;  // Enable timer

  PORTA.DIRSET = 0b1000'0000;

  PORTA.DIRSET = LD_DATE | LD_TIME;
  PORTA.OUTCLR = LD_DATE | LD_TIME;

  init_display();

  sei();

  while (1);
}
