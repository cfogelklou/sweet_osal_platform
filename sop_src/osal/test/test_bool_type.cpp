
#ifdef RUN_GTEST
#include "gtest/gtest.h"
#include "utils/ble_log.h"
LOG_MODNAME("test_bool_type")

extern "C" {
int TestBoolC_GetBoolSize(void);
int TestBoolC_GetTrue(void);
int TestBoolC_GetFalse(void);
}

int TestBoolCpp_GetBoolSize() {
  return sizeof(bool);
}

int TestBoolCpp_GetTrue() {
  return (int)true;
}

int TestBoolCpp_GetFalse() {
  return (int)false;
}

TEST( BoolTest, Test1 ) {
  EXPECT_EQ( TestBoolCpp_GetBoolSize(), TestBoolC_GetBoolSize());
  EXPECT_EQ( TestBoolCpp_GetTrue(), TestBoolC_GetTrue());
  EXPECT_EQ( TestBoolCpp_GetFalse(), TestBoolC_GetFalse());
}

#endif
