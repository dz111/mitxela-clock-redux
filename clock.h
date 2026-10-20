#pragma once

#if __AVR
#include <avr/pgmspace.h>
#endif

#include <stddef.h>
#include <stdint.h>

// Add days up to the first day of the present month
static const uint16_t MONTH_OFFSETS[] = {
  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};  // Offsets for non-leap years
static constexpr uint8_t MONTH_OFFSETS_LEN =
  sizeof(MONTH_OFFSETS) / sizeof(uint16_t);

struct ClockTime {
  uint8_t century;  // Year xx00
  uint8_t year;     // Year 00xx
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
  bool dst;
  bool utc;
};

struct TzOffset {
  uint32_t start;
  int16_t offset;
  bool dst;
};

#ifndef TIME_OFFSET_OVERRIDE_TABLE
#include "build/time_offset.h"
#endif  // TIME_OFFSET_OVERRIDE_TABLE

#if __AVR
inline TzOffset time_offset(const TzOffset* table, uint8_t offset) {
  uint8_t* eff_address = (uint8_t*)(table + offset);
  TzOffset retval;
  retval.start = pgm_read_dword(eff_address);
  retval.offset = (int16_t)pgm_read_word(eff_address + offsetof(TzOffset, offset));
  retval.dst = (bool)pgm_read_byte(eff_address + offsetof(TzOffset, dst));
  return retval;
}
#else
inline TzOffset time_offset(const TzOffset* table, uint8_t offset) {
  return table[offset];
}
#endif

#if 0
TzOffset time_offset_lookup(uint32_t seconds,
                            const TzOffset* table, uint8_t len) {
  for (uint8_t i = 1; i < len; ++i) {
    if (seconds < time_offset(table, i).start) {
      return time_offset(table, i - 1);
    }
  }
  return time_offset(table, len - 1);
}
#else
TzOffset time_offset_lookup(uint32_t seconds,
                            const TzOffset* table, uint8_t len) {
  static uint8_t i = 1;
  if (seconds < time_offset(table, i - 1).start) {
    i = 1;
  }
  for (; i < len; ++i) {
    if (seconds < time_offset(table, i).start) {
      return time_offset(table, i - 1);
    }
  }
  return time_offset(table, len - 1);
}
#endif

ClockTime sec_to_clock(uint32_t seconds, bool dst, bool utc) {
  ClockTime retval;

  uint32_t day_seconds = seconds % 86400;
  uint32_t days = seconds / 86400;

  retval.hour = (uint8_t)(day_seconds / 3600);
  uint16_t hour_seconds = day_seconds % 3600;
  retval.min = (uint8_t)(hour_seconds / 60);
  retval.sec = (uint8_t)(hour_seconds % 60);

  retval.century = 20;
  retval.year = 20;

  // There are 1461 days every 4 years (incl the leap day)
  while (days > 1461) {
    days -= 1461;
    retval.year += 4;
  }
  // Epoch year 2020 is leap year
  if (days >= 366) {
    retval.year += 1;
    days -= 366;
  }
  // Deal with remaining years
  retval.year += days / 365;
  uint16_t year_day = days % 365 + 1;

  for (uint8_t i = MONTH_OFFSETS_LEN - 1; i != 255; --i) {
    uint16_t month_offset = MONTH_OFFSETS[i];
    if (((retval.year % 4) == 0) && (i > 1)) month_offset += 1;
    if (year_day > month_offset) {
      retval.month = i + 1;
      retval.day = (uint8_t)(year_day - month_offset);
      break;
    }
  }

  retval.dst = dst;
  retval.utc = utc;

  return retval;
}

ClockTime sec_to_clock_utc(uint32_t seconds) {
  return sec_to_clock(seconds, false, true);
}

ClockTime sec_to_clock_local(uint32_t seconds,
                             const TzOffset* table, uint8_t len) {
  TzOffset offset = time_offset_lookup(seconds, table, len);
  return sec_to_clock(seconds + (uint32_t)offset.offset * 60UL,
                      offset.dst, false);
}
