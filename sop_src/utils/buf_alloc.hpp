#ifndef BUF_ALLOC_HPP
#define BUF_ALLOC_HPP

/*
  This is a simple class which takes a buffer pointer and then allocates
  memory (which should never be freed) from that buffer.
  To free memory, do a Reset(), which clears EVERYTHING.
*/

#ifdef __cplusplus

#include <stdint.h>
#include <stddef.h>
// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
class BufAlloc {
public:
  typedef uint64_t ba_t;

private:
  ba_t *const mpWordBufAry;
  const int mBufLenWords;
  int mBufIdx;

public:
  // //////////////////////////////////////////////////////////////////////////
  // Construct a buffer allocator
  explicit BufAlloc(ba_t *const pWordBufAry, int bufLenWords);

  // //////////////////////////////////////////////////////////////////////////
  // Reset a buffer allocator (deallocates all allocated from the buffer.)
  void Reset();

  // //////////////////////////////////////////////////////////////////////////
  // Number of words available to allocate
  size_t AvailWords();

  // //////////////////////////////////////////////////////////////////////////
  // Number of words available to allocate
  size_t AvailBytes();

  // //////////////////////////////////////////////////////////////////////////
  // Gets the current pointer
  void *GetCurPtr();

  // //////////////////////////////////////////////////////////////////////////
  // Do a MALLOC from the buffer (use sizeof())
  void *Malloc(const size_t bytes);

  // //////////////////////////////////////////////////////////////////////////
  // Do a MALLOC from the buffer (use number of 64-bit words.)
  ba_t *MallocWords(const size_t numWords);
};

#endif // #ifdef __cplusplus

#endif
