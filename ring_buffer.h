#pragma once

#include "stddef.h"

template<typename T, uint8_t N>
class ring_buffer {
  static_assert(N < 255);

  T data[N];
  uint8_t head = 0;
  uint8_t tail = 0;
  uint8_t count = 0;

public:
  constexpr uint8_t size() const {
    return count;
  }

  constexpr uint8_t capacity() const {
    return N;
  }

  constexpr bool empty() const {
    return count == 0;
  }

  constexpr bool full() const {
    return count == N;
  }

  bool push(const T& value) {
    if (full())
      return false;

    data[head] = value;

    if (++head == N)
      head = 0;

    ++count;
    return true;
  }

  bool pop(T* value) {
    if (!value) return false;
    if (empty()) return false;

    *value = data[tail];

    if (++tail == N)
      tail = 0;

    --count;
    return true;
  }

  void clear() {
    head = 0;
    tail = 0;
    count = 0;
  }

  const T* front() const {
    return empty() ? nullptr : &data[tail];
  }
};
