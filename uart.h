#pragma once

#include "ring_buffer.h"

/* NEO-6M:
 *  9600 baud, 8 bits, no parity bit, 1 stop bit
 */

class _Uart {
public:
  ring_buffer<uint8_t, 64> buffer;

  void initialise() {
    PORTB.DIRCLR = RXD;  // Set as input
    USART0.BAUD = 8333;
    USART0.CTRLA = USART_RXCIE_bm; // Enable recv interrupt
    USART0.CTRLB = USART_RXMODE_NORMAL_gc;
    USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc;
    USART0.CTRLC |= USART_PMODE_DISABLED_gc;
    USART0.CTRLC |= USART_CHSIZE_8BIT_gc;
    USART0.CTRLC |= USART_SBMODE_1BIT_gc;
    USART0.CTRLB |= USART_RXEN_bm;  // Enable receiver
  }
} Uart;

ISR(USART0_RXC_vect) {
  uint8_t c = USART0.RXDATAL;
  Uart.buffer.push(c);
}
