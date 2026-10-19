#define TIME_OFFSET_OVERRIDE_TABLE 1
#include "clock.h"

#ifdef __AVR
#include "avr_test.h"
#else
#include "host_test.h"
#endif  // __AVR

bool verify_clock_utc(const ClockTime& actual, uint8_t century,
                      uint8_t year, uint8_t month, uint8_t day,
                      uint8_t hour, uint8_t min, uint8_t sec) {
    return (actual.century == century) &&
           (actual.year    == year)    &&
           (actual.month   == month)   &&
           (actual.day     == day)     &&
           (actual.hour    == hour)    &&
           (actual.min     == min)     &&
           (actual.sec     == sec)     &&
           (actual.dst     == false)   &&
           (actual.utc     == true);
}

#ifdef __AVR
#define print_clock(...)
#else
void print_clock(const ClockTime& clock) {
  printf("%02d%02d-%02d-%02dT%02d:%02d:%02d",
         clock.century, clock.year, clock.month, clock.day,
         clock.hour, clock.min, clock.sec);
  if (clock.utc) {
    if (clock.dst) {
      printf("???\n");
    } else {
      printf("Z\n");
    }
  } else {
    if (clock.dst) {
      printf("DST\n");
    } else {
      printf("LCL\n");
    }
  }
}
#endif  // __AVR

AVR_TEST(test_epoch_start) {
    // 0 seconds = 1 January 2020 00:00:00 UTC
    ClockTime result = sec_to_clock_utc(0);
    print_clock(result);
    TEST_ASSERT(verify_clock_utc(result, 20, 20, 1, 1, 0, 0, 0));
}

AVR_TEST(test_small_increment) {
    // 12 hours, 34 minutes, 56 seconds into Day 1
    // (12 * 3600) + (34 * 60) + 56 = 45296
    ClockTime result = sec_to_clock_utc(45296);
    print_clock(result);
    TEST_ASSERT(verify_clock_utc(result, 20, 20, 1, 1, 12, 34, 56));
}

AVR_TEST(test_jan_to_feb_rollover) {
    // 31 days into 2020 = 1 February 2020 00:00:00 UTC
    // 31 * 86400 = 2678400
    ClockTime result = sec_to_clock_utc(2678400);
    print_clock(result);
    TEST_ASSERT(verify_clock_utc(result, 20, 20, 2, 1, 0, 0, 0));
}

AVR_TEST(test_leap_day_2020) {
    // 29 February 2020 12:00:00 UTC
    // Jan(31) + Feb(28 elapsed) = 59 days. Plus 12 hours.
    // (59 * 86400) + 43200 = 5140800
    ClockTime result = sec_to_clock_utc(5140800);
    print_clock(result);
    TEST_ASSERT(verify_clock_utc(result, 20, 20, 2, 29, 12, 0, 0));
}

AVR_TEST(test_post_leap_march_rollover) {
    // 1 March 2020 00:00:00 UTC (Verifies Feb 29 rollover was counted)
    // Jan(31) + Feb(29) = 60 days
    // 60 * 86400 = 5184000
    ClockTime result = sec_to_clock_utc(5184000);
    print_clock(result);
    TEST_ASSERT(verify_clock_utc(result, 20, 20, 3, 1, 0, 0, 0));
}

AVR_TEST(test_start_of_year_2021) {
    // 1 January 2021 00:00:00 UTC
    // 2020 (366 days)
    // 366 * 86400 = 31,622,400
    ClockTime result = sec_to_clock_utc(31'622'400);
    print_clock(result);
    TEST_ASSERT(verify_clock_utc(result, 20, 21, 1, 1, 0, 0, 0));
}

AVR_TEST(test_non_leap_year_2021) {
    // 1 March 2021 00:00:00 UTC (Verifies 2021 Feb correctly drops to 28 days)
    // 2020(366) + Jan 2021(31) + Feb 2021(28) = 425 days
    // 425 * 86400 = 36720000
    ClockTime result = sec_to_clock_utc(36720000);
    print_clock(result);
    TEST_ASSERT(verify_clock_utc(result, 20, 21, 3, 1, 0, 0, 0));
}

AVR_TEST(test_mid_decade_rollover) {
    // 15 May 2026 18:30:00 UTC
    // Elapsed full years: 2020(366), 2021(365), 2022(365), 2023(365),
    //                     2024(366), 2025(365) = 2192 days
    // Months in 2026: Jan(31), Feb(28), Mar(31), Apr(30) = 120 days
    // Total complete days = 2192 + 120 + 14 (elapsed days in May) = 2326 days
    // Seconds: (2326 * 86400) + (18 * 3600) + (30 * 60)
    //        = 200,966,400 + 66,600 = 201,033,000
    ClockTime result = sec_to_clock_utc(201'033'000);
    print_clock(result);
    TEST_ASSERT(verify_clock_utc(result, 20, 26, 5, 15, 18, 30, 0));
}

AVR_TEST(test_year_end_boundary) {
    // 31 December 2022 23:59:59 UTC
    ClockTime result = sec_to_clock_utc(94694399);
    print_clock(result);
    TEST_ASSERT(verify_clock_utc(result, 20, 22, 12, 31, 23, 59, 59));
}

AVR_TEST(test_late_century_leap) {
    // 29 February 2096 23:59:59 UTC
    ClockTime result = sec_to_clock_utc(2403561599);
    print_clock(result);
    TEST_ASSERT(verify_clock_utc(result, 20, 96, 2, 29, 23, 59, 59));
}

AVR_TEST(test_leap_to_standard_year_boundary) {
    // 1 January 2025 00:00:00 UTC
    ClockTime result = sec_to_clock_utc(157852800);
    print_clock(result);
    TEST_ASSERT(verify_clock_utc(result, 20, 25, 1, 1, 0, 0, 0));
}

AVR_TEST(test_consecutive_31_day_months) {
    // 1 August 2023 00:00:00 UTC
    ClockTime result = sec_to_clock_utc(113011200);
    print_clock(result);
    TEST_ASSERT(verify_clock_utc(result, 20, 23, 8, 1, 0, 0, 0));
}

AVR_TEST(test_end_of_century_20xx) {
    // 31 December 2099 23:59:59 UTC
    ClockTime result = sec_to_clock_utc(2524607999);
    print_clock(result);
    TEST_ASSERT(verify_clock_utc(result, 20, 99, 12, 31, 23, 59, 59));
}


// Test timezone data is for Aus EST/EDT from 1 Jan 2020 to end of 2025
static const TzOffset test_offsets[] = {
// start, offset, dst
  {0, 660, 1},          // epoch
  {8179200, 600, 0},    // apr 2020
  {23904000, 660, 1},   // oct 2020
  {39628800, 600, 0},   // apr 2021
  {55353600, 660, 1},   // oct 2021
  {71078400, 600, 0},   // apr 2022
  {86803200, 660, 1},   // oct 2022
  {102528000, 600, 0},  // apr 2023
  {118252800, 660, 1},  // oct 2023
  {134582400, 600, 0},  // apr 2024
  {150307200, 660, 1},  // oct 2024
  {166032000, 600, 0},  // apr 2025
  {181756800, 660, 1},  // oct 2025
};

constexpr uint8_t test_offset_len = sizeof(test_offsets) / sizeof(TzOffset);

bool verify_clock_lst(const ClockTime& actual, uint8_t century,
                      uint8_t year, uint8_t month, uint8_t day,
                      uint8_t hour, uint8_t min, uint8_t sec) {
    return (actual.century == century) &&
           (actual.year    == year)    &&
           (actual.month   == month)   &&
           (actual.day     == day)     &&
           (actual.hour    == hour)    &&
           (actual.min     == min)     &&
           (actual.sec     == sec)     &&
           (actual.dst     == false)   &&
           (actual.utc     == false);
}

bool verify_clock_dst(const ClockTime& actual, uint8_t century,
                      uint8_t year, uint8_t month, uint8_t day,
                      uint8_t hour, uint8_t min, uint8_t sec) {
    return (actual.century == century) &&
           (actual.year    == year)    &&
           (actual.month   == month)   &&
           (actual.day     == day)     &&
           (actual.hour    == hour)    &&
           (actual.min     == min)     &&
           (actual.sec     == sec)     &&
           (actual.dst     == true)    &&
           (actual.utc     == false);
}

AVR_TEST(test_local_at_epoch) {
  // 1 January 2020 midnight UTC = 11am
  ClockTime result = sec_to_clock_local(0, test_offsets, test_offset_len);
  print_clock(result);
  TEST_ASSERT(verify_clock_dst(result, 20, 20, 1, 1, 11, 0, 0));
}

AVR_TEST(test_local_random_day_2025) {
  // 16 July 2025 23:59:59 UTC = 17th at 9:59:59am AEST
  ClockTime result = sec_to_clock_local(174'873'599, test_offsets, test_offset_len);
  print_clock(result);
  TEST_ASSERT(verify_clock_lst(result, 20, 25, 7, 17, 9, 59, 59));
}

AVR_TEST(test_local_before_est_transition) {
  // 4 April 2020 15:59:59 UTC = 5th at 2:59:59am AEDT
  ClockTime result = sec_to_clock_local(8'179'199, test_offsets, test_offset_len);
  print_clock(result);
  TEST_ASSERT(verify_clock_dst(result, 20, 20, 4, 5, 2, 59, 59));
}

AVR_TEST(test_local_on_est_transition) {
  // 4 April 2020 16:00:00 UTC = 5th at 2am AEST / 3am AEDT
  ClockTime result = sec_to_clock_local(8'179'200, test_offsets, test_offset_len);
  print_clock(result);
  TEST_ASSERT(verify_clock_lst(result, 20, 20, 4, 5, 2, 0, 0));
}

AVR_TEST(test_local_before_dst_transition) {
  // 3 Oct 2020 15:59:59 UTC = 4th at 1:59:59am AEST
  ClockTime result = sec_to_clock_local(23'903'999, test_offsets, test_offset_len);
  print_clock(result);
  TEST_ASSERT(verify_clock_lst(result, 20, 20, 10, 4, 1, 59, 59));
}

AVR_TEST(test_local_on_dst_transition) {
  // 3 Oct 2020 16:00:00 UTC = 4th at 3am AEDT / 2am AEST
  ClockTime result = sec_to_clock_local(23'904'000, test_offsets, test_offset_len);
  print_clock(result);
  TEST_ASSERT(verify_clock_dst(result, 20, 20, 10, 4, 3, 0, 0));
}

AVR_CASES {
    AVR_CASE(test_epoch_start),
    AVR_CASE(test_small_increment),
    AVR_CASE(test_jan_to_feb_rollover),
    AVR_CASE(test_leap_day_2020),
    AVR_CASE(test_post_leap_march_rollover),
    AVR_CASE(test_start_of_year_2021),
    AVR_CASE(test_non_leap_year_2021),
    AVR_CASE(test_mid_decade_rollover),
    AVR_CASE(test_year_end_boundary),
    AVR_CASE(test_late_century_leap),
    AVR_CASE(test_leap_to_standard_year_boundary),
    AVR_CASE(test_consecutive_31_day_months),
    AVR_CASE(test_end_of_century_20xx),
    AVR_CASE(test_local_at_epoch),
    AVR_CASE(test_local_random_day_2025),
    AVR_CASE(test_local_before_est_transition),
    AVR_CASE(test_local_on_est_transition),
    AVR_CASE(test_local_before_dst_transition),
    AVR_CASE(test_local_on_dst_transition),
};

#ifdef __AVR
#define AVR_TEST_H_MAIN
#include "avr_test.h"
#else
int main() {
  AVR_RUN();
  return 0;
}
#endif  // __AVR
