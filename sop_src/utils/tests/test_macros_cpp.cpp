/**
 * COPYRIGHT    (c) Applicaudia 2018
 * @file        test_macros_cpp.cpp
 */


#ifdef RUN_GTEST

#include "tests/gtest_test_wrapper.hpp"
#include "utils/helper_macros.h"

#include "gtest/gtest.h"
#include <stdint.h>
#include <stdlib.h>

LOG_MODNAME("test_macros_cpp.cpp")


class TestMacrosCpp : public GtestMempoolsWrapper {
protected:
  TestMacrosCpp() {
  }
  virtual ~TestMacrosCpp() {
  }
};

TEST_F(TestMacrosCpp, ABS) {
  int x = 3;
  int y = 4;
  EXPECT_TRUE(ABS(x - y) == 1);
  EXPECT_TRUE(ABS(y - x) == 1);
  EXPECT_TRUE(ABS(-x + y) == 1);
  EXPECT_TRUE(ABS(-y + x) == 1);

  y = 4;
  EXPECT_TRUE(ABS(y++) == 4); // postfixed shouldn't affect result
  EXPECT_TRUE(ABS(++y) == 6); // prefixed should affeect result before

  x = -3;
  EXPECT_TRUE(ABS(x++) == 3); // postfixed shouldn't affect result
  EXPECT_TRUE(ABS(++x) == 1); // prefixed should affeect result before
}

TEST_F(TestMacrosCpp, MIN) {
  int x = 3;
  int y = 4;
  EXPECT_TRUE(MIN(x, y) == x);

  x = 10;
  y = 4;
  EXPECT_TRUE(MIN(x, y++) == 4);
  EXPECT_TRUE(y == 5);
  EXPECT_TRUE(MIN(y++, x) == 5);
  EXPECT_TRUE(y == 6);

  x = 10;
  y = 6;
  EXPECT_TRUE(MIN(x, ++y) == 7);
  EXPECT_TRUE(y == 7);
  EXPECT_TRUE(MIN(++y, x) == 8);
  EXPECT_TRUE(y == 8);
}

TEST_F(TestMacrosCpp, MAX) {
  int x = 3;
  int y = 4;
  EXPECT_TRUE(MAX(x, y) == y);

  x = 3;
  y = 4;
  EXPECT_TRUE(MAX(x, y++) == 4);
  EXPECT_TRUE(y == 5);
  EXPECT_TRUE(MAX(y++, x) == 5);
  EXPECT_TRUE(y == 6);

  x = 3;
  y = 6;
  EXPECT_TRUE(MAX(x, ++y) == 7);
  EXPECT_TRUE(y == 7);
  EXPECT_TRUE(MAX(++y, x) == 8);
  EXPECT_TRUE(y == 8);
}

TEST_F(TestMacrosCpp, ARRSZ) {
  uint8_t foo[ 10 ];
  EXPECT_TRUE(ARRSZ(foo) == 10);

  uint32_t bar[ 20 ];
  EXPECT_TRUE(ARRSZ(bar) == 20);

  uint16_t fubar[] = { 1, 2, 3, 4, 5 };
  EXPECT_TRUE(ARRSZ(fubar) == 5);
}

// Hard to test the false case here (tested manually 2018-03-08), but here is the positive test:
COMPILE_TIME_ASSERT(true);

TEST_F(TestMacrosCpp, COMPILE_TIME_ASSERT) {
  COMPILE_TIME_ASSERT(true);
}

TEST_F(TestMacrosCpp, set_bits) {
  int x = 0;
  EXPECT_TRUE(0x10 == SET_BITS(x, 0x10));
  EXPECT_TRUE(0x10 == x);

  // Check that it works for numbers larger than uint32_t
  uint64_t x64 = 0x55;
  EXPECT_TRUE(0x1000000055ull == SET_BITS(x64, 0x1000000000ull));
  EXPECT_TRUE(0x1000000055ull == x64);

  uint8_t y = 0x80;
  EXPECT_TRUE(0x93 == SET_BITS(y, 0x13));
  EXPECT_TRUE(0x93 == y);

  y = 0;
  EXPECT_TRUE(0x11 == SET_BITS(y, 0x01 | 0x10));
  EXPECT_TRUE(0x11 == y);
}

TEST_F(TestMacrosCpp, clear_bits) {
  int x = 0xff;
  EXPECT_TRUE(0xef == CLEAR_BITS(x, 0x10));
  EXPECT_TRUE(0xef == x);

  // Check that it works for numbers larger than uint32_t
  uint64_t x64 = 0x1000000055ull;
  EXPECT_TRUE(0x55ull == CLEAR_BITS(x64, 0x1000000000ull));
  EXPECT_TRUE(0x55ull == x64);

  uint8_t y = 0xff;
  EXPECT_TRUE(0xec == CLEAR_BITS(y, 0x13));
  EXPECT_TRUE(0xec == y);

  y = 0xff;
  EXPECT_TRUE(0xee == CLEAR_BITS(y, 0x01 | 0x10));
  EXPECT_TRUE(0xee == y);
}


#endif // RUN_GTEST
