#include "phone_al.hpp"
#include "osal/platform_type.h"

#if (!((TARGET_OS_ANDROID) || (TARGET_OS_IOS)))

// For unsupported platforms, return a stub.
PhoneAL &PhoneAL::inst(){
  static PhoneAL theInst;
  return theInst;
}

#endif
