#include "avr_test.h"
#include "ring_buffer.h"


AVR_TEST(empty_buffer) {
  ring_buffer<int, 4> rb;

  TEST_ASSERT_TRUE(rb.empty());
  TEST_ASSERT_FALSE(rb.full());
  TEST_ASSERT_EQ(rb.size(), 0);
}


AVR_TEST(push_one) {
  ring_buffer<int, 4> rb;

  TEST_ASSERT_TRUE(rb.push(42));

  TEST_ASSERT_FALSE(rb.empty());
  TEST_ASSERT_FALSE(rb.full());
  TEST_ASSERT_EQ(rb.size(), 1);
}


AVR_TEST(push_and_pop_one) {
  ring_buffer<int, 4> rb;

  TEST_ASSERT_TRUE(rb.push(42));

  int value = 0;

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 42);

  TEST_ASSERT_TRUE(rb.empty());
  TEST_ASSERT_FALSE(rb.full());
  TEST_ASSERT_EQ(rb.size(), 0);
}


AVR_TEST(fifo_order) {
  ring_buffer<int, 4> rb;

  TEST_ASSERT_TRUE(rb.push(10));
  TEST_ASSERT_TRUE(rb.push(20));
  TEST_ASSERT_TRUE(rb.push(30));
  TEST_ASSERT_TRUE(rb.push(40));

  int value = 0;

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 10);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 20);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 30);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 40);

  TEST_ASSERT_TRUE(rb.empty());
}


AVR_TEST(full_buffer) {
  ring_buffer<int, 4> rb;

  TEST_ASSERT_TRUE(rb.push(1));
  TEST_ASSERT_TRUE(rb.push(2));
  TEST_ASSERT_TRUE(rb.push(3));
  TEST_ASSERT_TRUE(rb.push(4));

  TEST_ASSERT_TRUE(rb.full());
  TEST_ASSERT_FALSE(rb.empty());
  TEST_ASSERT_EQ(rb.size(), 4);

  TEST_ASSERT_FALSE(rb.push(5));

  TEST_ASSERT_TRUE(rb.full());
  TEST_ASSERT_EQ(rb.size(), 4);
}


AVR_TEST(empty_pop) {
  ring_buffer<int, 4> rb;

  int value = 1234;

  TEST_ASSERT_FALSE(rb.pop(&value));

  TEST_ASSERT_EQ(value, 1234);
  TEST_ASSERT_TRUE(rb.empty());
  TEST_ASSERT_EQ(rb.size(), 0);
}


AVR_TEST(pop_then_push) {
  ring_buffer<int, 4> rb;

  TEST_ASSERT_TRUE(rb.push(1));
  TEST_ASSERT_TRUE(rb.push(2));
  TEST_ASSERT_TRUE(rb.push(3));

  int value = 0;

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 1);

  TEST_ASSERT_TRUE(rb.push(4));

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 2);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 3);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 4);

  TEST_ASSERT_TRUE(rb.empty());
}


AVR_TEST(wraparound) {
  ring_buffer<int, 4> rb;

  int value = 0;

  TEST_ASSERT_TRUE(rb.push(1));
  TEST_ASSERT_TRUE(rb.push(2));
  TEST_ASSERT_TRUE(rb.push(3));

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 1);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 2);

  TEST_ASSERT_TRUE(rb.push(4));
  TEST_ASSERT_TRUE(rb.push(5));
  TEST_ASSERT_TRUE(rb.push(6));

  TEST_ASSERT_TRUE(rb.full());
  TEST_ASSERT_EQ(rb.size(), 4);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 3);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 4);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 5);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 6);

  TEST_ASSERT_TRUE(rb.empty());
}


AVR_TEST(repeated_wraparound)
{
  ring_buffer<int, 4> rb;

  int value = 0;

  for (int i = 0; i < 32; ++i) {
    TEST_ASSERT_TRUE(rb.push(i));

    TEST_ASSERT_TRUE(rb.pop(&value));
    TEST_ASSERT_EQ(value, i);

    TEST_ASSERT_TRUE(rb.empty());
  }
}


AVR_TEST(fill_drain_refill) {
  ring_buffer<int, 4> rb;

  int value = 0;

  TEST_ASSERT_TRUE(rb.push(1));
  TEST_ASSERT_TRUE(rb.push(2));
  TEST_ASSERT_TRUE(rb.push(3));
  TEST_ASSERT_TRUE(rb.push(4));

  TEST_ASSERT_TRUE(rb.full());

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 1);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 2);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 3);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 4);

  TEST_ASSERT_TRUE(rb.empty());

  TEST_ASSERT_TRUE(rb.push(5));
  TEST_ASSERT_TRUE(rb.push(6));

  TEST_ASSERT_EQ(rb.size(), 2);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 5);

  TEST_ASSERT_TRUE(rb.pop(&value));
  TEST_ASSERT_EQ(value, 6);

  TEST_ASSERT_TRUE(rb.empty());
}


AVR_CASES {
  AVR_CASE(empty_buffer),
  AVR_CASE(push_one),
  AVR_CASE(push_and_pop_one),
  AVR_CASE(fifo_order),
  AVR_CASE(full_buffer),
  AVR_CASE(empty_pop),
  AVR_CASE(pop_then_push),
  AVR_CASE(wraparound),
  AVR_CASE(repeated_wraparound),
  AVR_CASE(fill_drain_refill),
};


#define AVR_TEST_H_MAIN
#include "avr_test.h"
