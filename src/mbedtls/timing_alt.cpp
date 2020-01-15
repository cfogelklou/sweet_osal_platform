#include "mbedtls/myconfig.h"
#include "mbedtls/platform.h"


#if defined(MBEDTLS_TIMING_C)

#include "mbedtls/timing.h"

#if defined(MBEDTLS_TIMING_ALT)

//#include "rtc/rtc.h"
#include "osal/osal.h"
#include "utils/platform_log.h"
#include "task_sched/task_sched.h"
#include "osal/singleton_defs.hpp"


LOG_MODNAME("timing_alt.cpp");

unsigned long mbedtls_timing_hardclock(void){
  return OSALGetMS();
}

volatile int mbedtls_timing_alarmed = 0;

class Alarm {
public:
  SINGLETON_DECLARATIONS(Alarm);

  void setAlarm(int seconds) {
    TaskSchedCancelScheduledTask(&mTask);
    TaskSchedAddTimerFn(TS_PRIO_APP_EVENTS, &mTask, 0, seconds * 1000);
  }

private:

  Alarm() {
    TaskSchedInitSched(&mTask, alarmC, this);
  }

  TaskSchedulable mTask;

  static void alarmC(void *p, uint32_t) {
    (void)p;
    mbedtls_timing_alarmed = 1;
  }

};

SINGLETON_INSTANTIATIONS(Alarm);

extern "C" {

  void mbedtls_set_alarm(int seconds)
  {
    mbedtls_timing_alarmed = 0;
    Alarm::inst().setAlarm(seconds);
  }

}



// Returns diff in milliseconds.
unsigned long mbedtls_timing_get_timer(struct mbedtls_timing_hr_time *val, int reset)
{
  unsigned long delta;
  uint64_t offset;
  uint64_t *t = (uint64_t *)val;

  offset = mbedtls_timing_hardclock();
  const long long nDelta = offset - *t;
  delta = nDelta;

  if (reset) {
    *t = offset;
  }
  else {
    if (nDelta < 0) {
      LOG_ASSERT_WARN(0);
    }
  }

  return(delta);
}

void mbedtls_timing_set_delay(void *data, uint32_t int_ms, uint32_t fin_ms)
{
  mbedtls_timing_delay_context *ctx = (mbedtls_timing_delay_context *)data;

  ctx->int_ms = int_ms;
  ctx->fin_ms = fin_ms;

  if (fin_ms != 0) {
    (void)mbedtls_timing_get_timer(&ctx->timer, 1);
  }
}

/*
* Get number of delays expired
*/
int mbedtls_timing_get_delay(void *data)
{
  mbedtls_timing_delay_context *ctx = (mbedtls_timing_delay_context *)data;
  unsigned long elapsed_ms;

  if (ctx->fin_ms == 0)
    return(-1);

  elapsed_ms = mbedtls_timing_get_timer(&ctx->timer, 0);

  if (elapsed_ms >= ctx->fin_ms)
    return(2);

  if (elapsed_ms >= ctx->int_ms)
    return(1);

  return(0);
}

#endif /* !MBEDTLS_TIMING_ALT */

#if 0 // defined(MBEDTLS_SELF_TEST)

/*
* Busy-waits for the given number of milliseconds.
* Used for testing mbedtls_timing_hardclock.
*/
static void busy_msleep(unsigned long msec)
{
  struct mbedtls_timing_hr_time hires;
  unsigned long i = 0; /* for busy-waiting */
  volatile unsigned long j; /* to prevent optimisation */

  (void)mbedtls_timing_get_timer(&hires, 1);

  while (mbedtls_timing_get_timer(&hires, 0) < msec)
    i++;

  j = i;
  (void)j;
}

#define FAIL    do                      \
{                                       \
    if( verbose != 0 )                  \
        mbedtls_printf( "failed\n" );   \
                                        \
    return( 1 );                        \
} while( 0 )

/*
* Checkup routine
*
* Warning: this is work in progress, some tests may not be reliable enough
* yet! False positives may happen.
*/
int mbedtls_timing_self_test(int verbose)
{
  unsigned long cycles, ratio;
  unsigned long millisecs, secs;
  int hardfail;
  struct mbedtls_timing_hr_time hires;
  uint32_t a, b;
  mbedtls_timing_delay_context ctx;

  if (verbose != 0)
    mbedtls_printf("  TIMING tests note: will take some time!\n");


  if (verbose != 0)
    mbedtls_printf("  TIMING test #1 (set_alarm / get_timer): ");

  for (secs = 1; secs <= 3; secs++)
  {
    (void)mbedtls_timing_get_timer(&hires, 1);

    mbedtls_set_alarm((int)secs);
    while (!mbedtls_timing_alarmed)
      ;

    millisecs = mbedtls_timing_get_timer(&hires, 0);

    /* For some reason on Windows it looks like alarm has an extra delay
    * (maybe related to creating a new thread). Allow some room here. */
    if (millisecs < 800 * secs || millisecs > 1200 * secs + 300)
    {
      if (verbose != 0)
        mbedtls_printf("failed\n");

      return(1);
    }
  }

  if (verbose != 0)
    mbedtls_printf("passed\n");

  if (verbose != 0)
    mbedtls_printf("  TIMING test #2 (set/get_delay        ): ");

  for (a = 200; a <= 400; a += 200)
  {
    for (b = 200; b <= 400; b += 200)
    {
      mbedtls_timing_set_delay(&ctx, a, a + b);

      busy_msleep(a - a / 8);
      if (mbedtls_timing_get_delay(&ctx) != 0)
        FAIL;

      busy_msleep(a / 4);
      if (mbedtls_timing_get_delay(&ctx) != 1)
        FAIL;

      busy_msleep(b - a / 8 - b / 8);
      if (mbedtls_timing_get_delay(&ctx) != 1)
        FAIL;

      busy_msleep(b / 4);
      if (mbedtls_timing_get_delay(&ctx) != 2)
        FAIL;
    }
  }

  mbedtls_timing_set_delay(&ctx, 0, 0);
  busy_msleep(200);
  if (mbedtls_timing_get_delay(&ctx) != -1)
    FAIL;

  if (verbose != 0)
    mbedtls_printf("passed\n");

  if (verbose != 0)
    mbedtls_printf("  TIMING test #3 (hardclock / get_timer): ");

  /*
  * Allow one failure for possible counter wrapping.
  * On a 4Ghz 32-bit machine the cycle counter wraps about once per second;
  * since the whole test is about 10ms, it shouldn't happen twice in a row.
  */
  hardfail = 0;

hard_test:
  if (hardfail > 1)
  {
    if (verbose != 0)
      mbedtls_printf("failed (ignored)\n");

    goto hard_test_done;
  }

  /* Get a reference ratio cycles/ms */
  millisecs = 1;
  cycles = mbedtls_timing_hardclock();
  busy_msleep(millisecs);
  cycles = mbedtls_timing_hardclock() - cycles;
  ratio = cycles / millisecs;

  /* Check that the ratio is mostly constant */
  for (millisecs = 2; millisecs <= 4; millisecs++)
  {
    cycles = mbedtls_timing_hardclock();
    busy_msleep(millisecs);
    cycles = mbedtls_timing_hardclock() - cycles;

    /* Allow variation up to 20% */
    if (cycles / millisecs < ratio - ratio / 5 ||
      cycles / millisecs > ratio + ratio / 5)
    {
      hardfail++;
      goto hard_test;
    }
  }

  if (verbose != 0)
    mbedtls_printf("passed\n");

hard_test_done:

  if (verbose != 0)
    mbedtls_printf("\n");

  return(0);
}

#endif /* MBEDTLS_SELF_TEST */

#endif /* MBEDTLS_TIMING_C */
