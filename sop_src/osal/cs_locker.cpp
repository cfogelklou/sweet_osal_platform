#include "cs_locker.hpp"
#include "osal/osal.h"

CSLocker::CSLocker() {
  OSALEnterCritical();
}

CSLocker::~CSLocker() {
  OSALExitCritical();
}

