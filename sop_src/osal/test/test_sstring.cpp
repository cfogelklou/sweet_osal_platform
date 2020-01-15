#ifdef RUN_GTEST
#include <string.h>
#include "gtest/gtest.h"
#include "platform/osal.h"
#include "utils/ble_log.h"
#include "utils/util_vsnprintf.h"
#include "utils/helper_macros.h"
#include "utils/convert_utils.h"
#include "tests/gtest_test_wrapper.hpp"
#include <string.h>
LOG_MODNAME("test_sstring.cpp");

class TestSstring : public GtestMempoolsWrapper {
public:
  TestSstring(){}
  ~TestSstring() {}
};

TEST_F(TestSstring, test_vsnprintf){
  char dst[20];
  {
    const char goal[] = "hex = 0x12345678";
    int dstlen = util_snprintf(dst, ARRSZ(dst), "hex = 0x%x", 0x12345678);
    ASSERT_TRUE( dstlen == strlen(goal));
    ASSERT_TRUE( 0 == memcmp(goal, dst, dstlen));
  }
  {
#ifdef USE_UTIL_SPRINTF
    const char goal[] = "hex = 0x00000678";
#else
    const char goal[] = "hex = 0x678";
#endif
    int dstlen = util_snprintf(dst, ARRSZ(dst), "hex = 0x%x", 0x00000678);
    ASSERT_TRUE( dstlen == strlen(goal));
    ASSERT_TRUE( 0 == memcmp(goal, dst, dstlen));
  }
  {
    const char goal[] = "dec = 22";
    int dstlen = util_snprintf(dst, ARRSZ(dst), "dec = %d", 22);
    ASSERT_TRUE( dstlen == strlen(goal));
    ASSERT_TRUE( 0 == memcmp(goal, dst, dstlen));
  }
  {
    const char goal[] = "dec = -22";
    int dstlen = util_snprintf(dst, ARRSZ(dst), "dec = %d", -22);
    ASSERT_TRUE( dstlen == strlen(goal));
    ASSERT_TRUE( 0 == memcmp(goal, dst, dstlen));
  }
}


TEST_F(TestSstring, cnv_atoi) {
  int r = CNV_atoi("1");
  EXPECT_EQ(r, 1);

  r = CNV_atoi("01");
  EXPECT_EQ(r, 1);

  r = CNV_atoi("001");
  EXPECT_EQ(r, 1);

  r = CNV_atoi("10");
  EXPECT_EQ(r, 10);

  r = CNV_atoi("100");
  EXPECT_EQ(r, 100);

  r = CNV_atoi("100000");
  EXPECT_EQ(r, 100000);

  r = CNV_atoi("1234567");
  EXPECT_EQ(r, 1234567);

  r = CNV_atoi("89102");
  EXPECT_EQ(r, 89102);

}

#endif
