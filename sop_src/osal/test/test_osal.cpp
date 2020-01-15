
#include "osal/osal.h"
#include <stdlib.h>
#include <string.h>
#include "gtest/gtest.h"
#include "osal/cs_task_locker.hpp"
#include "osal/endian_convert.h"
#include "osal/mempools.h"
#include "task_sched/task_rescheduler.hpp"
#include "utils/platform_log.h"
#include "utils/helper_macros.h"
#include "tests/gtest_test_wrapper.hpp"

LOG_MODNAME("osaltest.cpp");


class OSALTest : public GtestMempoolsWrapper {
public:
  OSALTest()
    : GtestMempoolsWrapper(50)
  {}
  ~OSALTest(){}
};


#if !defined(OSAL_SINGLE_TASK)
typedef struct CSTaskLockerTaskTag {
  TaskSchedulable sched;
  volatile uint32_t timesExecuted;

} CSTaskLockerTask;

static void cs_task_locker_test(void *p, uint32_t ts) {
  (void)ts;
  CSTaskLocker lock;
  CSTaskLockerTask *pData = (CSTaskLockerTask *)p;
  CSTaskLockerTask &data = *pData;
  const TaskSchedPriority id = TaskSched_GetCurrentPriority();
  LOG_TRACE(("Task %d reporting for duty!\r\n", id));
  data.timesExecuted++;
}


TEST_F(OSALTest, CSTaskLocker) {
  const TaskSchedPriority id = TaskSched_GetCurrentPriority();
  CSTaskLockerTask testData[TS_NUM_PRIORITIES];
  memset(&testData, 0, sizeof(testData));
  for (int i = 0; i < TS_NUM_PRIORITIES; i++) {
    CSTaskLockerTask &t = testData[i];
    TaskSchedInitSched(&t.sched, cs_task_locker_test, &t);
  }
  OSALSleep(200);
  {
    LOG_TRACE(("Locking task scheduler...\r\n", id));
    CSTaskLocker lock;
    for (int i = 0; i < TS_NUM_PRIORITIES; i++) {
      const TaskSchedPriority thisId = (const TaskSchedPriority)i;
      if (thisId != id) {
        CSTaskLockerTask &t = testData[i];
        TaskSchedAddTimerFn(thisId, &t.sched, 0, 0);
      }
    }

    // Ensure that no tasks are triggered.
    bool noTasksRan = true;
    for (volatile int wait = 0; wait < 1000000; wait++) {
      for (int i = 0; i < TS_NUM_PRIORITIES; i++) {
        CSTaskLockerTask &t = testData[i];
        noTasksRan &= (t.timesExecuted == 0);
      }
    }
    ASSERT_EQ(noTasksRan, true);
    LOG_TRACE(("Unlocking task scheduler...\r\n", id));
  }

  // Now task locker shall be released
  OSALSleep(200);
  bool someTasksRan = false;
  for (int i = 0; i < TS_NUM_PRIORITIES; i++) {
    const TaskSchedPriority thisId = (const TaskSchedPriority)i;
    CSTaskLockerTask &t = testData[i];
    TaskSchedCancelScheduledTask(&t.sched);
    if (thisId != id) {
      someTasksRan |= (t.timesExecuted > 0);
    }
  }
  ASSERT_EQ(someTasksRan, true);

}
#endif // OSAL_SINGLE_TASK



#if !defined(OSAL_SINGLE_TASK)
TEST_F(OSALTest, Rescheduler) {
  {
    const float resched_period = 100;
    const float resched_period_fast = 10;
    const float timelimit = 2000;
    const float executionsWithoutForceResched = timelimit / resched_period;
    volatile float numExecutions = 0;
    const float executionsWithForceResched = timelimit / resched_period_fast;
    const auto reschedTaskPrio = TS_PRIO_NPI;
    LOG_ASSERT(reschedTaskPrio != TaskSched_GetCurrentPriority());

    typedef struct {
      TaskRescheduler *resched;
      volatile float *pf;
    } CbData;

    CbData data = { NULL, &numExecutions };

    auto incExecutions = [](void *p, uint32_t) {
      CSTaskLocker cs;
      volatile CbData *pData = (volatile CbData *)p;
      *pData->pf = *pData->pf + 1;
      pData->resched->doReschedule(100);
    };
    TaskRescheduler resched(TSCHED_F_OCH_L, incExecutions, &data);
    data.resched = &resched;
    resched.doReschedule(100);

    {
      const uint32_t startTime = OSALGetMS();
      uint32_t timePassed = 0;
      while (timePassed < timelimit) {
        OSALSleep(100);
        const uint32_t now = OSALGetMS();
        timePassed = now - startTime;
      }
    }

    resched.disable();
    OSALSleep(500);

    EXPECT_TRUE(numExecutions >(0.8 * executionsWithoutForceResched));
    numExecutions = 0;

    auto resched_test_fast_cb = [](void *p, uint32_t) {
      TaskRescheduler *pTester = (TaskRescheduler *)p;
      pTester->forceReschedule();
    };

    resched.enable();
    TaskSchedulable sched(resched_test_fast_cb, &resched, reschedTaskPrio, 0, resched_period_fast);

    {
      const uint32_t startTime = OSALGetMS();
      uint32_t timePassed = 0;
      while (timePassed < timelimit) {
        OSALSleep(100);
        const uint32_t now = OSALGetMS();
        timePassed = now - startTime;
      }
    }

    TaskSchedCancelScheduledTask(&sched);
    resched.disable();
    OSALSleep(500);

    EXPECT_TRUE(numExecutions > (0.8 * executionsWithForceResched));
  }
  OSALSleep(500);
}
#endif // OSAL_SINGLE_TASK


// REMOVE
#if !defined(OSAL_SINGLE_TASK)
class CheckTimer {
private:
  uint32_t mStartMs;
  uint32_t mExpectedMsMin;
  uint32_t mExpectedMsMax;

  CheckTimer() : mStartMs(0), mExpectedMsMin(0), mExpectedMsMax(0) {}

public:
  CheckTimer(const uint32_t expectedMsMin, const uint32_t expectedMsMax)
      : mStartMs(OSALGetMS()), mExpectedMsMin(expectedMsMin),
        mExpectedMsMax(expectedMsMax) {}

  ~CheckTimer() {
    const uint32_t time2 = OSALGetMS();
    const uint32_t timeElapsed = time2 - mStartMs;
    if (mExpectedMsMin) {
      EXPECT_TRUE(timeElapsed >= mExpectedMsMin);
    }
    if (mExpectedMsMax) {
      EXPECT_TRUE(timeElapsed <= mExpectedMsMax);
    }
  }
};

#endif // OSAL_SINGLE_TASK

#if defined(WIN32) || defined(__linux__)
TEST_F(OSALTest, osal_CheckEndian){
  uint8_t w0[4];
  uint8_t s0[2];
  uint32_t tmpw = 0x01234567;
  uint16_t tmps = 0x0123;
  
  HostToLE32(tmpw, w0);
  HostToLE16(tmps, s0);
  LOG_ASSERT( 0 == memcmp(&tmpw, w0, sizeof(tmpw)));
  LOG_ASSERT( 0 == memcmp(&tmps, s0, sizeof(tmps)));
  
  tmpw = LE32ToHost(w0);
  LOG_ASSERT(tmpw == 0x01234567);
  tmps = LE16ToHost(s0);
  LOG_ASSERT(tmps == 0x0123);
  
  // Big endian test
  uint8_t cmp32[] = {0x01, 0x23, 0x45, 0x67};
  HostToBE32(tmpw, w0);
  LOG_ASSERT( 0 == memcmp( w0, cmp32, sizeof(tmpw) ));

  uint8_t cmp16[] = {0x01, 0x23};
  HostToBE16(tmps, s0);
  LOG_ASSERT( 0 == memcmp( s0, cmp16, sizeof(tmps) ));

}
#endif



#if !defined(OSAL_SINGLE_TASK)

TEST_F(OSALTest, osal_TestLambdaWithCapture) {

  static auto lambdaNoCap = [](uint32_t) {
    typedef struct lmTest {
      volatile int m;
    }lmTest;
    lmTest * pLmTest = new lmTest;
    pLmTest->m = 6;
    static auto lambdaCap = [pLmTest](uint32_t) {
      LOG_ASSERT(pLmTest->m == 6);
      delete pLmTest;
    };
    TaskSchedScheduleCaptureLambda(lambdaCap, 100);
    
  };

  TaskSchedScheduleLambda(TSCHED_F_OCH_L, lambdaNoCap, 100);
  OSALSleep(500);

}

/* ****************************************************************************
   Description: Test the OSALSleep() function against the OSAL timer
******************************************************************************/
TEST_F(OSALTest, osal_TestSleep) {
  uint32_t time2;
  uint32_t time = OSALGetMS();
  LOG_TRACE(("Sleeping 1\r\n"));
  OSALSleep(1);
  time2 = OSALGetMS();
  EXPECT_TRUE((time2 - time) >= 1);

  LOG_TRACE(("Sleeping 10\r\n"));
  {
    CheckTimer checkTimer(5, 20);
    OSALSleep(10);
  }

  LOG_TRACE(("Sleeping 100\r\n"));
  {
    CheckTimer checkTimer(70, 200);
    OSALSleep(100);
  }

  LOG_TRACE(("Sleeping 1000\r\n"));
  {
    CheckTimer checkTimer(970, 2000);
    OSALSleep(1000);
  }

  OSALSleep(1);
}

#endif // OSAL_SINGLE_TASK

#if !defined(OSAL_SINGLE_TASK)
/* ****************************************************************************
   Description: Checks that the MUTEX acts like a mutex and can be locked
     more than once in the same thread and that, if it is locked more than
     once, will only be unlocked by the last unlock command.
******************************************************************************/
TEST_F(OSALTest, TestMutex) {
  OSALMutexPtrT mId, mIdSave;
  OSALInit();

  uint32_t time = OSALGetMS();

  // Recursive check: Test that a mutex can be locked more than once...
  mId = OSALCreateMutex();
  mIdSave = mId;

  time = OSALGetMS();

  // This function should return immediately every time we call it
  for (unsigned int i = 0; i < 2; i++) {
    EXPECT_TRUE(OSALLockMutex(mId, 1000));
  }

  EXPECT_TRUE(OSALUnlockMutex(mId));

  EXPECT_TRUE((OSALGetMS() - time) < 100);

  auto osal_TestMutex_task1 = [](void *pParam, uint32_t timeOrTicks) {
    (void)timeOrTicks;
    uint32_t time2;
    OSALSemaphorePtrT *pMid = (OSALSemaphorePtrT *)pParam;
    uint32_t time = OSALGetMS();

    EXPECT_FALSE(OSALLockMutex(*pMid, 1000));

    time2 = OSALGetMS();
    const uint32_t timeElapsed = (time2 - time);
    EXPECT_TRUE(timeElapsed >= 990);
    EXPECT_TRUE(timeElapsed < 1200);

    // Indicate finished
    *pMid = 0;
  };

  // Create task 1, which will try to get the mutex but will FAIL!
  TaskSchedScheduleFn(TS_PRIO_APP_EVENTS, osal_TestMutex_task1, &mId, 0);

  OSALSleep(100);
  OSALSleep(2000);
  EXPECT_TRUE(mId == 0);
  mId = mIdSave;

  // Now post the mutex
  EXPECT_TRUE(OSALUnlockMutex(mId));


  auto osal_TestMutex_task2 = [](void *pParam, uint32_t timeOrTicks) {
    uint32_t time2;
    (void)timeOrTicks;
    OSALSemaphorePtrT *pMid = (OSALSemaphorePtrT *)pParam;
    uint32_t time = OSALGetMS();

    EXPECT_TRUE(OSALLockMutex(*pMid, 1000));

    time2 = OSALGetMS();

    EXPECT_TRUE((time2 - time) <= 20);

    EXPECT_TRUE(OSALUnlockMutex(*pMid));

    // Indicate finished
    *pMid = 0;
  };
  TaskSchedScheduleFn(TS_PRIO_APP_EVENTS, osal_TestMutex_task2, &mId, 0);

  OSALSleep(2000);
  EXPECT_TRUE(mId == 0);
  mId = mIdSave;
  EXPECT_TRUE(mId != 0);

  OSALDeleteMutex(&mId);
}
#endif // OSAL_SINGLE_TASK

#if !defined(OSAL_SINGLE_TASK)

TEST_F(OSALTest, TestSemaphore1) {
  OSALSemaphorePtrT pSem = OSALSemaphoreCreate(0, OSAL_COUNT_INFINITE);

  {
    CheckTimer checkTimer(90, 200);
    EXPECT_FALSE(OSALSemaphoreWait(pSem, 100));
  }

  EXPECT_TRUE(OSALSemaphoreSignal(pSem, 2));

  {
    CheckTimer check(0, 10);
    EXPECT_TRUE(OSALSemaphoreWait(pSem, 10000));
    EXPECT_TRUE(OSALSemaphoreWait(pSem, 10000));
  }

  {
    CheckTimer check(90, 200);
    EXPECT_FALSE(OSALSemaphoreWait(pSem, 100));
  }

  EXPECT_TRUE(pSem != NULL);
  EXPECT_TRUE(OSALSemaphoreDelete(&pSem));
  EXPECT_TRUE(pSem == NULL);
}

/* ****************************************************************************
   Description: See the prototype at the top of the file.
******************************************************************************/
const int TEST_SEM_TASK_SIGNALS = 100;
const int TEST_SEM_TASK_RUN_TIME = 2000;

// The TestSemaphore2 test case.
TEST_F(OSALTest, TestSemaphore2) {


  typedef struct _osaltest_TestSemaphoreDataT {
    OSALSemaphorePtrT pSem;
    volatile bool done;
  } osaltest_TestSemaphoreDataT;

  osaltest_TestSemaphoreDataT taskData;
  memset(&taskData, 0, sizeof(taskData));

  // taskData.pSem = OSALSemaphoreCreate( 0, OSAL_WAIT_INFINITE );
  taskData.pSem = OSALSemaphoreCreate(0, 0x7fffffff);
  LOG_ASSERT(taskData.pSem != NULL);

  {
    CheckTimer checkTimer(90, 200);
    EXPECT_FALSE(OSALSemaphoreWait(taskData.pSem, 100));
  }

  // This task is used by TestSemaphore2 to test the semaphore
  auto osal_TestSemaphore_task = [](void *pParam, uint32_t timeOrTicks) {
    (void)timeOrTicks;
    osaltest_TestSemaphoreDataT *pTaskData =
      (osaltest_TestSemaphoreDataT *)pParam;
    int signals = 0;
    const int loops = 20;
    const int signalsPerLoop = TEST_SEM_TASK_SIGNALS / loops;
    const int sleepTimePerLoop = TEST_SEM_TASK_RUN_TIME / loops;

    for (int iter = 0; iter < 20; iter++) {
      signals += signalsPerLoop;
      OSALSemaphoreSignal(pTaskData->pSem, signalsPerLoop);
      const uint32_t time1 = OSALGetMS();
      OSALSleep(sleepTimePerLoop);
      const uint32_t time2 = OSALGetMS();
      const uint32_t timeElapsed = (time2 - time1);
      EXPECT_TRUE(timeElapsed >= (sleepTimePerLoop * 4 / 5));
    }
    pTaskData->done = true;
  };

  TaskSchedScheduleFn(TS_PRIO_APP_EVENTS, osal_TestSemaphore_task, &taskData, 0);

  OSALSleep(100);
  {
    CheckTimer checkTimer(TEST_SEM_TASK_RUN_TIME / 2,
                          TEST_SEM_TASK_RUN_TIME * 2);

    // Wait for messages from the thread we created...
    int signals = 0;
    for (int i = 0; i < TEST_SEM_TASK_SIGNALS; i++) {
      LOG_ASSERT(taskData.pSem);
      EXPECT_TRUE(OSALSemaphoreWait(taskData.pSem, OSAL_WAIT_INFINITE));
      signals++;
    }
  }

  while(!taskData.done){
    OSALSleep(5);
  }

  EXPECT_TRUE(taskData.pSem != NULL);
  EXPECT_TRUE(OSALSemaphoreDelete(&taskData.pSem));
  EXPECT_TRUE(taskData.pSem == NULL);
}



#endif // OSAL_SINGLE_TASK

#if !defined(OSAL_SINGLE_TASK)
// The test that signals the semaphore 00 times, but the sem itself has a max
// value of 3.
TEST_F(OSALTest, TestSemaphoreMaxValue) {
  typedef struct _osaltest_TSMaxValueT {
    OSALSemaphorePtrT pSem;
    volatile int consumedCount;
  } osaltest_TSMaxValueT;


  osaltest_TSMaxValueT taskData;
  memset(&taskData, 0, sizeof(taskData));

  // taskData.pSem = OSALSemaphoreCreate( 0, OSAL_WAIT_INFINITE );
  taskData.pSem = OSALSemaphoreCreate(0, 3);
  EXPECT_TRUE(taskData.pSem != NULL);

  // This should only signal the semaphore 3 times!

  EXPECT_TRUE(OSALSemaphoreSignal(taskData.pSem, 1));
  EXPECT_TRUE(OSALSemaphoreSignal(taskData.pSem, 1));
  EXPECT_TRUE(OSALSemaphoreSignal(taskData.pSem, 1));
  EXPECT_FALSE(OSALSemaphoreSignal(taskData.pSem, 1));
  EXPECT_FALSE(OSALSemaphoreSignal(taskData.pSem, 1));

  // The thread that tries to consume the sempahore
  auto osal_TestSemaphore_ConsumerTask = [](void *pParam, uint32_t) {
    osaltest_TSMaxValueT *pTaskData = (osaltest_TSMaxValueT *)pParam;
    for (int i = 0; i < 100; i++) {
      if (OSALSemaphoreWait(pTaskData->pSem, 1000)) {
        pTaskData->consumedCount++;
      }
    }
  };

  TaskSchedScheduleFn(TS_PRIO_APP_EVENTS, osal_TestSemaphore_ConsumerTask, &taskData,   0);

  // Wait a while for the thread to start and consume 3 signals.
  OSALSleep(200);

  EXPECT_TRUE(3 == taskData.consumedCount);
  for (int i = 0; i < 97; i++) {
    EXPECT_TRUE(OSALSemaphoreSignal(taskData.pSem, 1));
    OSALSleep(5);
  }
  
  while (taskData.consumedCount != 100){
    OSALSleep(5);
  }

  EXPECT_TRUE(taskData.pSem != NULL);
  EXPECT_TRUE(OSALSemaphoreDelete(&taskData.pSem));
  EXPECT_TRUE(taskData.pSem == NULL);
}
// ////////////////////////////////////////////////////////////////////////////
// END Test of Semaphore Max Value
// ////////////////////////////////////////////////////////////////////////////

#endif // OSAL_SINGLE_TASK

#if !defined(OSAL_SINGLE_TASK)

typedef struct {
  TaskSchedulable sched;
  TaskSchedEventTrigger trig;
  uint32_t offset_ms;
  uint32_t osalMs;
  uint32_t timeOrTicksMs;
} test_scheduler_data;



TEST_F(OSALTest, scheduler) {
  bool passedTests = false;
  int retries = 0;
  bool completeFailure = false;
  while ((!passedTests) && (retries < 3) && (!completeFailure)) {
    int simultaneousExecs = 0;
    test_scheduler_data schedulables[20];
    memset(schedulables, 0, sizeof(schedulables));

    auto test_scheduler_cb = [](void *pCallbackData, uint32_t timeOrTicks) {
      test_scheduler_data *pData = (test_scheduler_data *)pCallbackData;
      LOG_TRACE(("Scheduler with offset %u ms called at ms %u\r\n", pData->offset_ms, timeOrTicks));
      EXPECT_TRUE(TS_PRIO_APP_EVENTS == TaskSched_GetCurrentPriority());
      pData->osalMs = OSALGetMS();
      pData->timeOrTicksMs = timeOrTicks;
    };

    for (int direction = 0; direction <= 1; direction++) {

      const int start = (direction) ? ARRSZ(schedulables) - 1 : 0;
      const int end = (direction) ? -1 : ARRSZ(schedulables);
      const int inc = (direction) ? -1 : 1;
      for (int i = start; i != end; i += inc) {
        TaskSchedInitSched(&schedulables[i].sched, test_scheduler_cb,
          &schedulables[i]);
        schedulables[i].offset_ms = (i + 1) * 50;
        TaskSchedAddTimerFn(TS_PRIO_APP_EVENTS, &schedulables[i].sched, 0,
          schedulables[i].offset_ms);
      }

      LOG_TRACE(("Running task scheduler for 1s.\r\n"));
      {
        uint32_t startTime = OSALGetMS();
        uint32_t timeElapsed = OSALGetMS() - startTime;
        while (timeElapsed < 3000) {
          OSALSleep(50);
          timeElapsed = OSALGetMS() - startTime;
        }
      }

      for (size_t i = 1; i < ARRSZ(schedulables); i++) {
        int32_t diff = schedulables[i].osalMs - schedulables[i - 1].osalMs;
        if (diff < 0) {
          completeFailure = true;
          LOG_TRACE(("Bad osalMs. %d < %d\r\n", schedulables[i].osalMs, schedulables[i - 1].osalMs));
        }
        else if (diff == 0) {
          simultaneousExecs++;
        }
        diff = schedulables[i].timeOrTicksMs - schedulables[i - 1].timeOrTicksMs;
        if (diff < 0) {
          completeFailure = true;
          LOG_TRACE(("Bad timeOrTicksMs. %d <= %d\r\n", schedulables[i].timeOrTicksMs, schedulables[i - 1].timeOrTicksMs));
        }
        else if (diff == 0) {
          simultaneousExecs++;
        }
      }

      LOG_TRACE(("Cancelling schedulables and running for 500 more milliseconds\r\n"));
      for (size_t i = 0; i < ARRSZ(schedulables); i++) {
        TaskSchedCancelScheduledTask(&schedulables[i].sched);
      }
      
      {
        uint32_t startTime = OSALGetMS();
        uint32_t timeElapsed = OSALGetMS() - startTime;
        while (timeElapsed < 500) {
          OSALSleep(10);
#ifdef OSAL_SINGLE_TASK
          TaskSchedPollIdle();
#endif
          timeElapsed = OSALGetMS() - startTime;
        }
      }
    }
    retries++;
    if ((!completeFailure) && (simultaneousExecs < 6)) {
      passedTests |= true;
    }
    retries++;
  }
  EXPECT_TRUE(passedTests);
}

TEST_F(OSALTest, scheduler_events) {
  bool passedTests = false;
  int retries = 0;
  bool completeFailure = false;
  while ((!passedTests) && (retries < 3) && (!completeFailure)) {
    int simultaneousExecs = 0;

    test_scheduler_data schedulables[10];
    memset(schedulables, 0, sizeof(schedulables));

    auto test_scheduler_events_cb = [](void *pCallbackData, uint32_t timeOrTicks) {
      test_scheduler_data *pData = (test_scheduler_data *)pCallbackData;
      LOG_TRACE(("Scheduler with offset %u ms called at ms %u\r\n", pData->offset_ms, timeOrTicks));
      EXPECT_TRUE(TS_PRIO_APP_EVENTS == TaskSched_GetCurrentPriority());
      TaskSchedTriggerEvent(pData->trig);
    };

    auto test_scheduler_events_triggered_cb = [](void *pCallbackData, uint32_t timeOrTicks) {
      test_scheduler_data *pData = (test_scheduler_data *)pCallbackData;
      LOG_TRACE(("Scheduler EVENT with offset %u ms called at ms %u\r\n", pData->offset_ms, timeOrTicks));
      EXPECT_TRUE(TS_PRIO_APP_EVENTS == TaskSched_GetCurrentPriority());
      pData->osalMs = OSALGetMS();
      pData->timeOrTicksMs = timeOrTicks;
    };

    for (int direction = 0; direction <= 1; direction++) {

      const int start = (direction) ? ARRSZ(schedulables) - 1 : 0;
      const int end = (direction) ? -1 : ARRSZ(schedulables);
      const int inc = (direction) ? -1 : 1;
      for (int i = start; i != end; i += inc) {
        TaskSchedInitSched(&schedulables[i].sched, test_scheduler_events_cb,
          &schedulables[i]);
        schedulables[i].offset_ms = (i + 1) * 20;
        TaskSchedAddTimerFn(TS_PRIO_APP_EVENTS, &schedulables[i].sched, 0,
          schedulables[i].offset_ms);

        // Install or re-install the event.
        schedulables[i].trig =
          TaskSchedAddEventFn(
            TS_PRIO_APP_EVENTS,
            test_scheduler_events_triggered_cb,
            &schedulables[i],
            schedulables[i].trig);
      }

      LOG_TRACE(("Running task scheduler for 1s.\r\n"));
      {
        uint32_t startTime = OSALGetMS();
        uint32_t timeElapsed = OSALGetMS() - startTime;
        while (timeElapsed < 1000) {
          OSALSleep(50);
          // TaskSchedPoll();
          timeElapsed = OSALGetMS() - startTime;
        }
      }

      for (size_t i = 1; i < ARRSZ(schedulables); i++) {
        if (schedulables[i].osalMs < schedulables[i - 1].osalMs) {
          completeFailure = true;
        }
        else if (schedulables[i].osalMs == schedulables[i - 1].osalMs) {
          simultaneousExecs++;
        }

        if (schedulables[i].timeOrTicksMs < schedulables[i - 1].timeOrTicksMs) {
          completeFailure = true;
        }
        else if (schedulables[i].timeOrTicksMs < schedulables[i - 1].timeOrTicksMs) {
          simultaneousExecs++;
        }
      }

      LOG_TRACE(("Cancelling schedulables and running for 500 more milliseconds\r\n"));
      for (size_t i = 0; i < ARRSZ(schedulables); i++) {
        TaskSchedCancelScheduledTask(&schedulables[i].sched);
      }

      {
        uint32_t startTime = OSALGetMS();
        uint32_t timeElapsed = OSALGetMS() - startTime;
        while (timeElapsed < 500) {
          OSALSleep(10);
#ifdef OSAL_SINGLE_TASK
          TaskSchedPollIdle();
#endif
          timeElapsed = OSALGetMS() - startTime;
        }
      }
    }
    retries++;
    if ((!completeFailure) && (simultaneousExecs < 6)) {
      passedTests |= true;
    }
    retries++;
  }
  EXPECT_TRUE(passedTests);
}
#endif // OSAL_SINGLE_TASK

