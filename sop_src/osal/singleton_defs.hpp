/**
* COPYRIGHT	(c)	Applicaudia 2020
* @file     singleton_defs.hpp
* @brief    
*           Singleton definitions, using static memory
*           to guarantee that constructed objects are 
*           always lazily constructed.
*/
#ifndef SINGLETON_DEFS_HPP__
#define SINGLETON_DEFS_HPP__

#include <stdint.h>
#if defined(__cplusplus)
#include <new>

//##type##

#define SINGLETON_DECLARATION(type) \
static uintptr_t mInstMem[]; \
static type *mInst;

#define SINGLETON_INST_DECLARATION(type) \
static type &inst();

#define SINGLETON_DECLARATIONS(type) \
SINGLETON_DECLARATION(type) \
SINGLETON_INST_DECLARATION(type)

#define SINGLETON_INSTANTIATION(type) \
uintptr_t type::mInstMem[(7+sizeof(type))/sizeof(uintptr_t)]; \
type *type::mInst;

#define SINGLETON_INST_INSTANTIATION(type) \
type &type::inst(){ \
  if (NULL == mInst){ \
    OSALEnterTaskCritical(); \
    if (NULL == mInst){ \
      mInst = new ((char *)mInstMem) type(); \
    } \
    OSALExitTaskCritical(); \
  } \
  return *mInst; \
}

// For use when you are SURE that only one thread will instantiate.
#define SINGLETON_INST_INSTANTIATION_1THREAD(type) \
type &type::inst(){ \
if (NULL == mInst){ \
mInst = new ((char *)mInstMem) type(); \
} \
return *mInst; \
}

// Protects from multiple threads possibly instantiating
#define SINGLETON_INSTANTIATIONS(type) \
SINGLETON_INSTANTIATION(type) \
SINGLETON_INST_INSTANTIATION(type)

// For use when you are SURE that only one thread will instantiate.
#define SINGLETON_INSTANTIATIONS_1THREAD(type) \
SINGLETON_INSTANTIATION(type) \
SINGLETON_INST_INSTANTIATION_1THREAD(type)

#define OVERRIDE_IN_PLACE_NEW_DELETE_NONSINGLETON(type)

#define OVERRIDE_IN_PLACE_NEW_DELETE(type) \
SINGLETON_DECLARATION(type)

#else
#define OVERRIDE_IN_PLACE_NEW_DELETE_NONSINGLETON(type)
#define OVERRIDE_IN_PLACE_NEW_DELETE(type)
#endif

#endif

