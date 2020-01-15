
#include "osal/mempools.h"
#include "gtest/gtest.h"
#include "utils/platform_log.h"

LOG_MODNAME("test_mempools")


TEST(MemPools, TestHeap) {
  // Just check that the heap doesn't get fragmented after many allocations and
  // frees.
  for (int i = 0; i < 100; i++) {
    void *p = malloc(32);
    EXPECT_TRUE(NULL != p);
    MemPoolsFree(p);
  }
}

#if !defined(__SPC5__)
#include <string>
static void stringMallocTest() {
  std::string s = "Hi";
  for (int i = 0; i < 256; i++) {
    s += "!";
  }
  EXPECT_TRUE(s.length() == 258);
}

#ifndef MEMPOOLS_USE_FREERTOS
TEST(MemPools, TestNewOverrideAndString) {
  const bool oldOverride = MemPoolsEnableNewOverride(true);
  stringMallocTest();
  MemPoolsEnableNewOverride(oldOverride);
}

#if (MEMPOOLS_DEBUG > 0)
TEST(MemPools, TestLogAllAllocations) {
  void *pMem = MemPoolsMalloc(100);
  MemPoolsPrintAllocatedMemory();
  MemPoolsFree(pMem);
}
#endif

#endif

#endif

