#pragma once

#include <stdint.h>
#include <stdio.h>

#define AVR_TEST(name) static void test_##name()
#define AVR_CASES static const avr_test::test_case test_cases[] =
#define AVR_CASE(name) { #name, test_##name }
#define AVR_INT() avr_test::interactive(test_cases, \
                    sizeof(test_cases) / sizeof(avr_test::test_case))
#define AVR_RUN() avr_test::run(test_cases, \
                    sizeof(test_cases) / sizeof(avr_test::test_case))

#define TEST_ASSERT(expression)                   \
  do {                                            \
    if (!(expression))                            \
      ::avr_test::assertion_failed(#expression);  \
  } while (0)

namespace avr_test {

/*
 * Test registration
 */
using test_function = void (*)();

struct test_case {
  const char* name;
  test_function function;
};


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

  printf("  ASSERT: %s\n", expression);
}

/*
 * Run one test.
 */

inline bool run_test(const test_case& test) {
  printf("[ RUN    ] %s\n", test.name);

  test_failed = false;

  test.function();

  if (test_failed) {
    printf("[  FAILED  ] %s\n", test.name);
    return false;
  }

  printf("[     OK ] %s\n", test.name);

  return true;
}


/*
 * Run a test suite.
 */

inline void run_all(const test_case* tests, uint8_t n) {
  uint8_t passed = 0;
  uint8_t failed = 0;

  printf("\n");
  printf("AVR Test Framework\n");
  printf("==================\n\n");

  for (uint8_t i = 0; i < n; ++i) {
    if (run_test(tests[i])) {
      ++passed;
    } else {
      ++failed;
    }

    printf("\n");
  }

  printf("Tests: %d, Passed: %d, Failed: %d\n\n", n, passed, failed);
}


/*
 * Interactive test runner.
 *
 * Commands:
 *
 *   r - run all tests
 *   l - list tests
 *
 */

void interactive(const avr_test::test_case* tests, int n) {
  printf("[L]ist tests  [R]un tests  ^D = Exit\n");
  int ch;
  while ((ch = getchar()) != EOF) {
    switch (ch) {
      case 'r':
      case 'R':
        run_all(tests, n);
        break;

      case 'l':
      case 'L': {
        for (uint8_t i = 0; i < n; ++i) {
          printf("%d: %s\n", i, tests[i].name);
        }
        printf("Total %d test/(s)\n", n);
      } break;

      default:
        break;
    }
  }
}

void run(const avr_test::test_case* tests, int n) {
  run_all(tests, n);
}


} // namespace avr_test
