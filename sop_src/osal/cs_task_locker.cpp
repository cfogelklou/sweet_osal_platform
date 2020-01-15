#include "cs_task_locker.hpp"
#include "osal/osal.h"

CSTaskLocker::CSTaskLocker() {
  OSALEnterTaskCritical();
}

CSTaskLocker::~CSTaskLocker() {
  OSALExitTaskCritical();
}

