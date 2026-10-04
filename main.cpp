#define MOSI    _BV(1)  // Pin 17 (PA1)
#define SCLK    _BV(3)  // Pin 19 (PA3)
#define LD_DATE _BV(5)  // Pin 3  (PA5)
#define LD_TIME _BV(6)  // Pin 4  (PA6)
#define PPS     _BV(5)  // Pin 6  (PB5)

#include "spi.h"

#include <stdint.h>
#include <avr/cpufunc.h>
#include <avr/io.h>
#include <util/delay.h>

volatile bool course = false;
volatile uint8_t seconds = 0;
volatile uint8_t centiseconds = 0;
volatile uint16_t period = 50000;
volatile int16_t phase_cs = 0;
volatile int16_t phase_cs_ema = 0;
volatile int8_t phase_s = 0;

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
  //auto m = period / 10;
  auto m = phase_cs / 10;
  //auto m = phase_cs_ema / 10;
  uint16_t k = static_cast<uint16_t>(abs(m));
  for (uint8_t i = 0; i < 4; ++i) {
    uint8_t v = k % 10;
    if (i == 0 && m < 0) v |= 0x80;
    Spi.write(0x05 + i, v, LD_DATE);
    k /= 10;
  }

  auto g = static_cast<uint8_t>(abs(phase_s));
  for (uint8_t i = 0; i < 2; ++i) {
    uint8_t v = g % 10;
    if (i == 0 && phase_s < 0) v |= 0x80;
    Spi.write(0x03 + i, v, LD_DATE);
    g /= 10;
  }

  if (course) {
    Spi.write(0x01, 0x8F, LD_DATE);
  } else {
    Spi.write(0x01, 0x0F, LD_DATE);
  }
#endif

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

void blink() {
  PORTA.OUTTGL = 0b1000'0000;
}

uint16_t next_per() {
#if 1
  constexpr uint16_t divider_cs = 512;
  constexpr uint16_t gain_s = 256;
  constexpr uint16_t gain_1s = 64;
  int16_t pce = phase_cs_ema;
  if (phase_s > 1) {
    course = true;
    return period + static_cast<uint16_t>(phase_s) * gain_s;
  } else if (phase_s < -1) {
    course = true;
    return period - static_cast<uint16_t>(-phase_s) * gain_s;
  } else if (phase_s == 1) {
    if (pce < 10000) {
      pce += period;
    } else {
      course = true;
      return period + gain_1s;
    }
  } else if (phase_s == -1) {
    if (pce > -10000) {
      pce -= period;
    } else {
      course = true;
      return period - gain_1s;
    }
  }

  if (phase_cs_ema > 0) {
    course = false;
    return period + static_cast<uint16_t>(pce) / divider_cs;
  } else {
    course = false;
    return period - static_cast<uint16_t>(-pce) / divider_cs;
  }
#else
  return period;
#endif
}

ISR(TCA0_OVF_vect) {
  centiseconds = centiseconds + 1;
  if (centiseconds > 99) {
    centiseconds = 0;
#if 1
    seconds = seconds + 1;
    if (seconds > 99) {
      seconds = 0;
    }
#endif
  }
  render();

  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}

ISR(TCB1_INT_vect) {
  uint16_t count = TCA0.SINGLE.CNT;
  static bool discard = false;
  if (TCB1.INTFLAGS & TCB_OVF_bm) {
    TCB1.INTFLAGS = TCB_OVF_bm;
    discard = true;
    return;
  }
  if (discard) {
    TCB1.INTFLAGS = TCB_CAPT_bm;
    discard = false;
    return;
  }
  uint32_t value = ((uint32_t)TCB1.CCMP << 16) | TCB0.CCMP;
  value /= 100;  // Divide PPS by 100 to obtain centiseconds
  if (value > 45000 && value < 55000) {
    if (value > period) {
      period = period + (((uint16_t)value - period) >> 2);
    } else {
      period = period - ((period - (uint16_t)value) >> 2);
    }
    int16_t phase;
    while (count > period) count -= period;
    if (count < period / 2) {
      phase = static_cast<int16_t>(count);
    } else {
      phase = static_cast<int16_t>(count - period);
    }
    phase_cs = phase;
    phase_cs_ema = phase_cs_ema + ((phase - phase_cs_ema) >> 5);
    uint8_t cs = centiseconds;
    if (phase_cs < 0) cs += 1;
    if (cs > 99) cs -= 100;
    if (cs < 50) {
      phase_s = static_cast<int8_t>(centiseconds);
    } else {
      phase_s = static_cast<int8_t>(centiseconds - 100);
    }
    TCA0.SINGLE.PERBUF = next_per() - 1;
  }
}

int main() {
  cli();
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);

  Spi.initialise();

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

  sei();

  PORTA.DIRSET = LD_DATE | LD_TIME;
  PORTA.OUTCLR = LD_DATE | LD_TIME;

  init_display();
  clear_display();
  render();

  while (1);
}
