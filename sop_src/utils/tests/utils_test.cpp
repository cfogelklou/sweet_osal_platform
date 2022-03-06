// Note, a majority of the utilities in utils/ subdirectory are tested by osaltest.
// This new module is a lighter wait unit test for newer modules.

#include "utils/byteq.hpp"
#include "utils/platform_log.h"
#include "utils/simple_string.hpp"

#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace std;
using namespace testing;


bool fgetpath_recursive(const char* fname, std::string& fpath) {
  const std::string up = "../";
  std::string path     = fname;
  bool open            = false;
  int maxAttempts      = 10;
  fpath                = "";
  while ((!open) && (--maxAttempts > 0)) {
    sstring newpath = path.c_str();
#ifdef WIN32
    char* pc = newpath.c8DataPtr();
    while (0 != *pc) {
      *pc = (*pc == '/') ? '\\' : *pc;
      pc++;
    }
#endif
    fstream f(newpath.c_str(), ios_base::in);
    if (!f.is_open()) {
      path = up + path;
    } else {
      fpath = path;
      f.close();
      open = true;
    }
  }
  return open;
}


class UtilsTest : public ::testing::Test {
protected:
  void SetUp() override {
  }
  void TearDown() override {
  }
};

#include "utils/q.hpp"

TEST_F(UtilsTest, Q) {
  typedef Q<uint32_t> Q32;
  uint32_t buf[ 200 ];
  const auto sz = ARRSZN(buf);
  EXPECT_EQ(sz, 200);

  Q32 q(buf, sz);
  EXPECT_EQ(0, q.GetReadReady());
  EXPECT_EQ(sz, q.GetWriteReady());

  q.Write(0);
  EXPECT_EQ(1, q.GetReadReady());
  EXPECT_EQ(sz - 1, q.GetWriteReady());
  EXPECT_EQ(0, q[ 0 ]);
  EXPECT_EQ(0, q[ -1 ]);
  q.Write(1);
  EXPECT_EQ(2, q.GetReadReady());
  EXPECT_EQ(sz - 2, q.GetWriteReady());
  EXPECT_EQ(0, q[ 0 ]);
  EXPECT_EQ(1, q[ -1 ]);
  EXPECT_EQ(1, q[ 1 ]);
  EXPECT_EQ(0, q[ -2 ]);

  for (uint32_t i = 0; i < 1024; i++) {
    q.ForceWrite(i);
    EXPECT_EQ(i, q[ -1 ]);
  }
}

TEST_F(UtilsTest, Q2) {
  typedef Q<uint32_t> Q32;
  uint32_t buf[ 200 ];
  const auto sz = ARRSZN(buf);
  Q32 q(buf, sz);
  q.Write(0x5544);
  uint32_t x = 0;
  q.PeekFromEnd(&x, 1);
  EXPECT_EQ(x, 0x5544);
}


#include "osal/osal.h"
#include "task_sched/task_sched.h"

#include <thread>


int main(int argc, char** argv) {
  OSALInit();
  TaskSchedInit();

  // The following line must be executed to initialize
  // Google Mock (and Google Test) before running the tests.
  ::testing::InitGoogleMock(&argc, argv);
  auto gtest_rval = RUN_ALL_TESTS();


  TaskSchedQuit();

  return gtest_rval;
}
