/**
 * COPYRIGHT    (c)	Applicaudia 2020
 * @file        platform_log.h
 * @brief       Logging/Assertions for Applicaudia PAKM
 */


#ifndef BLE_LOG_H
#define BLE_LOG_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#ifndef PLATFORM_TYPE_H__
#include "osal/platform_type.h"
#endif

typedef void(*LOG_LoggingFn)(
  void *pUserData, 
  const uint32_t ts, 
  const char *szLine, 
  const int len);

typedef enum LOG_UIMessageTag {
  LOG_MSG_NOTIFICATION,
  LOG_MSG_WARNING,
  LOG_MSG_ERROR,
} LOG_UIMessageT;

// Installed by iOS and Android to relay errors to the UI.
typedef void(*LOG_LogUIFn)(
  void * const pUserData,
  const LOG_UIMessageT type,
  const char * const szMessage,
  const int len
  );

#ifdef __cplusplus
extern "C" {
#endif

extern bool log_TraceEnabled;
extern bool log_VerboseEnabled;

// Pass in a different function to use for logging.
void LOG_Init(LOG_LoggingFn logFn, void *pUserData);

void LOG_InitUI(LOG_LogUIFn logUiFn, void *pUserData);

int LOG_VPrintf(const char * format, va_list arg);

// Debug, prints to the platform's logging area.
void LOG_Log( const char *szFormat, ...);

// Prints something to the UI (Android or IOS)
void LOG_LogUI(const LOG_UIMessageT type, const char *szFormat, ...);

// Prints the buffer in pHex as hex characters.
void LOG_Hex( const uint8_t * const pHex, const int hexLen);

// Prints the buffer in pBuf as base64 characters.
void LOG_Base64(const uint8_t *pBuf, const int len);

// Used within BLE code if a runtime test fails.
void LOG_AssertionFailed( const char * szFile, const int line );

// Used within BLE code if a runtime warning fails.
void LOG_AssertionWarningFailed( const char * szFile, const int line );

void LOG_HexArrC(
  const uint8_t * const pHex,
  const int hexLen,
  const char * const pVarName
);

void LOG_LargeStrC(
  const char * const pLargeStr,
  const int largeStrLen,
  const int lineLen,
  const bool insertCrLf
);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
void LOG_HexArr(
  const uint8_t * const pHex,
  const int hexLen,
  const char * const pVarName = NULL
);

void LOG_LargeStr(
  const char * const pLargeStr,
  const int largeStrLen,
  const int lineLen = 80,
  const bool insertCrLf = true
);

#endif

// TRACE code that can be disabled optionally.
// Can be LOG_Log function can have its log support overridden at run time.
#define LOG_TRACE(x) if (log_TraceEnabled) do {LOG_Log x ;} while(0)
#define LOG_ERROR(x) if (log_TraceEnabled) do {LOG_Log x ;} while(0)
#define LOG_WARNING(x) if (log_TraceEnabled) do {LOG_Log x ;} while(0)

// Only verbose logs on PC/desktop.
#if ((PLATFORM_EMBEDDED > 0) || defined(ANDROID) || (TARGET_OS_IPHONE && !DEBUG) || (LOG_VERBOSE_DISABLED))
#define LOG_VERBOSE(x)
#define LOG_VERBOSE_ENABLED 0
#else
#define LOG_VERBOSE(x) if ((log_TraceEnabled)&&(log_VerboseEnabled)) do {LOG_Log x ;} while(0)
#define LOG_VERBOSE_ENABLED 1
#endif

#if (PLATFORM_EMBEDDED > 0)
// Used for debugging - Allows for a shorter string than __FILE__
#define LOG_MODNAME(str) \
    static const char dbgModId[] = (str);
#else
#define LOG_MODNAME(str)
#define dbgModId __FILE__
#endif

//
// Debug stuff that can be disabled optionally.
//
#ifndef LOG_DISABLE_ASSERT
#define LOG_ASSERT(x) do {if (!(x)) {LOG_AssertionFailed(dbgModId, __LINE__);}}while(0)
#if (PLATFORM_EMBEDDED > 0) // #if (PLATFORM_EMBEDDED > 0)
#define LOG_ASSERT_HPP(x) do {if (!(x)) {LOG_AssertionFailed("hpp", __LINE__);}}while(0)
#define LOG_ASSERT_WARN_HPP(x) do {if (!(x)) {LOG_AssertionWarningFailed("hpp", __LINE__);}}while(0)
#else // #if (PLATFORM_EMBEDDED > 0)
#define LOG_ASSERT_HPP(x) LOG_ASSERT_FN(x)
#define LOG_ASSERT_WARN_HPP(x) LOG_ASSERT_WARN_FN(x)
#endif // #if (PLATFORM_EMBEDDED > 0)
#define LOG_ASSERT_FN(x) do {if (!(x)) {LOG_AssertionFailed(dbgModId, __LINE__);}}while(0)
#define LOG_ASSERT_WARN(x) do {if (!(x)) {LOG_AssertionWarningFailed(dbgModId, __LINE__);}}while(0)
#define LOG_ASSERT_WARN_FN(x) do {if (!(x)) {LOG_AssertionWarningFailed(dbgModId, __LINE__);}}while(0)
#define ASSERT_AT_COMPILE_TIME(pred) switch(0){case 0:break;case (pred):break;}
#define ASSERT_POWER_OF_TWO(sz) ASSERT_AT_COMPILE_TIME(!(sz & (sz - 1)))
#else // #ifndef LOG_DISABLE_ASSERT

#define LOG_ASSERT(x) do{}while(0)
#define LOG_ASSERT_HPP(x) do {x;}while(0)
#define LOG_ASSERT_FN(x) do {x;}while(0)
#define LOG_ASSERT_WARN(x) do{}while(0)
#define LOG_ASSERT_WARN_HPP(x) do{}while(0)
#define LOG_ASSERT_WARN_FN(x) do {x;}while(0)
#define ASSERT_AT_COMPILE_TIME(pred) do{}while(0)
#define ASSERT_POWER_OF_TWO(pred)do{}while(0)
#endif // #ifndef LOG_DISABLE_ASSERT

#define LOG_ASSERT_EXIT_CODE 255

#endif /* BLE_LOG_H */
