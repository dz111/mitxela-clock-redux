#define F_CPU 20e6  // 20 MHz

#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "ring_buffer.h"

volatile class _Spi {
private:
  struct message {
    uint8_t payload0;
    uint8_t payload1;
    uint8_t load_mask;  // These pins are blipped high after payload is transmitted
  };

  inline ring_buffer<message, 16> m_rb;
  message m_curr_msg = {0};

  void write(message msg) {
    while (m_rb.full()) { }  // Block while buffer full
    if (m_rb.empty()) {
      m_curr_msg = msg;
      next

  //enum class _state {
  //  idle,
  //  payload0,
  //  payload1,
  //  blip,
  //} m_state;

  //void _do_write(message msg) {
  //  curr_mask = msg.load_mask;
  //  SPI0.DATA = msg.payload;
  //}

  //void write(message msg) {
  //  while (m_rb.full()) { }  // Blocks while buffer full
  //  if (m_rb.empty()) {
  //    _do_write(msg);
  //  } else {
  //    m_rb.push(msg);
  //  }
  //}

  //void write_next() {
  //  if (!m_rb.empty()) {
  //    message msg;
  //    m_rb.pop(msg);
  //    _do_write(msg);
  //  }
  //}

  //void blip() {
  //  PORTA.OUTSET = curr_mask;
  //  _NOP();
  //  _NOP();
  //  PORTA.OUTCLR = curr_mask;
  //  _NOP();
  //  _NOP();
  //}
public:
  void initialise();

  void write(uint8_t payload0, uint8_t payload1, uint8_t load_mask) {
    write(message{payload0, payload1, load_mask});
  }

  void next() {
    switch (m_state) {
    case _state::idle: return;
    case _state::payload0:
      SPI0.DATA =
    }
  }
} Spi;

ISR(SPI0_INT_vect) {
  Spi::next();
  SPI0.INTFLAGS &= (7 << 1); // Clear interrupt
}

/*
 MOSI = 17
 SCLK = 19
 ~LD_DATE = 3
 ~LD_TIME = 4
*/

#define MOSI    _BV(1)  // Pin 17 (PA1)
#define SCLK    _BV(3)  // Pin 19 (PA3)
#define LD_DATE _BV(5)  // Pin 3  (PA5)
#define LD_TIME _BV(6)  // Pin 4  (PA6)

void init_display() {
  // Initialise SPI0 peripheral
  PORTA.DIRSET = MOSI | SCLK;
  PORTMUX.SPIROUTEA = 0;
  PORTA.DIRSET = LD_DATE | LD_TIME; // Set output drivers for pins 3 (PA5), 4 (PA6)
  PORTA.OUTCLR = LD_DATE | LD_TIME; // Set slave select to low (off)
  SPI0.CTRLA |= SPI_MASTER_bm; // Set SPI host mode
  SPI0.CTRLA &= ~(SPI_PRESC_0_bm | SPI_PRESC_1_bm | SPI_CLK2X_bm);
  SPI0.CTRLA |= (SPI_PRESC_0_bm | SPI_PRESC_1_bm); // Set SPI clock to 1.25MHz (MAX7219 max clock 10MHz)
  SPI0.CTRLB &= ~(SPI_MODE_0_bm | SPI_MODE_1_bm); // Set SPI mode 0
  SPI0.CTRLA &= ~(SPI_DORD_bm); // Set data order to MSB first
  SPI0.CTRLB &= ~(SPI_BUFEN_bm); // Set SPI buffer to off
  SPI0.CTRLB |= SPI_SSD_bm; // Disable client select line
  SPI0.CTRLA |= SPI_ENABLE_bm; // Enable SPI

  SPI0.INTCTRL = 0x01;  // Enable interrupt

  // Set decode mode
  SPI0.DATA = 0x09;
  _delay_ms(1);
  SPI0.DATA = 0xFF;
  _delay_ms(1);
  PORTA.OUTSET = LD_DATE | LD_TIME;
  _delay_ms(1);
  PORTA.OUTCLR = LD_DATE | LD_TIME;
  _delay_ms(1);

  // Set intensity
  SPI0.DATA = 0x0A;
  _delay_ms(1);
  SPI0.DATA = 0x0F;
  _delay_ms(1);
  PORTA.OUTSET = LD_DATE | LD_TIME;
  _delay_ms(1);
  PORTA.OUTCLR = LD_DATE | LD_TIME;
  _delay_ms(1);

  // Set scan limit
  SPI0.DATA = 0x0B;
  _delay_ms(1);
  SPI0.DATA = 0x07;
  _delay_ms(1);
  PORTA.OUTSET = LD_DATE | LD_TIME;
  _delay_ms(1);
  PORTA.OUTCLR = LD_DATE | LD_TIME;
  _delay_ms(1);

  // Set test mode
  SPI0.DATA = 0x0F;
  _delay_ms(1);
  SPI0.DATA = 0x00;
  _delay_ms(1);
  PORTA.OUTSET = LD_DATE | LD_TIME;
  _delay_ms(1);
  PORTA.OUTCLR = LD_DATE | LD_TIME;
  _delay_ms(1);

  // Bring both display drivers out of shutdown mode
  SPI0.DATA = 0x0C;
  _delay_ms(1);
  SPI0.DATA = 0x01;
  _delay_ms(1);
  PORTA.OUTSET = LD_DATE | LD_TIME;
  _delay_ms(1);
  PORTA.OUTCLR = LD_DATE | LD_TIME;
  _delay_ms(1);

  // Set digits on time
  for (int i = 0; i < 8; i++) {
    SPI0.DATA = 0x01 + i;
    _delay_ms(1);
    SPI0.DATA = i | (1 << 7);
    _delay_ms(1);
    PORTA.OUTSET = LD_TIME;
    _delay_ms(1);
    PORTA.OUTCLR = LD_TIME;
    _delay_ms(1);
  }

  // Set digits on date
  for (int i = 0; i < 8; i++) {
    SPI0.DATA = 0x01 + i;
    _delay_ms(1);
    SPI0.DATA = (7 - i) | (1 << 7);
    _delay_ms(1);
    PORTA.OUTSET = LD_DATE;
    _delay_ms(1);
    PORTA.OUTCLR = LD_DATE;
    _delay_ms(1);
  }

}
int w = 0;
ISR(TCA0_OVF_vect) {
  if (w == 10) w = 0;
  else w++;

  for (int i = 0; i < 8; i++) {
    SPI0.DATA = 0x01 + i;
    SPI0.DATA = w;
    PORTA.OUTSET = LD_TIME;
    PORTA.OUTCLR = LD_TIME;
  }

  // Set digits on date
  for (int i = 0; i < 8; i++) {
    SPI0.DATA = 0x01 + i;
    SPI0.DATA = w;
    PORTA.OUTSET = LD_DATE;
    PORTA.OUTCLR = LD_DATE;
  }
  TCA0.SINGLE.INTFLAGS = 1;  // Clear interrupt
}

void blink() {
  PORTA.OUTTGL = 0b1000'0000;
}

int main() {
  cli();
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);

  TCA0.SINGLE.PER = 19531;  // 19531 * 1024 = 19,999,744 ~1Hz
  TCA0.SINGLE.CTRLA = (0x7 << 1);  // Set prescaler to 1/1024
  TCA0.SINGLE.CTRLB = 0x0;  // Count from 0 to PER then overflow
  TCA0.SINGLE.INTCTRL = 1;  // Enable interrupt
  TCA0.SINGLE.CTRLA |= 1;  // Enable timer

  PORTA.DIRSET = 0b1000'0000;
  init_display();
  sei();
  while (1) {
    blink();
    _delay_ms(1000);
  }
}
