#ifndef WORDQ_HPP__
#define WORDQ_HPP__

#ifdef __cplusplus
#include "utils/byteq.h"
#include "buf_io/buf_io.hpp"

#ifdef ccs
#define 
#endif

template <typename WordT>
class WordQ {
private:
  static constexpr size_t toBytes(const size_t words) {
    return words * sizeof(WordT);
  }

  static constexpr size_t toWords(const size_t bytes) {
    return bytes / sizeof(WordT);
  }

  static constexpr uint8_t* toPtr8(WordT* p) {
    return (uint8_t*)p;
  }

  static constexpr WordT* toPtrW(uint8_t * p) {
    return (WordT*)p;
  }

public:

  // Constructor
  explicit WordQ()
    : mByteQ()
  {
  }

  // Constructor
  explicit WordQ(WordT * const pBuf, const size_t nBufWords,
    const bool lockOnWrites = false, bool lockOnReads = false) 
    : WordQ::WordQ()
  {
    Init( pBuf, nBufSz, lockOnWrites, lockOnReads );
  }

  void Init(WordT * const pBuf, size_t nBufWords,
    const bool lockOnWrites = false, bool lockOnReads = false) {
    ByteQCreate(&mByteQ, (uint8_t *)pBuf, toBytes(nBufWords), lockOnWrites, lockOnReads);
  }

  // Destructor
  virtual ~WordQ(){
    ByteQDestroy( &mByteQ );
  }

  // Write function, returns amount written.  Typically uses a FIFO.
  size_t Write(const WordT *const pWords, const size_t numWords)  {    
    return toWords(ByteQWrite( &mByteQ, toPtr8(pWords), toBytes(numWords)));
  }

  // Get Write Ready.  Returns amount that can be written.
  size_t GetWriteReady() {
    return toWords(ByteQGetWriteReady( &mByteQ ));
  }

  // Read function - returns amount available.  Typically uses an internal FIFO.
  size_t Read(uint8_t *const pWords, const size_t numWords)  {
    return toWords(ByteQRead(&mByteQ, toPtr8(pWords), toBytes(numWords)));
  }

  // Get the number of bytes that can be read.
  size_t GetReadReady()  {
    return toWords(ByteQGetReadReady(&mByteQ));
  }

  size_t ForceWrite(const WordT * const pWrBuf, const size_t numWords) {
    return toWords(ByteQForceWrite( &mByteQ, toPtr8((WordT * )pWrBuf), toBytes(numWords)));
  }

  // Get a pointer that can be used for advanced functions.
  ByteQ_t *GetByteQPtr(){
    return &mByteQ;
  }

  void Flush() {
    ByteQFlush(&mByteQ);
  }

  size_t CommitWrite(const size_t numWords) {
    return toWords(ByteQCommitWrite(&mByteQ, toBytes(numWords)));
  }

  size_t CommitRead(const size_t numWords) {
    return toWords(ByteQCommitRead(&mByteQ, toBytes(numWords)));
  }

private:
  ByteQ_t mByteQ;
};

typedef WordQ<float> FloatQ;
typedef WordQ<double> DoubleQ;
typedef WordQ<uint32_t> UInt32Q;
typedef WordQ<uint16_t> UInt16Q;
typedef WordQ<int32_t> Int32Q;
typedef WordQ<int16_t> Int16Q;

#endif // __cplusplus

#endif
