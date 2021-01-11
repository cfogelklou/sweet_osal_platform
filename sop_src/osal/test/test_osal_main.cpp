/**
 * COPYRIGHT    (c)	Applicaudia 2017
 * @file        test_osal_main.cpp
 * @brief       Test of OSAL
 */
#include "gtest/gtest.h"

//#ifndef RUN_GTEST
#include <gmock/gmock.h>

//#endif

#include "osal/osal.h"
#include "task_sched/task_sched.h"
#include <iostream>
#include <thread>

using namespace testing;
using namespace std;

//#ifndef RUN_GTEST
static volatile bool main_done = false;
// ////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  cout << "OSALInit()" << endl;

  OSALInit();

  cout << "TaskSchedInit" << endl;
  TaskSchedInit();

  #ifndef OSAL_SINGLE_TASK
  static const auto idleFn = [](void* p) {
    while (!main_done) {
      TaskSchedPollIdle();
      OSALSleep(20);
    }
  };


  static std::thread t(idleFn, nullptr);
  t.detach();
  #endif

  cout << "InitGoogleMock" << endl;
  // The following line must be executed to initialize Google Mock
  // (and Google Test) before running the tests.
  ::testing::InitGoogleMock(&argc, argv);

  cout << "RUN_ALL_TESTS" << endl;  
  const int gtest_rval = RUN_ALL_TESTS();

  OSALSleep(10000);
  main_done = true;
  OSALSleep(100);

  TaskSchedQuit();
  OSALSleep(100);

  return gtest_rval;

}
//#endif
