/**
 * COPYRIGHT    (c)	Chris Fogelklou 2020
 * @author:     Chris Fogelklou
 * @brief       Tests the generic JSON command handler.
 */

#include "json_command/json_command.hpp"
#include "task_sched/task_sched.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>


LOG_MODNAME("test_json_commands.cpp");

using namespace testing;

#ifdef WIN32
#include <Windows.h>
#define usleep(x) Sleep(x/1000)
#endif


TEST(TestJsonCommands, TestJsonCommands) {
  // TODO: Currently no tests, fill in
  OSALSleep(5000);
}



int main(int argc, char** argv){
  OSALInit();
  TaskSchedInit();
  
  // The following line must be executed to initialize Google Mock
  // (and Google Test) before running the tests.
  ::testing::InitGoogleMock(&argc, argv);
  const int gtest_rval = RUN_ALL_TESTS();

  TaskSchedQuit();
  
  return gtest_rval;
}
