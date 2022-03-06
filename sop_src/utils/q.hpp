#ifndef Q_TEMPLATE_HPP__
#define Q_TEMPLATE_HPP__

#ifdef __cplusplus
#include "utils/byteq.h"
#include "utils/helper_macros.h"
#include "utils/platform_log.h"

template <typename T> class Q {
public:
  // Constructor
  explicit Q()
    : mByteQ() {
  }

  // Constructor
  explicit Q(
    T* const pBuf, unsigned int nBufSz,
    const bool lockOnWrites = false, bool lockOnReads = false) {
    Init(pBuf, nBufSz, lockOnWrites, lockOnReads);
  }

  void Init(
    T* const pBuf, unsigned int nBufSz,
    const bool lockOnWrites = false, bool lockOnReads = false) {
    ByteQCreate(&mByteQ, (uint8_t*)pBuf, sizeof(T) * nBufSz, lockOnWrites, lockOnReads);
  }

  // Destructor
  virtual ~Q() {
    ByteQDestroy(&mByteQ);
  }

  // Write function, returns amount written.  Typically uses a FIFO.
  int Write(const T* const pBuf, const int num) {
    return ByteQWrite(&mByteQ, (uint8_t*)pBuf, sizeof(T) * num);
  }

  int Write(const T& buf) {
    return Write(&buf, 1);
  }

  int ForceWrite(const T& buf) {
    return ForceWrite(&buf, 1);
  }

  // Get Write Ready.  Returns amount that can be written.
  int GetWriteReady() {
    return (ByteQGetWriteReady(&mByteQ) / sizeof(T));
  }

  // Read function - returns amount available.  Typically uses an internal FIFO.
  int Read(T* const pBytes, const int numT) {
    return (ByteQRead(&mByteQ, (uint8_t*)pBytes, sizeof(T) * numT) / sizeof(T));
  }

  int Peek(T* const pT, const int numT) {
    return (ByteQPeek(&mByteQ, (uint8_t*)pT, sizeof(T) * numT) / sizeof(T));
  }

  int PeekFromEnd(T* const pT, const int numT) {
    return (
      ByteQDoReadFromEnd(&mByteQ, (uint8_t*)pT, sizeof(T) * numT) /
      sizeof(T));
  }

  // Get the number of bytes that can be read.
  int GetReadReady() {
    return (ByteQGetReadReady(&mByteQ) / sizeof(T));
  }

  int ForceWrite(const T* const pWrBuf, const int nLen) {
    return (
      ByteQForceWrite(&mByteQ, (uint8_t*)pWrBuf, sizeof(T) * nLen) /
      sizeof(T));
  }

  // Get a pointer that can be used for advanced functions.
  ByteQ_t* GetByteQPtr() {
    return &mByteQ;
  }

  void Flush() {
    ByteQFlush(&mByteQ);
  }

  int CommitWrite(int nLen) {
    return (ByteQCommitWrite(&mByteQ, sizeof(T) * nLen) / sizeof(T));
  }

  int CommitRead(int nLen) {
    return (ByteQCommitRead(&mByteQ, sizeof(T) * nLen) / sizeof(T));
  }

  // Fetch from 0..size or from writePtr -idx
  T& operator[](const int idx) {
    static T end;
    T* pRVal        = &end;
    auto numEntries = GetReadReady();
    if (idx >= 0) {
      if (idx < numEntries) {
        auto newIdx = mByteQ.nRdIdx;
        newIdx += (idx * sizeof(T));
        newIdx %= mByteQ.nBufSz;
        pRVal = (T*)&mByteQ.pfBuf[ newIdx ];
      } else {
        LOG_ASSERT_HPP(false);
      }
    } else {
      const int aidx = ABS(idx);
      if (aidx <= numEntries) {
        // Allow fetching from writePtr - index
        int newIdx = mByteQ.nWrIdx;
        newIdx -= (aidx * sizeof(T));
        if (newIdx < 0) {
          newIdx += mByteQ.nBufSz;
        }
        pRVal = (T*)&mByteQ.pfBuf[ newIdx ];
      } else {
        LOG_ASSERT_HPP(false);
      }
    }
    return *pRVal;
  }

private:
  ByteQ_t mByteQ;
};

#endif // __cplusplus

#endif // Q_TEMPLATE_HPP__
