#pragma once

#include "clock.h"

#include <stddef.h>
#include <stdint.h>

#define MemoryBarrier() __asm__ __volatile__("" ::: "memory")

#ifdef DEBUG_PARSER
#include <stdio.h>
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif

struct NmeaTime {
  uint32_t sec_since_epoch;
  uint8_t status;
  uint8_t mode;
};

class _Parser {
public:
  uint8_t sequence() const {
    return m_sequence;
  }

  NmeaTime get_unsafe() const {
#if 0
    return const_cast<const NmeaTime&>(m_time_struct);
#else
    NmeaTime retval;
    retval.sec_since_epoch = m_time_struct.sec_since_epoch;
    retval.status          = m_time_struct.status;
    retval.mode            = m_time_struct.mode;
    return retval;
#endif
  }

  NmeaTime get() const {
    uint8_t seq1, seq2;
    NmeaTime retval;
    do {
      seq1 = sequence();
      MemoryBarrier();
      retval = get_unsafe();
      MemoryBarrier();
      seq2 = sequence();
    } while (seq1 & 1 || seq1 != seq2);
    return retval;
  }

  bool parse(uint8_t ch) {
    if (ch < 32 || ch > 127) {
      LOG("Invalid char: %d. Reset.\n", ch);
      reset();
      return false;
    }
    if (m_state != state::idle &&
        ch != '*' &&
        m_state != state::checksum) {
      m_checksum ^= ch;
    }
    if (ch == '$') {
      m_state = state::header;
      m_field_length = 0;
      m_checksum = 0;
      LOG("Found '$'.\n");
      return false;
    }
    switch (m_state) {
      case state::idle: break;
      case state::skip_fields: {
        if (ch == ',') {
          if (--m_fields_to_skip == 0) {
            m_state = m_next_state;
            m_field_length = 0;
          }
          LOG("Found ',', %d fields to skip.\n", m_fields_to_skip);
        }
      } break;
      case state::header: {
        ++m_field_length;
        if (m_field_length == 3 && ch != 'R') {
          LOG("Header pos 2 expected 'R', got '%c'. Reset.\n", ch);
          reset();
        } else if (m_field_length == 4 && ch != 'M') {
          LOG("Header pos 3 expected 'M', got '%c'. Reset.\n", ch);
          reset();
        } else if (m_field_length == 5 && ch != 'C') {
          LOG("Header pos 4 expected 'C', got '%c'. Reset.\n", ch);
          reset();
        } else if (m_field_length == 6) {
          if (ch != ',') {
            LOG("Header pos 5 expected ',', got '%c'. Reset.\n", ch);
            reset();
          } else {
            m_state = state::utctime;
            m_field_length = 0;
            LOG("Got RMC header. [%02X]\n", m_checksum);
          }
        } else if (!is_alpha(ch)) {
          LOG("Header expected alpha, got '%c'. Reset.\n", ch);
          reset();
        }
      } break;
      case state::utctime: {
        if (ch == ',') {
          if (m_field_length == 6) {
            m_state = state::status;
            m_field_length = 0;
            LOG("Got time. [%02X]\n", m_checksum);
          } else {
            LOG("Time expected length 6, got length %d. Reset.\n",
                m_field_length);
            reset();
          }
        } else if (!is_digit(ch) && ch != '.') {
          LOG("Time expected digit or '.', got '%c'. Reset.\n", ch);
          reset();
        } else if (m_field_length < 6) {
          m_time[m_field_length++] = ch;
        }
      } break;
      case state::status: {
        if (ch == ',') {
          if (m_field_length == 1) {
            m_state = state::skip_fields;
            m_fields_to_skip = 6;
            m_next_state = state::utcdate;
            LOG("Got status. [%02X]\n", m_checksum);
          } else {
            LOG("Status expected length 1, got length %d. Reset.\n",
                m_field_length);
            reset();
          }
        } else if (!is_alpha(ch)) {
          LOG("Status expected alpha, got '%c'. Reset.\n", ch);
          reset();
        } else if (m_field_length < 1) {
          m_status = ch;
          ++m_field_length;
        }
      } break;
      case state::utcdate: {
        if (ch == ',') {
          if (m_field_length == 6) {
            m_state = state::skip_fields;
            m_fields_to_skip = 2;
            m_next_state = state::mode;
            LOG("Got date. [%02X]\n", m_checksum);
          } else {
            LOG("Date expected length 6, got length %d. Reset.\n",
                m_field_length);
            reset();
          }
        } else if (!is_digit(ch)) {
          LOG("Date expected digit, got '%c'. Reset.\n", ch);
          reset();
        } else if (m_field_length < 6) {
          m_date[m_field_length++] = ch;
        }
      } break;
      case state::mode: {
        if (ch == '*') {
          if (m_field_length == 1) {
            m_state = state::checksum;
            m_field_length = 0;
            LOG("Got mode. [%02X]\n", m_checksum);
          } else {
            LOG("Mode expected length 1, got length %d. Reset.\n",
                m_field_length);
            reset();
          }
        } else if (!is_alpha(ch)) {
          LOG("Mode expected alpha, got '%c'. Reset.\n", ch);
          reset();
        } else if (m_field_length < 1) {
          m_mode = ch;
          ++m_field_length;
        }
      } break;
      case state::checksum: {
        if (!is_alpha(ch) && !is_digit(ch)) {
          LOG("Checksum expected alphanum, got '%c'. Reset.\n", ch);
          reset();
        } else {
          m_recv_checksum[m_field_length++] = ch;
          if (m_field_length == 2) {
            bool valid = checksum_valid(m_checksum, m_recv_checksum);
            if (valid) {
              update_time();
              LOG("Message complete, checksum validated.\n");
              LOG("Sec since epoch: %d.\n", m_time_struct.sec_since_epoch);
            } else {
              LOG("Checksum expected '%02X', got '%c%c'.\n",
                  m_checksum, m_recv_checksum[0], m_recv_checksum[1]);
            }
            reset();
            return valid;
          }
        }
      } break;
    }
    return false;
  }

  [[gnu::always_inline]]
  static bool is_alpha(uint8_t ch) {
    return (ch >= 'A' && ch <= 'Z');
  }

  [[gnu::always_inline]]
  static bool is_digit(uint8_t ch) {
    return (ch >= '0' && ch <= '9');
  }

  [[gnu::always_inline]]
  static uint8_t digits_to_int(const uint8_t str[2]) {
    return (str[0] - '0') * 10 + (str[1] - '0');
  }

  static bool checksum_valid(uint8_t calc, uint8_t recv[2]) {
    uint8_t calc_s[2];
    calc_s[1] = calc & 0x0F;
    calc_s[0] = (calc & 0xF0) >> 4;
    for (uint8_t i = 0; i < 2; ++i) {
      uint8_t ch = calc_s[i];
      if (ch < 10) ch += '0';
      else ch += ('A' - 10);
      if (ch != recv[i]) {
        return false;
      }
    }
    return true;
  }

  template <size_t N, typename T>
  [[gnu::always_inline]]
  static auto array_cast(T* arr) -> T(&)[N] {
    return *reinterpret_cast<T(*)[N]>(arr);
  }

  // Epoch = 1 January 2020 Midnight UTC
  static uint32_t calculate_time(const uint8_t (&date)[6],
                          const uint8_t (&time)[6]) {
    uint8_t day   = digits_to_int(array_cast<2>(date));
    uint8_t month = digits_to_int(array_cast<2>(date + 2));
    uint8_t year  = digits_to_int(array_cast<2>(date + 4));
    uint8_t hour  = digits_to_int(array_cast<2>(time));
    uint8_t min   = digits_to_int(array_cast<2>(time + 2));
    uint8_t sec   = digits_to_int(array_cast<2>(time + 4));

    year -= 20; // Epoch year 2020

    // Calculate elapsed days from 2020 to present year, excl leap days
    uint16_t days = year * 365;

    // Add leap days for full years since 2020
    // 2020 (Year 0) was a leap year. For Years 1-4 (2021-24), 1 leap
    // day has passed. For Years 5-8 (2025-28), 2 leap days have passed
    // (2020 and 2024).
    days += (year + 3) / 4;

    // If the current year is a leap year and we have passed February,
    // add a leap day.
    if (month > 2 && (year % 4) == 0) {
      days++;
    }

    days += MONTH_OFFSETS[month - 1];

    // Add days within present month
    days += (day - 1);

    // Convert to seconds
    uint32_t seconds = (uint32_t)days * 86400UL;
    seconds += (uint32_t)hour * 3600UL;
    seconds += (uint32_t)min * 60UL;
    seconds += sec;

    return seconds;
  }

private:
  /* Parser state */
  enum class state : uint8_t {
    idle,
    skip_fields,
    header,
    utctime,
    status,
    utcdate,
    mode,
    checksum,
  } m_state = state::idle;
  uint8_t m_field_length = 0;
  uint8_t m_checksum = 0;
  uint8_t m_fields_to_skip;
  state m_next_state;
  uint8_t m_recv_checksum[2];

  /* Intermediate results
     These are used internally only.
   */
  uint8_t m_time[6];
  uint8_t m_status;
  uint8_t m_date[6];
  uint8_t m_mode;

  /* Results
     These must be volatile because they are accessed by more
     than one thread.
   */
  volatile uint8_t m_sequence = 0;
  volatile NmeaTime m_time_struct;

  void reset() {
    m_state = state::idle;
    m_field_length = 0;
    m_checksum = 0;
  }

  void update_time() {
    m_sequence = m_sequence + 1;
    MemoryBarrier();
    m_time_struct.sec_since_epoch = calculate_time(m_date, m_time);
    m_time_struct.status          = m_status;
    m_time_struct.mode            = m_mode;
    MemoryBarrier();
    m_sequence = m_sequence + 1;
  }
} Parser;
