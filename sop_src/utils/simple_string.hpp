#ifndef SIMPLE_STRING_H__
#define SIMPLE_STRING_H__
/**
* COPYRIGHT    (c)	Applicaudia 2020
* @file        simple_string.hpp
* @brief       A simple string class for building up PaK messages without too many lines
*              of code.  Uses mempools instead of heap, so grows by powers of 2 to match
*              standard pool sizes.
*/

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>

class sstring {
private:
  static const int SS_INITIAL_STRING_SIZE = 32+1;

  // //////////////////////////////////////////////////////////////////////////
  void init();

public:
  static const int SS_NOALLOC_STRING_SIZE = (SS_INITIAL_STRING_SIZE-1);

  typedef size_t ssize_t;

  // //////////////////////////////////////////////////////////////////////////
  sstring(const sstring &rhs);

  // //////////////////////////////////////////////////////////////////////////
  sstring();

  // //////////////////////////////////////////////////////////////////////////
  sstring(uint8_t *const pBuf, const ssize_t len);

  // //////////////////////////////////////////////////////////////////////////
  sstring(const uint8_t *const pBuf, const ssize_t len);

  // //////////////////////////////////////////////////////////////////////////
  sstring(const char *const pBuf, const ssize_t len = (uint32_t)-1);

  // //////////////////////////////////////////////////////////////////////////
  sstring(const ssize_t size);

  // //////////////////////////////////////////////////////////////////////////
  ~sstring();

  // //////////////////////////////////////////////////////////////////////////
  void dtor();

  // //////////////////////////////////////////////////////////////////////////
  void assign(const char *const pBuf, const ssize_t len = 1);

  // //////////////////////////////////////////////////////////////////////////
  void assign(
    const uint8_t *const pBuf, 
    const ssize_t len, 
    const bool placeholder = false
  );

  // //////////////////////////////////////////////////////////////////////////
  void assign_static(uint8_t *const pBuf, const ssize_t len);

  // //////////////////////////////////////////////////////////////////////////
  sstring &operator=(const char *cstr);

  // //////////////////////////////////////////////////////////////////////////
  sstring &operator=(const sstring &rhs);

  // //////////////////////////////////////////////////////////////////////////
  char operator[](const int idx);

  // //////////////////////////////////////////////////////////////////////////
  void setChar(const int idx, const char c);

  // //////////////////////////////////////////////////////////////////////////
  bool operator==(const sstring &rhs) const ;

  // //////////////////////////////////////////////////////////////////////////
  bool operator!=(const sstring &rhs) const ;
  
  // //////////////////////////////////////////////////////////////////////////
  bool operator==(const char *rhs) const ;

  // //////////////////////////////////////////////////////////////////////////
  // Sets the length of the buffer.
  // Assumes that the buffer returned in data() was written externally.
  void set_len(const ssize_t len) ;

  // //////////////////////////////////////////////////////////////////////////
  inline ssize_t max_len() const { return mMaxSize; }

  // //////////////////////////////////////////////////////////////////////////
  // Commits some new length to the end of the buffer.
  // Assumes that the buffer returned in data() was written externally.
  void commit_append(const ssize_t len) ;

  // //////////////////////////////////////////////////////////////////////////
  void push_back(const uint8_t c);

  // //////////////////////////////////////////////////////////////////////////
  void push_char(const char c);

  // //////////////////////////////////////////////////////////////////////////
  void appendu(const uint8_t * const pC, const ssize_t len);

  // //////////////////////////////////////////////////////////////////////////
  void appendc(const char * const pC, const ssize_t len);

  // //////////////////////////////////////////////////////////////////////////
  void append_str(const sstring &rhs);

  // //////////////////////////////////////////////////////////////////////////
  // Character append.
  void appendp(const char *rhs, const bool nullTerminate = true);

    // //////////////////////////////////////////////////////////////////////////
  void operator+=(const sstring &rhs);

  // //////////////////////////////////////////////////////////////////////////
  void operator+=(const char *rhs);

  // //////////////////////////////////////////////////////////////////////////
  inline  const char *c_str() const { return (const char *)mpBuf; }

  // //////////////////////////////////////////////////////////////////////////
  inline const uint8_t *u_str() const { return (const uint8_t *)mpBuf; }

  // //////////////////////////////////////////////////////////////////////////
  uint8_t *u8DataPtr(
    const ssize_t amtToGrow = SS_INITIAL_STRING_SIZE,
    const ssize_t setLenTo  = (ssize_t )-1);
  
  // //////////////////////////////////////////////////////////////////////////
  uint16_t* u16DataPtr(
    const ssize_t amtToGrow = SS_INITIAL_STRING_SIZE / sizeof(uint16_t),
    const ssize_t setLenTo = (ssize_t)-1);

  // //////////////////////////////////////////////////////////////////////////
  uint32_t* u32DataPtr(
    const ssize_t amtToGrow = SS_INITIAL_STRING_SIZE / sizeof(uint32_t),
    const ssize_t setLenTo = (ssize_t)-1);

  // //////////////////////////////////////////////////////////////////////////
  int16_t* s16DataPtr(
      const ssize_t amtToGrow = SS_INITIAL_STRING_SIZE / sizeof(int16_t),
      const ssize_t setLenTo = (ssize_t)-1);

  // //////////////////////////////////////////////////////////////////////////
  int32_t* s32DataPtr(
    const ssize_t amtToGrow = SS_INITIAL_STRING_SIZE / sizeof(int32_t),
    const ssize_t setLenTo = (ssize_t)-1);

  // //////////////////////////////////////////////////////////////////////////
  float* floatDataPtr(
    const ssize_t amtToGrow = SS_INITIAL_STRING_SIZE / sizeof(float),
    const ssize_t setLenTo = (ssize_t)-1);

  // //////////////////////////////////////////////////////////////////////////
  double* doubleDataPtr(
    const ssize_t amtToGrow = SS_INITIAL_STRING_SIZE / sizeof(double),
    const ssize_t setLenTo = (ssize_t)-1);

  // //////////////////////////////////////////////////////////////////////////
  // Returns a writable buffer
  char *c8DataPtr(
    const ssize_t amtToGrow = SS_INITIAL_STRING_SIZE,
    const ssize_t setLenTo  = (ssize_t)-1);

  // //////////////////////////////////////////////////////////////////////////
  void clear();

  // //////////////////////////////////////////////////////////////////////////
  void trim_null();

  // //////////////////////////////////////////////////////////////////////////
  void trim_char(const char c);

  // //////////////////////////////////////////////////////////////////////////
  int index_of(const char c) const;

  // //////////////////////////////////////////////////////////////////////////
  inline ssize_t length() const { return mCurSize; }
  
  // //////////////////////////////////////////////////////////////////////////
  inline int nlength() const { return (int)mCurSize; }

  // //////////////////////////////////////////////////////////////////////////
  static ssize_t nextPowerOfTwo(ssize_t v);

  // //////////////////////////////////////////////////////////////////////////
  static bool isPowerOfTwo(const ssize_t nX);

  // //////////////////////////////////////////////////////////////////////////
  // Grows the internal buffer to at incomingSize or greater.
  void grow(const ssize_t incomingSize) ;

private:
  bool mIsHeap;
  uint8_t *mpBuf;
  ssize_t mMaxSize;
  ssize_t mCurSize;
  uint8_t mBufInitial[SS_INITIAL_STRING_SIZE];
};

#endif // #ifdef __cplusplus
#endif
