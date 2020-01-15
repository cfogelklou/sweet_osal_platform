/**
 * COPYRIGHT    (c)	Applicaudia 2017
 * @file        tone_ble_crypto.cpp
 * @brief       Test of freertos OS on Windows.
 */
#include "platform/platform_type.h"
#include "task_sched/task_sched.h"
#include "platform/osal.h"
#include "platform/mempools.h"
#include "tests/gtest_test_wrapper.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#if defined(__FREERTOS__)
/* Kernel includes. */
#include "portmacro.h"
#include <FreeRTOS.h>
#include "task.h"
#endif

#include <string.h>


LOG_MODNAME("test_freertos.cpp")

typedef struct MainTestDataTag {
  TaskSchedulable sched;
  volatile bool quit;
  int testsRval;
  int argc;
  char ** argv;
} MainTestDataT;

static MainTestDataT mainTestData = { };

static void main_TestTask(void *pCallbackData, uint32_t timeOrTicks) {
  
  GTestRandomFileInitializer randInit;

  testing::InitGoogleTest(&mainTestData.argc, mainTestData.argv);

#ifdef __FREERTOS__
  OSALFreertosSwitchToMempools(true);
#endif

  mainTestData.testsRval = RUN_ALL_TESTS();

  mainTestData.quit = true;

}

#if defined(__FREERTOS__)
/* Kernel includes. */
#include "portmacro.h"
#include <FreeRTOS.h>
#include "task.h"
#endif

int main(int argc, char** argv) {

  OSALInit();
#ifdef __FREERTOS__
  OSALFreertosSwitchToMempools(true);
#endif
  TaskSchedInit();

  mainTestData.argc = argc;
  mainTestData.argv = argv;

  TaskSchedInitSched(&mainTestData.sched, main_TestTask, &mainTestData);
  TaskSchedAddTimerFn(TS_PRIO_APP, &mainTestData.sched, 0, 0);

#ifdef __FREERTOS__
  vTaskStartScheduler();
#endif

  // Should never get here in __FREERTOS__ case, vTaskStartScheduler doesn't return.
  while (!mainTestData.quit) {
    OSALSleep(100);
  }

  TaskSchedQuit();

  return mainTestData.testsRval;
}



#ifdef __FREERTOS__
extern "C" {
  // ----------------------------------------------------------------------------
  void vApplicationIdleHook(void)
  {
    if (!(mainTestData.quit)) {
      TaskSchedPollIdle();
    }
    else {
      exit(mainTestData.testsRval);
    }
  }

  // ----------------------------------------------------------------------------
  void vApplicationStackOverflowHook(void) {
    LOG_ASSERT(false);
  }

  // ----------------------------------------------------------------------------
  /* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
  implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
  used by the Idle task. */
  void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
    StackType_t **ppxIdleTaskStackBuffer,
    uint32_t *pulIdleTaskStackSize)
  {
    /* If the buffers to be provided to the Idle task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  }

}
#endif