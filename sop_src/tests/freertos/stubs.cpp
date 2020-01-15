#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "platform/task_sched.h"

extern "C" {


void vApplicationIdleHook( void )
{
  const unsigned long ulMSToSleep = 5;

  /* Sleep to reduce CPU load, but don't sleep indefinitely in case there are
  tasks waiting to be terminated by the idle task. */
  Sleep( ulMSToSleep );
  TaskSchedPollIdle();
}

} // extern "C"

