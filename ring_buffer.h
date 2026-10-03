#pragma once

#include <stdint.h>

#define MemoryBarrier() __asm__ __volatile__("" ::: "memory")

/*
  ring_buffer

  Implements a circular buffer containing elements of type T and with
  capacity Cap. T must be a trivial type.

  Where the buffer is used with interrupts, the calling code must be
  arranged so that only one thread calls push and the other thread calls
  pop. In this condition, the code is race free--see discussion below.

  size, empty, and full are race free, but may be stale if the buffer
  state is modified during the call.

  clear is *not* race free and must not be run if it could be interrupted
  by a routine that calls push or pop.

  push and pop are mutually race free in a single producer single consumer
  access pattern. Each function writes/reads to/from the buffer before
  updating head/tail. Using a memory barrier to ensure correct ordering,
  this scheme ensures that data is valid before it is advertised. full
  and empty checks at the top of push/pop are safe: e.g., during a push,
  after it is established that the buffer is not full, if a pop occurs,
  it will be even less 'full'.

*/

template<typename T, uint8_t Cap>
class ring_buffer {
  static constexpr uint8_t N = Cap+1;
  static_assert(N >= 2);
  static_assert(N < 255);

  T data[N];
  volatile uint8_t head = 0;
  volatile uint8_t tail = 0;

public:
  constexpr uint8_t size() const {
    if (head >= tail) return head - tail;
    else return N + head - tail;
  }

  constexpr uint8_t capacity() const {
    return Cap;
  }

  constexpr bool empty() const {
    return head == tail;
  }

  constexpr bool full() const {
    uint8_t next = head + 1;
    if (next == N) next = 0;
    return next == tail;
  }

  bool push(const T& value) {
    if (full()) return false;

    data[head] = value;

    MemoryBarrier();

    head = head + 1;
    if (head == N) head = 0;

    return true;
  }

  bool pop(T* value) {
    if (!value) return false;
    if (empty()) return false;

    *value = data[tail];

    MemoryBarrier();

    tail = tail + 1;
    if (tail == N) tail = 0;

    return true;
  }

  void clear() {
    head = 0;
    tail = 0;
  }
};
