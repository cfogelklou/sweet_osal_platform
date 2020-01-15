/*
  This is a simple class which takes a buffer pointer and then allocates
  memory (which should never be freed) from that buffer.
  To free memory, do a Reset(), which clears EVERYTHING.
*/

#include "buf_alloc.hpp"
#include "utils/platform_log.h"
#include <stddef.h>
LOG_MODNAME("buf_alloc")

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////

// //////////////////////////////////////////////////////////////////////////
// Construct a buffer allocator
BufAlloc::BufAlloc(ba_t *const pWordBufAry, int bufLenWords)
    : mpWordBufAry(pWordBufAry), mBufLenWords(bufLenWords), mBufIdx(0) {
  LOG_ASSERT(mpWordBufAry);
  ASSERT_AT_COMPILE_TIME(sizeof(ba_t) == 8);
  Reset();
}

// //////////////////////////////////////////////////////////////////////////
// Reset a buffer allocator (deallocates all allocated from the buffer.)
void BufAlloc::Reset() { mBufIdx = 0; }

// //////////////////////////////////////////////////////////////////////////
// Number of words available to allocate
size_t BufAlloc::AvailWords() { return (mBufLenWords - mBufIdx); }

// //////////////////////////////////////////////////////////////////////////
// Number of words available to allocate
size_t BufAlloc::AvailBytes() { return AvailWords() * sizeof(ba_t); }

// //////////////////////////////////////////////////////////////////////////
// Gets the current pointer
void *BufAlloc::GetCurPtr() {
  LOG_ASSERT(mpWordBufAry);
  return (void *)&mpWordBufAry[mBufIdx];
}

// //////////////////////////////////////////////////////////////////////////
// Do a MALLOC from the buffer (use sizeof())
void *BufAlloc::Malloc(const size_t bytes) {
  const size_t wordsToAlloc = (bytes + sizeof(ba_t) - 1) / sizeof(ba_t);
  void *rval = MallocWords(wordsToAlloc);
  LOG_ASSERT(rval);
  return rval;
}

// //////////////////////////////////////////////////////////////////////////
// Do a MALLOC from the buffer (use number of 64-bit words.)
BufAlloc::ba_t *BufAlloc::MallocWords(const size_t numWords) {
  ba_t *rval = NULL;
  const size_t availwords = AvailWords();
  if (numWords <= availwords) {
    rval = &mpWordBufAry[mBufIdx];
    mBufIdx += numWords;
  }
  LOG_ASSERT(rval);
  return rval;
}
