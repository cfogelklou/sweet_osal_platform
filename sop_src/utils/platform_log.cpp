/**
 * COPYRIGHT    (c)	Applicaudia 2020
 * @file        platform_log.cpp
 * @brief       Logging/Assertions for Applicaudia PAKM
 */

#include "platform_log.h"
#include "helper_macros.h"
#include "osal/platform_type.h"
#include "osal/cs_task_locker.hpp"
#include "osal/osal.h"
#include "osal/singleton_defs.hpp"
#include "task_sched/task_sched.h"
#include "utils/convert_utils.h"
#include "utils/simple_string.hpp"
#include "utils/dl_list.hpp"
#include "utils/cnv_utils.hpp"
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define PRINTFBUFSZ 100
#if defined(ccs) && !defined(NO_TI_LIBS)
#include <xdc/runtime/Log.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Text.h>
#include <xdc/runtime/Types.h>
#else
#include "util_vsnprintf.h"
#define System_vsnprintf(pb, sz, fmt, va) util_vsnprintf((pb), (sz), (fmt), (va))
#endif

#include "utils/byteq.hpp"
#include "utils/sl_list.hpp"

#if (PLATFORM_FULL_OS > 0)
#include <cstdio>
#endif

#ifdef WIN32
#include <Windows.h>
#endif

LOG_MODNAME("platform_log")

class Logger;

#if !defined(OSAL_SINGLE_TASK) && (PLATFORM_EMBEDDED > 0)
#define USE_BACKGROUND_PROCESS_FOR_PRINTING 1
#else
#define USE_BACKGROUND_PROCESS_FOR_PRINTING 0
#endif


#if (USE_BACKGROUND_PROCESS_FOR_PRINTING > 0)

extern "C" {
typedef struct BleBufHdrTag {
  TaskSchedulable sched;
  uint8_t payloadLen;
  char *pPayload;
  uint32_t ts;
} BleBufHdrT;
}

#define LOG_BUFEVENTSIZE(payloadLen) (sizeof(BleBufHdrT) + (payloadLen))
#endif

//#define LOG_PRIO TS_PRIO_BACKGROUND

class Logger {
private:

  
public:

  SINGLETON_DECLARATIONS(Logger);
  ~Logger() {}
  void Init(LOG_LoggingFn logFn, void *const pUserData);
  void InitUI(LOG_LogUIFn logFn, void *const pUserData);
  void AssertionFailed(const char *szFile, const int line);
  void AssertionWarningFailed(const char *szFile, const int line);
#if (USE_BACKGROUND_PROCESS_FOR_PRINTING > 0)
  void ProcessBuf(BleBufHdrT *const pBuf);
  BleBufHdrT *AddBuf(const uint32_t ts, const char *const pBuf, const int len);
#endif

private:

#if (USE_BACKGROUND_PROCESS_FOR_PRINTING > 0)
  const static int NUM_FREE_EVENTS = 10;

  void TimerCb(BleBufHdrT * const pBuf);
#endif

  Logger();

public:

  LOG_LoggingFn mLogFn;        // = blelog_defaultLogFn;
  void *mLogDataPtr;           // = NULL;

  LOG_LogUIFn mLogUIFn;        // = blelog_defaultLogFn;
  void *mLogUIDataPtr;           // = NULL;
private:

  bool mLogAssertionHasFailed; // = false;
#if (USE_BACKGROUND_PROCESS_FOR_PRINTING > 0)
  ByteQ mByteQ;

  BleBufHdrT mFreeEventsAry[NUM_FREE_EVENTS];
  bq_t mByteAry[NUM_FREE_EVENTS * 80];
  dll::list mFreeEventsList;
#endif

};

SINGLETON_INSTANTIATIONS(Logger);


#define VALID_TAG 0xa55ea7fa

typedef struct AssertionRecordTag {
  uint32_t validTag;
  const char *pFile;
  int  line;
} AssertionRecord;

AssertionRecord assertFail = {VALID_TAG,NULL,-1};
AssertionRecord assertWarn = {VALID_TAG,NULL,-1};

// ////////////////////////////////////////////////////////////////////////////////////////////////
static void blelog_defaultLogFn(void *pUserData,
                                const uint32_t ts,
                                const char *szLine,
                                const int len) {
  (void)pUserData;
  (void)ts;
  (void)szLine;
  (void)len;
#if (PLATFORM_FULL_OS > 0)
  printf("%s", szLine);
#endif
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
static void blelog_defaultLogUIFn(
  void * const pUserData,
  const LOG_UIMessageT type,
  const char * const szMessage,
  const int len
) {
  // If we get here, then there is no UI installed, so send notifications
  // to the terminal.  Errors and warnings are already sent to the terminal.
  (void)pUserData;
  (void)len;
  if (type == LOG_MSG_NOTIFICATION) {
    LOG_Log("%s\r\n", szMessage);
  }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
Logger::Logger()
  : mLogFn(blelog_defaultLogFn)
  , mLogDataPtr(NULL)
  , mLogUIFn(blelog_defaultLogUIFn)
  , mLogUIDataPtr(NULL)
  , mLogAssertionHasFailed(false)
#if (USE_BACKGROUND_PROCESS_FOR_PRINTING > 0)
  , mByteQ(mByteAry, ARRSZ(mByteAry))
  , mFreeEventsAry()
  , mByteAry()
  , mFreeEventsList()
#endif
{
  assertWarn = assertFail = {0,0,-1};
#if (USE_BACKGROUND_PROCESS_FOR_PRINTING > 0)
  memset(mFreeEventsAry, 0, sizeof(mFreeEventsAry));
  memset(mByteAry, 0, sizeof(mByteAry));

  auto TimerCbC = [](void *pCallbackData, uint32_t) {
      BleBufHdrT * const pBuf = (BleBufHdrT *)pCallbackData;
      Logger::inst().TimerCb(pBuf);
    };

  for (int i = 0; i < NUM_FREE_EVENTS; i++) {
    BleBufHdrT *const pEvt = &mFreeEventsAry[i];
    TaskSchedInitSched(&pEvt->sched, TimerCbC, pEvt);
    mFreeEventsList.push_back(&pEvt->sched.listNode);
  }
#endif
}

#if (USE_BACKGROUND_PROCESS_FOR_PRINTING > 0)
// ////////////////////////////////////////////////////////////////////////////////////////////////
void Logger::TimerCb( BleBufHdrT * const pBuf) {
  
  if (mLogFn){
    mLogFn(mLogDataPtr, pBuf->ts, pBuf->pPayload, pBuf->payloadLen);
  }
  {
    CSTaskLocker lock;
    mFreeEventsList.push_back(&pBuf->sched.listNode);
  }
}
#endif

// ////////////////////////////////////////////////////////////////////////////////////////////////
void Logger::Init(LOG_LoggingFn logFn, void *pUserData) {
  mLogFn = (logFn) ? logFn : blelog_defaultLogFn;
  mLogDataPtr = pUserData;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
void Logger::InitUI(LOG_LogUIFn logFn, void *pUserData) {
  mLogUIFn = (logFn) ? logFn : blelog_defaultLogUIFn;
  mLogUIDataPtr = pUserData;
}

#if (USE_BACKGROUND_PROCESS_FOR_PRINTING > 0)
// ////////////////////////////////////////////////////////////////////////////////////////////////
void Logger::ProcessBuf(BleBufHdrT *const pbBuf) {
  if (pbBuf) {
#if 0 //ndef OSAL_SINGLE_TASK
    if (!mLogAssertionHasFailed) {
      TaskSchedAddTimerFn(LOG_PRIO, &pbBuf->u.sched, 0, 0);
    } else {
      TimerCb(pbBuf);
    }
#else
    TimerCb(pbBuf);
#endif
  }
}


// ////////////////////////////////////////////////////////////////////////////////////////////////
// Add the contents in pChars to the circular buffer.
// Note, must be called form within a critical section.
BleBufHdrT *Logger::AddBuf(const uint32_t ts, const char *const pChars, const int len) {
  BleBufHdrT *pEvt = NULL;
  if (len <= ((int)sizeof(mByteAry) / 2)) {
    pEvt = (BleBufHdrT *)mFreeEventsList.pop_front();
    if (pEvt) {
      ByteQ_t *const pq = mByteQ.GetByteQPtr();

      // If there isn't enough contiguous memory for the buffer, reset to 0.
      const int contig = pq->nBufSz - pq->nWrIdx;
      if (contig < len) {
        pq->nWrIdx = 0;
      }

      // Point at the new location in the buffer.
      pEvt->ts = ts;
      pEvt->pPayload = (char *)&pq->pfBuf[pq->nWrIdx];
      pEvt->payloadLen = len;

      // Copy over the data
      memcpy(pEvt->pPayload, pChars, len);

      // Increment the wr index.
      pq->nWrIdx += len;
    }
  }
  return pEvt;
}

#endif

// ////////////////////////////////////////////////////////////////////////////////////////////////
void Logger::AssertionFailed(const char *szFile, const int line) {
  mLogAssertionHasFailed = true;
  LOG_Log("ASSERT: %s(%d)\r\n", szFile, line);
  LOG_LogUI(LOG_MSG_ERROR, "ASSERT: %s(%d)\r\n", szFile, line);

}

// ////////////////////////////////////////////////////////////////////////////////////////////////
void Logger::AssertionWarningFailed(const char *szFile, const int line) {
  LOG_Log("ASSERT WARN: %s(%d)\r\n", szFile, line);
  LOG_LogUI(LOG_MSG_WARNING, "ASSERT WARN: %s(%d)\r\n", szFile, line);
}

extern "C" {

bool log_TraceEnabled = true;
#ifdef __EMBEDDED_MCU_BE__
bool log_VerboseEnabled = false;
#else
bool log_VerboseEnabled = true;
#endif

// ////////////////////////////////////////////////////////////////////////////////////////////////
void LOG_Init(LOG_LoggingFn logFn, void *pUserData) {
  auto &inst = Logger::inst();
  inst.Init(logFn, pUserData);
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
void LOG_InitUI(LOG_LogUIFn logUiFn, void *pUserData) {
  auto &inst = Logger::inst();
  inst.InitUI(logUiFn, pUserData);
}

#define WBUF_LEN 800
static char log_WorkingBuf[WBUF_LEN + 1];

// ////////////////////////////////////////////////////////////////////////////////////////////////
int LOG_VPrintf(const char * szFormat, va_list va) {
  int printed = 0;
  Logger &inst = Logger::inst();

#if (USE_BACKGROUND_PROCESS_FOR_PRINTING > 0)
  BleBufHdrT *pbBuf = NULL;
#endif
  {
    CSTaskLocker lock;
    log_WorkingBuf[WBUF_LEN] = 'a';
    {
      printed = System_vsnprintf(log_WorkingBuf, WBUF_LEN - 2, szFormat, va);
      printed = MIN(printed, WBUF_LEN);
      log_WorkingBuf[printed] = 0;

#if (USE_BACKGROUND_PROCESS_FOR_PRINTING > 0)      
      pbBuf = inst.AddBuf(OSALGetMS(), log_WorkingBuf, printed + 1);
#else 
      if (inst.mLogFn) {
        inst.mLogFn(inst.mLogDataPtr, OSALGetMS(), log_WorkingBuf, printed);
      }
#endif
    }
    LOG_ASSERT(log_WorkingBuf[WBUF_LEN] == 'a');
  }

#if (USE_BACKGROUND_PROCESS_FOR_PRINTING > 0)      
  if (pbBuf) {
    inst.ProcessBuf(pbBuf);
  }
#endif
  return printed;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
void LOG_Log(const char *szFormat, ...) {
  va_list va;
  va_start(va, szFormat);
  LOG_VPrintf(szFormat, va);
  va_end(va);
};


// ////////////////////////////////////////////////////////////////////////////////////////////////
void LOG_LogUI(const LOG_UIMessageT type, const char *szFormat, ...) {
  Logger &inst = Logger::inst();

  {
    CSTaskLocker lock;
    log_WorkingBuf[WBUF_LEN] = 'a';
    {
      va_list va;
      va_start(va, szFormat);
      int printed = System_vsnprintf(log_WorkingBuf, WBUF_LEN - 2, szFormat, va);
      va_end(va);
      printed = MIN(printed, WBUF_LEN);
      log_WorkingBuf[printed] = 0;
    }
    LOG_ASSERT(log_WorkingBuf[WBUF_LEN] == 'a');
    inst.mLogUIFn(inst.mLogUIDataPtr, type, log_WorkingBuf, (int)strlen(log_WorkingBuf));
  }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Prints the buffer in pHex as hex characters.
void LOG_Hex(const uint8_t *const pHex, const int hexLen) {
  sstring hex;
  CNV_BinToHexStr(pHex, hexLen, hex);
  const char *p = hex.c_str();
  int iter = 0;
  sstring dbg;
  while (iter < hex.length()) {
    int remain = hex.length() - iter;
    int outSize = MIN(remain, 70);
    dbg.assign((uint8_t *)&p[iter], outSize);
    dbg.push_back(0);
    LOG_TRACE(("  %s\r\n", dbg.c_str()));
    iter += outSize;
  }
}

// ////////////////////////////////////////////////////////////////////////////
void LOG_Base64(
  const uint8_t *pBuf,
  const int len)
{
  sstring base64;
  CNV_Base64EncBin(pBuf, len, base64);
  const char *pB64 = base64.c_str();
  int remaining = base64.length();
  int idx = 0;
  char bytesArr[70 + 1];
  while (remaining > 0) {
    int bytes = ARRSZ(bytesArr) - 1;
    bytes = MIN(bytes, remaining);
    memcpy(bytesArr, &pB64[idx], bytes);
    bytesArr[bytes] = 0;
    LOG_TRACE(("%s\r\n", bytesArr));
    remaining -= bytes;
    idx += bytes;
  }
}

} // extern "C" {

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Prints the buffer in pHex as a variable.
void LOG_HexArr(
  const uint8_t * const pHex,
  const int hexLen,
  const char * const pVarName
) {
  int remain = hexLen;
  int written = 0;

  if (pVarName) {
    LOG_Log("uint8_t %s[%u] = {\r\n", pVarName, hexLen);
  }
  else {
    LOG_Log("uint8_t hex[%u] = {\r\n", hexLen);
  }

  sstring tmp;
  while (remain > 0) {
    const int bytes = MIN(8, remain);

    CNV_BinToHexDelimStr(&pHex[written], bytes, "0x", tmp, ",");
    remain -= bytes;
    written += bytes;
    if (remain > 0) {
      tmp.push_char(',');
    }
    LOG_Log("\t%s\r\n", tmp.c_str());
  }
  LOG_Log("};\r\n");
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// Splits a string into multiple lines
void LOG_LargeStr(
  const char * const pLargeStr,
  const int largeStrLen,
  const int lineLen,
  const bool insertCrLf
) {
  if (!pLargeStr) return;
  int remaining = largeStrLen;
  int idx = 0;
  sstring tmp;
  char * pOutBuf = tmp.c8DataPtr(lineLen + 2 + 1);
  LOG_ASSERT(pOutBuf);
  while (remaining > 0) {
    int numChars = MIN(lineLen, remaining);
    memcpy(pOutBuf, &pLargeStr[idx], numChars);
    if (insertCrLf) {
      pOutBuf[numChars++] = '\r';
      pOutBuf[numChars++] = '\n';
    }
    pOutBuf[numChars++] = 0;
    LOG_Log("%s", pOutBuf);
    idx += numChars;
    remaining -= numChars;
  }
}

extern "C" {
// ////////////////////////////////////////////////////////////////////////////////////////////////
void LOG_HexArrC(
    const uint8_t * const pHex,
    const int hexLen,
    const char * const pVarName
)
{
  LOG_HexArr(pHex, hexLen, pVarName);
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
void LOG_LargeStrC(
  const char * const pLargeStr,
  const int largeStrLen,
  const int lineLen,
  const bool insertCrLf
) {
  LOG_LargeStr(pLargeStr, largeStrLen, lineLen, insertCrLf);
}
}

#ifdef __EMBEDDED_MCU_BE__
#include "platform/drivers/st_spc5/st_uart.h"
#endif

extern "C" {
  static volatile bool ignoreAssert = false;
// ////////////////////////////////////////////////////////////////////////////////////////////////
void LOG_AssertionFailed(const char *szFile, const int line) {
  if ((!ignoreAssert) && (!assertFail.pFile)) {
    assertFail.pFile = szFile;
    assertFail.line = line;
#ifdef EVTLOGPINS
    uint8_t tmp[1] = {'F'};
    EvtLogPinsTraceBytes((const uint8_t *)tmp, 1);
    EvtLogPinsTraceBytes((const uint8_t *)szFile, strlen(szFile));
    EvtLogPinsTraceBytes((const uint8_t *)&line, sizeof(line));
#endif

#ifdef __EMBEDDED_MCU_BE__
    if (log_TraceEnabled){
      OSALEnterCritical();
      STUartAssertOut("\r\n\r\nASSERT:");
      STUartAssertOut(szFile);
      log_WorkingBuf[0] = ':';
      util_itoa(line, &log_WorkingBuf[1], 7);
      STUartAssertOut(log_WorkingBuf);
      log_WorkingBuf[0] = '\r';
      log_WorkingBuf[1] = '\n';
      log_WorkingBuf[2] = 0;
      STUartAssertOut(log_WorkingBuf);
      OSALExitCritical();
    }
#else
    Logger::inst().AssertionFailed(szFile, line);
#endif

#if (PLATFORM_EMBEDDED > 0)
    // In an embedded system, we cannot simply "exit" the program.
#ifdef __EMBEDDED_MCU_BE__
    OSALEnterTaskCritical();
    pal_lld_writepad(PORT_E, PIN_MRDY, 1);
    pal_lld_writepad(PORT_E, PIN_RST_BLE, 1); // A "high" pulls the line low via NPN.

    volatile unsigned int i = 0;
    while (i != 10000000) {
      i++;
    }

    OSALSoftReset();
    OSALExitTaskCritical();
#endif
    while (!ignoreAssert) {
      OSALSleep(1);
    }
#else
    printf("ASSERT: %s(%d)\r\n", szFile, line);
#if (!defined( ANDROID ) && !defined(APPLE) && !defined(__APPLE__))
    exit(LOG_ASSERT_EXIT_CODE);
#endif
#endif
 
  }
  else {
#ifdef __EMBEDDED_MCU_BE__
    while (!ignoreAssert) {
      OSALSleep(1);
    }
#else
    if (!ignoreAssert) {
      exit(LOG_ASSERT_EXIT_CODE);
    }
#endif
  }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
void LOG_AssertionWarningFailed(const char *szFile, const int line) {
  if (!assertWarn.pFile) {
    assertWarn.pFile = szFile;
    assertWarn.line = line;
  }
#ifdef EVTLOGPINS
  uint8_t tmp[1] = {'W'};
  EvtLogPinsTraceBytes((const uint8_t *)tmp, 1);
  EvtLogPinsTraceBytes((const uint8_t *)szFile, strlen(szFile));
  EvtLogPinsTraceBytes((const uint8_t *)&line, sizeof(line));
#endif
  Logger::inst().AssertionWarningFailed(szFile, line);
}

} // extern "C" {
