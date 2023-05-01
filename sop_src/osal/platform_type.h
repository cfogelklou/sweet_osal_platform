#ifndef PLATFORM_TYPE_H__
#define PLATFORM_TYPE_H__

#ifdef __APPLE__
#include "TargetConditionals.h"
#if defined(TARGET_OS_IPHONE) || defined(TARGET_IPHONE_SIMULATOR)
#ifndef TARGET_OS_IOS
#define TARGET_OS_IOS 1
#define TARGET_OS_ANDROID 0
#endif
#elif !TARGET_OS_IPHONE
#ifndef TARGET_OS_OSX
#define TARGET_OS_OSX 1
#define TARGET_OS_ANDROID 0
#endif
#endif
#endif

#if defined(ANDROID)
#define TARGET_OS_ANDROID 1
#endif

#if defined(WIN32) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix) || defined(ANDROID) || defined(__APPLE__)
#define PLATFORM_FULL_OS 1
#define PLATFORM_EMBEDDED 0
#else
#define PLATFORM_EMBEDDED 1
#define PLATFORM_FULL_OS 0
#endif

// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// Check GCC
#if (__GNUC__) || (TARGET_OS_ANDROID)
#define NORETURN __attribute__ ((noreturn))
#if __x86_64__ || __ppc64__ || defined(__LP64__) || defined(_LP64)
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

#ifdef __APPLE__
#undef NORETURN
#define NORETURN 
#include <stdint.h>
#if __WORDSIZE == 32
#define ENVIRONMENT32
#elif __WORDSIZE == 64
#define ENVIRONMENT64
#endif
#endif

#if (defined(ENVIRONMENT32) && defined(ENVIRONMENT64))
#error "Can't have both ENVIRONMENT32 and ENVIRONMENT64 defined"
#elif ((!defined(ENVIRONMENT32) && !defined(ENVIRONMENT64)))
#define ENVIRONMENT32
#endif

#ifndef NORETURN
#define NORETURN
#endif

#endif

