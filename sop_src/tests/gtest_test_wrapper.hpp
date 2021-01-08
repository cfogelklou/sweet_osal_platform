/**
* COPYRIGHT    (c) Applicaudia 2017
* @file        gtest_test_wrapper.hpp
* @brief       Wrappers to be reused in unit tests to ease set up and teardown
*              of test cases.
*/

#ifndef GTEST_TEST_WRAPPER
#define GTEST_TEST_WRAPPER
#include "gtest/gtest.h"
#include "osal/mempools.h"
#include "osal/platform_type.h"
#include "osal/osal.h"
#include "utils/platform_log.h"

#ifdef __cplusplus

#if (PLATFORM_FULL_OS > 0) || defined(EMSCRIPTEN)
// This takes care of checking for memory leaks.
class GtestMempoolsWrapper : public ::testing::Test {
private:
  LOG_MODNAME("GtestMempoolsWrapper");
protected:

  GtestMempoolsWrapper(const uint32_t allowMemToSettleSleepTime = 0)
    : mOriginalAllocs(0)
    , mNewOverridden(false)
    , mAllowMemToSettleSleepTime(allowMemToSettleSleepTime)
  {
  }

  virtual ~GtestMempoolsWrapper() {
  }

  virtual void SetUp() override {
#if (!NO_MEMPOOLS)
    // We assume that heap might store stuff that would normally be "static"
    mOriginalAllocs = MemPoolsGetAllocationsWithAllocId(0);
    mNewOverridden = MemPoolsEnableNewOverride(true);
#endif
  }

  virtual void TearDown() override {
#if (!NO_MEMPOOLS)

    if (mAllowMemToSettleSleepTime) {
      OSALSleep(mAllowMemToSettleSleepTime);
    }

    MemPoolsEnableNewOverride(mNewOverridden);
    const int newAllocs = MemPoolsGetAllocationsWithAllocId(0);
    if (newAllocs > mOriginalAllocs) {
      LOG_ASSERT_WARN(newAllocs <= mOriginalAllocs);
      LOG_Log("\r\n***Memory leak detected in test. Was %u, now %d\r\n***", mOriginalAllocs, newAllocs);
      MemPoolsPrintAllocatedMemory();
    }
#endif
  }
protected:
  int mOriginalAllocs;
  bool mNewOverridden;
  const int mAllowMemToSettleSleepTime;
};
#else
// Dummy wrapper for embedded targets.
class GtestMempoolsWrapper {
private:
protected:

  GtestMempoolsWrapper(
      const uint32_t allowMemToSettleSleepTime = 0){
    (void)allowMemToSettleSleepTime;
  }

  virtual ~GtestMempoolsWrapper() {}

  virtual void SetUp() {}
  virtual void TearDown() {}
};
#endif

// Class to be used in windows/linux/osx and ensure that OSALRandom() is at
// least started with entropy left over from last execution.
// Instantiate it in the main() for the testapp, and the destructor will
// ensure that a new random number is written to rand.bin.
#if (PLATFORM_FULL_OS > 0) || defined(EMSCRIPTEN)
#include <fstream>
#endif
class GTestRandomFileInitializer {
public:
  GTestRandomFileInitializer() {
#if (PLATFORM_FULL_OS > 0)
    std::fstream f("rand.bin", std::ios_base::in);
    char tmp[64];
    f.read(tmp, sizeof(tmp));
    OSALRandomSeed((uint8_t *)tmp, sizeof(tmp));
    f.close();
#endif
  }

  ~GTestRandomFileInitializer() {
#if (PLATFORM_FULL_OS > 0)
    char tmp[64];
    OSALRandom((uint8_t *)tmp, sizeof(tmp));
    std::fstream f("rand.bin", std::ios_base::out);
    f.write(tmp, sizeof(tmp));
    f.close();
#endif
  }
};

#endif

#endif
