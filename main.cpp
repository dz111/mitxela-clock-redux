#define MOSI    _BV(1)  // Pin 17 (PA1)
#define SCLK    _BV(3)  // Pin 19 (PA3)
#define LD_DATE _BV(5)  // Pin 3  (PA5)
#define LD_TIME _BV(6)  // Pin 4  (PA6)
#define PPS     _BV(5)  // Pin 6  (PB5)
#define RXD     _BV(3)  // Pin 8  (PB3)

#include "spi.h"
#include "parser.h"
#include "uart.h"

#include <stdint.h>
#include <avr/cpufunc.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

volatile uint8_t seconds = 0;
volatile uint8_t centiseconds = 0;
volatile uint16_t period = 50000;
volatile int32_t phase = 0;

void init_display() {
  Spi.write(0x09, 0xFF, LD_DATE | LD_TIME);  // Set decode mode
  Spi.write(0x0A, 0x0F, LD_DATE | LD_TIME);  // Set intensity
  Spi.write(0x0B, 0x07, LD_DATE | LD_TIME);  // Set scan limit
  Spi.write(0x0F, 0x00, LD_DATE | LD_TIME);  // Set test mode
  Spi.write(0x0C, 0x01, LD_DATE | LD_TIME);  // Exit shutdown mode
}

void clear_display() {
  for (uint8_t i = 0; i < 8; ++i) {
    Spi.write(0x01 + i, 0x0F, LD_TIME | LD_DATE);
  }
}

template <typename T>
inline T abs(T val) {
  constexpr uint8_t shift = sizeof(T) * 8 - 1;
  const T mask = val >> shift;
  return (val ^ mask) - mask;
}

void render() {
#if 0
  Spi.write(0x08, 0x0C, LD_DATE);
  Spi.write(0x07, 0x0B, LD_DATE);
  Spi.write(0x06, 0x0D, LD_DATE);
  Spi.write(0x05, 0x0E, LD_DATE);
#else
  int8_t cs = abs(phase) / period;
  int16_t tick = (abs(phase) % period) / 10;

  if (cs > 1) {
    for (uint8_t i = 0; i < 2; ++i) {
      uint8_t v = cs % 10;
      if (i == 0 && phase < 0) v |= 0x80;
      Spi.write(0x05 + i, v, LD_DATE);
      cs /= 10;
    }
    Spi.write(0x07, 0x0F, LD_DATE);
    Spi.write(0x08, 0x0F, LD_DATE);
  } else {
    for (uint8_t i = 0; i < 4; ++i) {
      uint8_t v = tick % 10;
      if (i == 0 && phase < 0) v |= 0x80;
      Spi.write(0x05 + i, v, LD_DATE);
      tick /= 10;
    }
  }
#endif

  uint16_t per = period / 10;
  for (uint8_t i = 0; i < 4; ++i) {
    uint8_t v = per % 10;
    Spi.write(0x05 + i, v, LD_TIME);
    per /= 10;
  }

  if (centiseconds > 99) {
    Spi.write(0x01, 0x0E, LD_TIME);
    Spi.write(0x02, 0x0E, LD_TIME);
  } else {
    Spi.write(0x01, centiseconds % 10, LD_TIME);
    Spi.write(0x02, centiseconds / 10, LD_TIME);
  }

  if (seconds > 99) {
    Spi.write(0x03, 0x0E, LD_TIME);
    Spi.write(0x04, 0x0E, LD_TIME);
  } else {
    Spi.write(0x03, seconds % 10 | 0x80, LD_TIME);
    Spi.write(0x04, seconds / 10, LD_TIME);
  }
}

ISR(TCA0_OVF_vect) {
  centiseconds = centiseconds + 1;
  if (centiseconds > 99) [[unlikely]] {
    centiseconds = 0;
    seconds = seconds + 1;
    if (seconds > 99) [[unlikely]] {
      seconds = 0;
    }
  }
  render();

  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}

ISR(TCB1_INT_vect) {
  uint16_t count = TCA0.SINGLE.CNT;
  static bool discard = false;
  if (TCB1.INTFLAGS & TCB_OVF_bm) [[unlikely]] {
    TCB1.INTFLAGS = TCB_OVF_bm;
    discard = true;
    return;
  }
  if (discard) [[unlikely]] {
    TCB1.INTFLAGS = TCB_CAPT_bm;
    discard = false;
    return;
  }
  uint32_t value = ((uint32_t)TCB1.CCMP << 16) | TCB0.CCMP;
  value /= 100;  // Divide PPS by 100 to obtain centiseconds
  if (value < 45000 || value > 55000) return;

  if (value > period) {
    period = period + (((uint16_t)value - period) >> 2);
  } else {
    period = period - ((period - (uint16_t)value) >> 2);
  }

  int32_t cs = centiseconds;
  if (cs > 50) cs -= 100;
  if (count > (period / 2)) count -= period;
  int32_t new_phase = cs * (int32_t)period + count;
  constexpr uint16_t divider = 256;
  phase = new_phase;
  TCA0.SINGLE.PERBUF = period + phase / divider;
//  Uart.buffer.push('a');
}

void clock_initialise() {
  TCA0.SINGLE.PER = period;
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV4_gc;
  TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
  TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;

  PORTB.DIRCLR = PPS;
  PORTB.OUTCLR = PPS;

  EVSYS.CHANNEL0 = EVSYS_CHANNEL0_PORTB_PIN5_gc;
  EVSYS.USERTCB0CAPT = EVSYS_USER_CHANNEL0_gc;
  EVSYS.USERTCB1CAPT = EVSYS_USER_CHANNEL0_gc;
  EVSYS.CHANNEL1 = EVSYS_CHANNEL1_TCB0_OVF_gc;
  EVSYS.USERTCB1COUNT = EVSYS_USER_CHANNEL1_gc;

  TCB0.CTRLA = TCB_CLKSEL_TCA0_gc;
  TCB0.CTRLB = TCB_CNTMODE_FRQ_gc;
  TCB0.EVCTRL = TCB_FILTER_bm | TCB_CAPTEI_bm;

  TCB1.CTRLA = TCB_CASCADE_bm | TCB_CLKSEL_EVENT_gc;
  TCB1.CTRLB = TCB_CNTMODE_FRQ_gc;
  TCB1.INTCTRL = TCB_CAPT_bm;
  TCB1.EVCTRL = TCB_CAPTEI_bm;

  TCB0.CTRLA |= TCB_ENABLE_bm;
  TCB1.CTRLA |= TCB_ENABLE_bm;
}

#define UPDI_UART_ENABLE   0x01
#define UPDI_UART_RX_READY 0x02

#define SYSCFG_OCDM  _SFR_MEM8(0x0F18)
#define SYSCFG_OCDMS _SFR_MEM8(0x0F19)

class _Debug {
public:
  inline bool updi_uart_enabled() {
    return GPIOR2 & UPDI_UART_ENABLE;
  }

  void write(uint8_t c) {
    if (!updi_uart_enabled()) return;
    while (SYSCFG_OCDMS) { }
    SYSCFG_OCDM = c;
  }
} Debug;

int main() {
  cli();
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);

//  wdt_enable(WDTO_250MS);

  Spi.initialise();
  Uart.initialise();
  clock_initialise();

  PORTA.DIRSET = LD_DATE | LD_TIME;
  PORTA.OUTCLR = LD_DATE | LD_TIME;

  PORTA.DIRSET = PIN7_bm;

  sei();

  init_display();
  clear_display();
  render();

  while (1) {
    //wdt_reset();
    while (!Uart.buffer.empty()) {
      uint8_t c;
      Uart.buffer.pop(&c);
      Debug.write(c);
      if (c == '\r') Debug.write('\n');
    }
  }
}
