/**
 * COPYRIGHT	(c)	Applicaudia 2017
 * @file        buf_io.hpp
 * Basic Read/Write API to support buffered reads and writes.
 * It is expected that bytes written to this API are immediately buffered for
 * transmission (set and forget.)
 * Read() commands read from the incoming data buffer, so return only the bytes that
 * have already been received by the internal buffer.
 */
#ifndef APPLICAUDIA_LIB_BLEIO_H
#define APPLICAUDIA_LIB_BLEIO_H

//#include "utils/dl_list.h"
#include <stdint.h>

struct BufIOEventDataTag;
class sstring;

// ////////////////////////////////////////////////////////////////////////////
// In order to allow C access to a BufIO CPP Object.
typedef struct BufIOCRefTag {
  void* pThis; // This is a pointer to the BufIO object.
} BufIOCRefT;

// ////////////////////////////////////////////////////////////////////////////
// START C++ Only definitions
#ifdef __cplusplus
struct BLEIOBaseTag;
class BufIO {
public:
  // Constructor
  BufIO();

  virtual ~BufIO();

  // Write function, returns amount written.  Typically uses a FIFO.
  virtual int Write(const uint8_t* const pBytes, const int numBytes);

  // Get Write Ready.  Returns amount that can be written.
  virtual int GetWriteReady() const;

  // Write function: Calls the other Write function directly.
  virtual int Write(const sstring& buf);

  // Read function - returns amount available.  Typically uses an internal FIFO.
  virtual int Read(uint8_t* const pBytes, const int numBytes);

  // Get the number of bytes that can be read.
  virtual int GetReadReady() const;

  // Read function - returns amount available.  Typically uses an internal FIFO.
  virtual int Read(sstring& buf);

  // Get a pointer to the C Ref
  BufIOCRefT* GetCRef();

  // Get a reference that can be used by legacy C code.
  static BufIO* GetCppRef(const BufIOCRefT* const pCRef);

public:
  // The following functions are only declared public to allow access from the "C" API.
  // Do not consider them public - they should not be called by code using this API

  // Get a pointer to the pure-C API associated with this class, if there is one.
  struct BLEIOBaseTag* GetCBleCApi();

  // Get a pointer to the pure-C API associated with this class, if there is one.
  void SetCBleCApi(struct BLEIOBaseTag* const pCApi);

protected:
  // An internal structure with the this pointer.
  BufIOCRefT thisRef;

  // Internal pointer to a C API, if connected.
  struct BLEIOBaseTag* pBleCApi;
};

#endif


#ifdef __cplusplus
extern "C" {
#endif


// ////////////////////////////////////////////////////////////////////////////
// C-accessible write function, for backwards compatibility with legacy C code.
int BufIOWrite(const BufIOCRefT* const pBaseIo, const uint8_t* const pBytes, const int numBytes);

// ////////////////////////////////////////////////////////////////////////////
// C-accessible GetReadReady function, for backwards compatibility with legacy C code.
int BufIOGetWriteReady(const BufIOCRefT* const pBaseIo);

// ////////////////////////////////////////////////////////////////////////////
// C-accessible Read function, for backwards compatibility with legacy C code.
int BufIORead(const BufIOCRefT* const pBaseIo, uint8_t* const pBytes, const int numBytes);

// ////////////////////////////////////////////////////////////////////////////
// C-accessible GetReadReady function, for backwards compatibility with legacy C code.
int BufIOGetReadReady(const BufIOCRefT* const pBaseIo);

#ifdef __cplusplus
}
#endif

#endif // APPLICAUDIA_LIB_BLEIO_H
