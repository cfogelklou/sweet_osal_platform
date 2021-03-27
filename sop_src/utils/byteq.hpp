#ifndef BYTEQ_HPP__
#define BYTEQ_HPP__

#ifdef __cplusplus
#include "buf_io/buf_io.hpp"
#include "utils/byteq.h"

#ifdef ccs
#define override
#endif

class ByteQ : public BufIO {
public:
  // Constructor
  explicit ByteQ()
    : mByteQ() {
  }

  // Constructor
  explicit ByteQ(
    bq_t* const pBuf, unsigned int nBufSz,
    const bool lockOnWrites = false, bool lockOnReads = false)
    : ByteQ::ByteQ() {
    Init(pBuf, nBufSz, lockOnWrites, lockOnReads);
  }

  void Init(
    bq_t* const pBuf, unsigned int nBufSz,
    const bool lockOnWrites = false, bool lockOnReads = false) {
    ByteQCreate(&mByteQ, pBuf, nBufSz, lockOnWrites, lockOnReads);
  }

  // Destructor
  virtual ~ByteQ() {
    ByteQDestroy(&mByteQ);
  }

  // Write function, returns amount written.  Typically uses a FIFO.
  int Write(const uint8_t* const pBytes, const int numBytes) override {
    return ByteQWrite(&mByteQ, pBytes, numBytes);
  }

  // Get Write Ready.  Returns amount that can be written.
  int GetWriteReady() const override {
    ByteQ_t* c = const_cast<ByteQ_t*>(&mByteQ);
    return ByteQGetWriteReady(c);
  }

  // Read function - returns amount available.  Typically uses an internal FIFO.
  int Read(uint8_t* const pBytes, const int numBytes) override {
    return ByteQRead(&mByteQ, pBytes, numBytes);
  }

  // Get the number of bytes that can be read.
  int GetReadReady() const override {
    ByteQ_t* c = const_cast<ByteQ_t*>(&mByteQ);
    return ByteQGetReadReady(c);
  }

  int ForceWrite(const bq_t* const pWrBuf, const int nLen) {
    return ByteQForceWrite(&mByteQ, pWrBuf, nLen);
  }

  // Get a pointer that can be used for advanced functions.
  ByteQ_t* GetByteQPtr() {
    return &mByteQ;
  }

  void Flush() {
    ByteQFlush(&mByteQ);
  }

  int CommitWrite(int nLen) {
    return ByteQCommitWrite(&mByteQ, nLen);
  }

  int CommitRead(int nLen) {
    return ByteQCommitRead(&mByteQ, nLen);
  }

private:
  ByteQ_t mByteQ;
};

#endif // __cplusplus

#endif
