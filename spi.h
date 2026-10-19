#pragma once

#include "ring_buffer.h"
#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

class _Spi {
private:
  struct message {
    uint8_t payload0;
    uint8_t payload1;
    uint8_t load_mask;  // These pins are blipped high after payload
  };

  ring_buffer<message, 16> m_rb;
  message m_curr_msg = {};

  volatile enum class state {
    idle,
    payload0,
    payload1,
  } m_state;

  void write(message msg) {
    while (m_rb.full()) { }  // Blocks while buffer full
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (m_state != state::idle) {
        m_rb.push(msg);
        return;
      }
    }
    m_curr_msg = msg;
    SPI0.DATA = msg.payload0;
    m_state = state::payload0;
  }

  void blip(uint8_t mask) {
    PORTA.OUTCLR = mask;
    PORTA.OUTSET = mask;
    PORTA.OUTCLR = mask;
  }

public:
  void initialise() {
    PORTA.DIRSET = MOSI | SCLK;
    PORTMUX.SPIROUTEA = 0;
    SPI0.CTRLA = SPI_MASTER_bm |
                 SPI_PRESC_DIV4_gc;  // SPI clock = 5MHz
    SPI0.CTRLB = SPI_SSD_bm | SPI_MODE_0_gc;
    SPI0.INTCTRL = SPI_IE_bm;    // Enable interrupt
    SPI0.CTRLA |= SPI_ENABLE_bm; // Enable SPI
  }

  // Call this from main code _only_
  void write(uint8_t payload0, uint8_t payload1, uint8_t load_mask) {
    write(message{payload0, payload1, load_mask});
  }

  // Call this from interrupt handler _only_
  void next() {
    switch (m_state) {
      case state::idle: return;
      case state::payload0: {
        SPI0.DATA = m_curr_msg.payload1;
        m_state = state::payload1;
      } break;
      case state::payload1: {
        blip(m_curr_msg.load_mask);
        if (m_rb.empty()) {
          m_state = state::idle;
        } else {
          m_rb.pop(&m_curr_msg);
          SPI0.DATA = m_curr_msg.payload0;
          m_state = state::payload0;
        }
      } break;
    }
  }
} Spi;

ISR(SPI0_INT_vect) {
  SPI0.INTFLAGS = SPI_IF_bm;  // Clear interrupt
  Spi.next();
}
