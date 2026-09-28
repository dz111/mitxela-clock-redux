#ifndef AVR_TEST_H
#define AVR_TEST_H

#include <avr/interrupt.h>
#include <stdint.h>
#include <avr/io.h>

namespace avr_test {

/*
 * Test registration
 */
using test_function = void (*)();

struct test_case {
  const char* name;
  test_function function;
};

#define AVR_TEST(name)                               \
  static void test_##name()

#define AVR_CASES static const avr_test::test_case test_cases[] =

#define AVR_CASE(name) { #name, test_##name }

/*

#define AVR_TEST(name) \
  static void name();                                 \
  [[gnu::used]]                                        \
  static ::avr_test::test_case name##_test_case = {   \
    #name,                                             \
    name,                                              \
    nullptr                                            \
  };                                                   \
  [[gnu::used]] static ::avr_test::test_registration   \
    name##_registration(name##_test_case);             \
  static void name()

inline test_case* first_test = nullptr;
inline test_case* last_test = nullptr;

class test_registration {
public:
  explicit test_registration(test_case& test) {
    if (last_test) {
      last_test->next = &test;
    } else {
      first_test = &test;
    }
    last_test = &test;
  }
};

*/

/*
 * UPDI UART functions
 */
#define UPDI_UART_ENABLE   0x01
#define UPDI_UART_RX_READY 0x02

#define SYSCFG_OCDM  _SFR_MEM8(0x0F18)
#define SYSCFG_OCDMS _SFR_MEM8(0x0F19)

inline bool updi_uart_enabled() {
  return GPIOR2 & UPDI_UART_ENABLE;
}

void write(uint8_t c) {
  if (!updi_uart_enabled()) return;
  while (SYSCFG_OCDMS) { }
  SYSCFG_OCDM = c;
}

int read() {
  if (!(GPIOR2 & UPDI_UART_RX_READY)) return -1;
  int value = GPIOR3;
  GPIOR2 &= ~UPDI_UART_RX_READY;
  return value;
}


/*
 * Simple output helpers.
 */

inline void print(const char* str) {
  while (*str) {
    write(*str++);
  }
}

inline void print_uint(uint32_t value) {
  char buffer[10];
  uint8_t i = 0;

  if (value == 0) {
    write('0');
    return;
  }

  while (value != 0) {
    buffer[i++] = '0' + (value % 10);
    value /= 10;
  }

  while (i != 0) {
    write(buffer[--i]);
  }
}


/*
 * Current test state.
 */
inline bool test_failed = false;


/*
 * Assertion failure.
 *
 * We deliberately don't print file/line information yet.
 * Keeping the first version small makes the generated AVR code
 * considerably easier to inspect.
 */
inline void assertion_failed(const char* expression) {
  test_failed = true;

  print("  ASSERT: ");
  print(expression);
  print("\n");
}


/*
 * Assertions.
 */

#define TEST_ASSERT(expression)                   \
  do {                                            \
    if (!(expression))                            \
      ::avr_test::assertion_failed(#expression);  \
  } while (0)

#define TEST_ASSERT_TRUE(expression)  \
  TEST_ASSERT(expression)

#define TEST_ASSERT_FALSE(expression)  \
  TEST_ASSERT(!(expression))

#define TEST_ASSERT_EQ(a, b)          \
  do {                                \
    auto _test_a = (a);               \
    auto _test_b = (b);               \
    TEST_ASSERT(_test_a == _test_b);  \
  } while (0)

#define TEST_ASSERT_NE(a, b)          \
  do {                                \
    auto _test_a = (a);               \
    auto _test_b = (b);               \
    TEST_ASSERT(_test_a != _test_b);  \
  } while (0)


/*
 * Run one test.
 */

inline bool run_test(const test_case& test) {
  print("[ RUN    ] ");
  print(test.name);
  print("\n");

  test_failed = false;

  test.function();

  if (test_failed) {
    print("[  FAILED  ] ");
    print(test.name);
    print("\n");
    return false;
  }

  print("[     OK ] ");
  print(test.name);
  print("\n");

  return true;
}


/*
 * Run a test suite.
 */

inline void run_all(const test_case* tests, uint8_t n) {
  uint8_t passed = 0;
  uint8_t failed = 0;

  print("\n");
  print("AVR Test Framework\n");
  print("==================\n\n");

  for (uint8_t i = 0; i < n; ++i) {
    if (run_test(tests[i])) {
      ++passed;
    } else {
      ++failed;
    }

    print("\n");
  }

  print("Tests: ");
  print_uint(n);

  print(", Passed: ");
  print_uint(passed);

  print(", Failed: ");
  print_uint(failed);

  print("\n\n");
}


/*
 * Interactive test runner.
 *
 * Commands:
 *
 *   r - run all tests
 *   l - list tests
 *
 * Returns immediately if no character is available.
 */
inline void poll(const test_case* tests, uint8_t n) {
  int ch = read();

  if (ch < 0)
    return;

  switch (ch) {

  case 'r':
  case 'R':
    run_all(tests, n);
    break;

  case 'l':
  case 'L': {
    for (uint8_t i = 0; i < n; ++i) {
      print_uint(i);
      print(": ");
      print(tests[i].name);
      print("\n");
    }
    print("Total ");
    print_uint(n);
    print(" test/(s)\n");
  } break;

  default:
    break;
  }
}

} // namespace avr_test

#endif

#ifdef AVR_TEST_H_MAIN

ISR(TCA0_OVF_vect) {
  TCA0.SINGLE.INTFLAGS = 1;  // Clear interrupt
  PORTA.OUTTGL = 0b1000'0000;  // Toggle A7 (LED)
}

int main() {
  // Disable pre-scaler (i.e. CLK_PER = 20MHz)
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);

  PORTA.DIRSET = 0b1000'0000;  // Output A7 (LED)

  TCA0.SINGLE.PER = 19531;  // 19531 * 1024 = 19,999,744 ~1Hz
  TCA0.SINGLE.CTRLA = (0x7 << 1);  // Set prescaler to 1/1024
  TCA0.SINGLE.CTRLB = 0x0;  // Count from 0 to PER then overflow
  TCA0.SINGLE.INTCTRL = 1;  // Enable interrupt
  TCA0.SINGLE.CTRLA |= 1;  // Enable timer

  sei();

  bool uart_on = false;

  while (1) {
    if (avr_test::updi_uart_enabled()) {
      if (!uart_on) {
        uart_on = true;
        avr_test::print("[L]ist tests  [R]un tests\n");
      }
      avr_test::poll(test_cases, sizeof(test_cases) / sizeof(avr_test::test_case));
    } else {
      uart_on = false;
    }
  }
  return 0;
}

#endif
