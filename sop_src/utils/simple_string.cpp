/**
 * COPYRIGHT    (c)	Applicaudia 2020
 * @file        simple_string.cpp
 * @brief       A simple string class for building up PaK
 * messages without too many lines of code.  Uses mempools
 * instead of heap, so grows by powers of 2 to match standard
 * pool sizes.
 */
#include "simple_string.hpp"

#include "osal/osal.h"
#include "utils/helper_macros.h"
#include "utils/platform_log.h"

#include <string.h>

LOG_MODNAME("sstring.cpp")


// //////////////////////////////////////////////////////////////////////////
void sstring::init() {
  mIsHeap = false;
  memset(mBufInitial, 0, sizeof(mBufInitial));
  mCurSize = 0;
  mMaxSize = SS_INITIAL_STRING_SIZE;
  mpBuf    = mBufInitial;
}

// //////////////////////////////////////////////////////////////////////////
sstring::sstring(const sstring& rhs)
  : sstring() {
  assign(rhs.mpBuf, rhs.mCurSize);
}

// //////////////////////////////////////////////////////////////////////////
sstring::sstring()
  : mIsHeap(false)
  , mpBuf(mBufInitial)
  , mMaxSize(SS_INITIAL_STRING_SIZE)
  , mCurSize(0)
  , mBufInitial{ 0 }

{
}

// //////////////////////////////////////////////////////////////////////////
sstring::sstring(uint8_t* const pBuf, const ssize_t len)
  : sstring() {
  LOG_ASSERT(pBuf);
  if (pBuf) {
    assign(pBuf, len);
  }
}

// //////////////////////////////////////////////////////////////////////////
sstring::sstring(const uint8_t* const pBuf, const ssize_t len)
  : sstring() {
  LOG_ASSERT(pBuf);
  if (pBuf) {
    assign(pBuf, len);
  }
}

// //////////////////////////////////////////////////////////////////////////
sstring::sstring(const char* const pBuf, const ssize_t len)
  : sstring() {
  LOG_ASSERT(pBuf);
  if (pBuf) {
    assign(pBuf, len);
  }
}

// //////////////////////////////////////////////////////////////////////////
sstring::sstring(const ssize_t size)
  : sstring() {
  set_len(size);
  LOG_ASSERT(mCurSize == size);
}


// //////////////////////////////////////////////////////////////////////////
sstring::~sstring() {
  dtor();
}

// //////////////////////////////////////////////////////////////////////////
void sstring::dtor() {
  clear();
  if (mIsHeap) {
    OSALFREE(mpBuf);
  }
  init();
}

// //////////////////////////////////////////////////////////////////////////
void sstring::assign(const char* const pBuf, const ssize_t slen) {
  LOG_ASSERT(pBuf);
  int32_t len = (int32_t)(uint32_t)slen;
  if (len < 0) {
    len = (int32_t)strlen(pBuf);
    len = MIN(2000, len);
  }
  // Make space for null termination
  if (mMaxSize < (len + 1)) {
    grow(len + 1);
  }
  if (pBuf) {
    assign((uint8_t*)pBuf, len);
    // Add null termination
    mpBuf[ len ] = 0;
  }
}

// //////////////////////////////////////////////////////////////////////////
void sstring::assign(const uint8_t* const pBuf, const ssize_t len, const bool placeholder) {
  clear();
  if (mMaxSize < len) {
    grow(len);
  }
  LOG_ASSERT((mpBuf) && (mMaxSize >= len));
  if (mMaxSize >= len) {
    if (mpBuf != pBuf) {
      memcpy(mpBuf, pBuf, len);
    }
    mCurSize = len;
  }
}

// //////////////////////////////////////////////////////////////////////////
void sstring::assign_static(uint8_t* const pBuf, const ssize_t len) {
  dtor();
  LOG_ASSERT(mpBuf == mBufInitial);
  LOG_ASSERT(pBuf != NULL);
  LOG_ASSERT_WARN(len > 0);
  if (pBuf) {
    mpBuf    = pBuf;
    mCurSize = mMaxSize = len;
  }
}

// //////////////////////////////////////////////////////////////////////////
sstring& sstring::operator=(const char* cstr) {
  LOG_ASSERT(cstr);
  if (cstr) {
    const size_t len = strlen(cstr);
    assign((uint8_t*)cstr, len);
  }
  return *this;
}

// //////////////////////////////////////////////////////////////////////////
sstring& sstring::operator=(const sstring& rhs) {
  assign(rhs.mpBuf, rhs.mCurSize);
  return *this;
}

// //////////////////////////////////////////////////////////////////////////
char sstring::operator[](const int idx) {
  if ((idx < 0) || (idx >= mCurSize)) {
    return 0;
  } else {
    return mpBuf[ idx ];
  }
}

// //////////////////////////////////////////////////////////////////////////
void sstring::setChar(const int idx, const char c) {
  if (idx >= mMaxSize) {
    grow(idx + 1);
  }
  if (idx < mMaxSize) {
    mpBuf[ idx ] = c;
    if (mCurSize <= idx) {
      mCurSize = idx + 1;
    }
  }
}

// //////////////////////////////////////////////////////////////////////////
void sstring::appendu(const uint8_t* const pC, const ssize_t len) {
  LOG_ASSERT(pC);
  if (pC) {
    const ssize_t newSize = length() + len;
    grow(newSize);
    LOG_ASSERT(mMaxSize >= newSize);
    if (mMaxSize >= newSize) {
      memcpy(&mpBuf[ mCurSize ], pC, len);
      commit_append(len);
    }
  }
}

// //////////////////////////////////////////////////////////////////////////
void sstring::appendc(const char* const pC, const ssize_t len) {
  appendu((uint8_t*)pC, len);
}


// //////////////////////////////////////////////////////////////////////////
bool sstring::operator==(const sstring& rhs) const {
  bool equals = (rhs.mCurSize == mCurSize);
  equals &= (0 == memcmp(rhs.mpBuf, mpBuf, mCurSize));
  return equals;
}

// //////////////////////////////////////////////////////////////////////////
bool sstring::operator==(const char* rhs) const {
  bool equals = (rhs != nullptr);
  if (equals) {
    equals &= (0 == memcmp(rhs, mpBuf, mCurSize));
  }
  return equals;
}

// //////////////////////////////////////////////////////////////////////////
bool sstring::operator!=(const sstring& rhs) const {
  return !(sstring::operator==(rhs));
}

// //////////////////////////////////////////////////////////////////////////
// Sets the length of the buffer.
// Assumes that the buffer returned in data() was written externally.
void sstring::set_len(const ssize_t len) {
  if (mMaxSize < len) {
    grow(len);
  }
  mCurSize = MIN(len, mMaxSize);
  LOG_ASSERT(mCurSize <= mMaxSize);
}

// //////////////////////////////////////////////////////////////////////////
// Commits some new length to the end of the buffer.
// Assumes that the buffer returned in data() was written externally.
void sstring::commit_append(const ssize_t len) {
  set_len(mCurSize + len);
  LOG_ASSERT(mCurSize <= mMaxSize);
}

// //////////////////////////////////////////////////////////////////////////
void sstring::push_back(const uint8_t c) {
  appendu(&c, 1);
}

// //////////////////////////////////////////////////////////////////////////
void sstring::push_char(const char c) {
  trim_null();
  grow(mCurSize + 2);
  if (mpBuf) {
    mpBuf[ mCurSize++ ] = c;
    mpBuf[ mCurSize ]   = 0;
  }
}


// //////////////////////////////////////////////////////////////////////////
void sstring::append_str(const sstring& rhs) {
  appendu(rhs.mpBuf, rhs.mCurSize);
}

// //////////////////////////////////////////////////////////////////////////
// Character append.
void sstring::appendp(const char* rhs, const bool nullTerminate) {
  trim_null();
  if (rhs) {
    const size_t len = strlen(rhs);
    if (len > 0) {
      appendu((uint8_t*)rhs, len);
    }
  }
  if (nullTerminate) {
    push_char(0);
  }
}

// //////////////////////////////////////////////////////////////////////////
void sstring::operator+=(const sstring& rhs) {
  append_str(rhs);
}

// //////////////////////////////////////////////////////////////////////////
void sstring::operator+=(const char* rhs) {
  appendp(rhs, true);
}

// //////////////////////////////////////////////////////////////////////////
uint8_t* sstring::u8DataPtr(const ssize_t amtToGrow, const ssize_t setLenTo) {
  if (mMaxSize < amtToGrow) {
    grow(amtToGrow);
  }
  if ((int32_t)setLenTo >= 0) {
    set_len(setLenTo);
  }
  return mpBuf;
}

// //////////////////////////////////////////////////////////////////////////
// Returns a writable buffer
char* sstring::c8DataPtr(const ssize_t amtToGrow, const ssize_t setLenTo) {
  return (char*)u8DataPtr(amtToGrow, setLenTo);
}

// //////////////////////////////////////////////////////////////////////////
uint16_t* sstring::u16DataPtr(const ssize_t amtToGrow, const ssize_t setLenTo) {
  const ssize_t slt = (setLenTo == (ssize_t)-1)
                        ? (ssize_t)-1
                        : setLenTo * sizeof(uint16_t);
  return (uint16_t*)u8DataPtr(amtToGrow * sizeof(uint16_t), slt);
}

// //////////////////////////////////////////////////////////////////////////
uint32_t* sstring::u32DataPtr(const ssize_t amtToGrow, const ssize_t setLenTo) {
  const ssize_t slt = (setLenTo == (ssize_t)-1)
                        ? (ssize_t)-1
                        : setLenTo * sizeof(uint32_t);
  return (uint32_t*)u8DataPtr(amtToGrow * sizeof(uint32_t), slt);
}

// //////////////////////////////////////////////////////////////////////////
int16_t* sstring::s16DataPtr(const ssize_t amtToGrow, const ssize_t setLenTo) {
  const ssize_t slt = (setLenTo == (ssize_t)-1)
                        ? (ssize_t)-1
                        : setLenTo * sizeof(int16_t);
  return (int16_t*)u8DataPtr(amtToGrow * sizeof(int16_t), slt);
}

// //////////////////////////////////////////////////////////////////////////
int32_t* sstring::s32DataPtr(const ssize_t amtToGrow, const ssize_t setLenTo) {
  const ssize_t slt = (setLenTo == (ssize_t)-1)
                        ? (ssize_t)-1
                        : setLenTo * sizeof(int32_t);
  return (int32_t*)u8DataPtr(amtToGrow * sizeof(int32_t), slt);
}

// //////////////////////////////////////////////////////////////////////////
float* sstring::floatDataPtr(const ssize_t amtToGrow, const ssize_t setLenTo) {
  const ssize_t slt = (setLenTo == (ssize_t)-1)
                        ? (ssize_t)-1
                        : setLenTo * sizeof(float);
  return (float*)u8DataPtr(amtToGrow * sizeof(float), slt);
}

// //////////////////////////////////////////////////////////////////////////
double* sstring::doubleDataPtr(const ssize_t amtToGrow, const ssize_t setLenTo) {
  const ssize_t slt = (setLenTo == (ssize_t)-1)
                        ? (ssize_t)-1
                        : setLenTo * sizeof(double);
  return (double*)u8DataPtr(amtToGrow * sizeof(double), slt);
}

// //////////////////////////////////////////////////////////////////////////
void sstring::clear() {
  if ((!mIsHeap) && (mpBuf != mBufInitial)) {
    mpBuf    = mBufInitial;
    mMaxSize = ARRSZ(mBufInitial);
    mCurSize = MIN(mMaxSize, mCurSize);
  }
  memset(mpBuf, 0, mCurSize);
  mCurSize = 0;
}

// //////////////////////////////////////////////////////////////////////////
void sstring::trim_null() {
  trim_char(0);
}

// //////////////////////////////////////////////////////////////////////////
void sstring::trim_char(const char c) {
  while ((mCurSize > 0) && (c == mpBuf[ mCurSize - 1 ])) {
    mpBuf[ mCurSize - 1 ] = 0;
    mCurSize--;
  }
}

// //////////////////////////////////////////////////////////////////////////
int sstring::index_of(const char c) const {
  int rval = -1;
  int idx  = 0;
  while ((rval < 0) && (idx < mCurSize)) {
    if (c == mpBuf[ idx ]) {
      rval = idx;
    }
    ++idx;
  }

  return rval;
}

// //////////////////////////////////////////////////////////////////////////
sstring::ssize_t sstring::nextPowerOfTwo(ssize_t v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

// //////////////////////////////////////////////////////////////////////////
bool sstring::isPowerOfTwo(const ssize_t nX) {
  return ((nX & -nX) == nX);
}

// //////////////////////////////////////////////////////////////////////////
// Grows the internal buffer to at incomingSize or greater.
void sstring::grow(const ssize_t incomingSize) {
  // We always ensure room for NULL terminator.
  const int allocSize = (int)(incomingSize + 1);

  // Only grow if we need to.
  if (allocSize > mMaxSize) {
    // Mempools are allocated in powers of 2.
    const ssize_t maxSize = nextPowerOfTwo(allocSize);
    LOG_ASSERT(isPowerOfTwo(maxSize));
    LOG_ASSERT((maxSize >= allocSize) && (maxSize > mMaxSize));

    // We need to free the old buffer after allocation.
    uint8_t* const pOldBuf = mpBuf;
    const bool wasHeap     = mIsHeap;
    mpBuf                  = (uint8_t*)OSALMALLOC(maxSize);
    LOG_ASSERT(mpBuf);
    if (mpBuf) {
      mIsHeap = true;
      memset(mpBuf, 0, maxSize);
      mMaxSize = maxSize;
      LOG_ASSERT(pOldBuf);
      if (mCurSize > 0) {
        LOG_ASSERT(mCurSize <= mMaxSize);
        memcpy(mpBuf, pOldBuf, mCurSize);
      }
    }
    if (wasHeap) {
      OSALFREE(pOldBuf);
    }
  }
}
