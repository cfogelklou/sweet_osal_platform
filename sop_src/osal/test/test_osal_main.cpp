/**
 * COPYRIGHT    (c)	Applicaudia 2017
 * @file        test_spi_bitbang.cpp
 * @brief       Test of SPI bitbang
 */
#include "gtest/gtest.h"

#ifndef RUN_GTEST
#include <gmock/gmock.h>
using namespace testing;
#endif

#include "osal/osal.h"
#include "task_sched/task_sched.h"
#include <thread>

#ifndef RUN_GTEST
static volatile bool main_done = false;
// ////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {

  OSALInit();
  TaskSchedInit();

  static const auto idleFn = [](void* p) {
    while (!main_done) {
      TaskSchedPollIdle();
      OSALSleep(20);
    }
  };


  static std::thread t(idleFn, nullptr);
  t.detach();

  // The following line must be executed to initialize Google Mock
  // (and Google Test) before running the tests.
  ::testing::InitGoogleMock(&argc, argv);
  const int gtest_rval = RUN_ALL_TESTS();

  OSALSleep(10000);
  main_done = true;
  OSALSleep(100);

  TaskSchedQuit();
  OSALSleep(100);

  return gtest_rval;

}
#endif
