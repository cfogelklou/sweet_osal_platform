/**
 * COPYRIGHT    (c)	Applicaudia 2018
 * @file        tone_ble_crypto.cpp
 * @brief       Test of BLE Cryptography Primitives.
 */
#include <gtest/gtest.h>



#ifndef RUN_GTEST
#include <gmock/gmock.h>
using namespace testing;
#endif

#include "utils/byteq.h"
#include "gtest/gtest.h"
#include "tests/gtest_test_wrapper.hpp"
#include "buf_io/buf_io.hpp"
#include "buf_io/buf_io_queue.hpp"
#include "utils/crc.h"
#include "task_sched/task_sched.h"

#define TRANS_TEST_POOL_ID 0x44

LOG_MODNAME("test_transport.cpp")

class TestTransport : public GtestMempoolsWrapper {
protected:
  TestTransport() {
#ifdef WIN32
    OSALMSHookToHardware(false);
#endif
  }
  virtual ~TestTransport() {
#ifdef WIN32
    OSALMSHookToHardware(true);
#endif
  }
};


TEST_F(TestTransport, test_txrx_huge) 
{
  // TODO: Currently no tests, fill in
  OSALSleep(5000);

}



#ifndef RUN_GTEST
int main(int argc, char** argv) {

  OSALInit();
  TaskSchedInit();

  int gtest_rval;
  {
      GTestRandomFileInitializer rndFile;

      // The following line must be executed to initialize Google Mock
      // (and Google Test) before running the tests.
      ::testing::InitGoogleMock(&argc, argv);

      gtest_rval = RUN_ALL_TESTS();

  }
  MemPoolsPrintUsage();

  TaskSchedQuit();

  return gtest_rval;

}
#endif
