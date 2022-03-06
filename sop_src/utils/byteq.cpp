/******************************************************************************
  Copyright 2014 Chris Fogelklou, Applaud Apps (Applicaudia)

  This source code may under NO circumstances be distributed
without the express written permission of the author.

  @author: chris.fogelklou@gmail.com
*******************************************************************************/
#include "byteq.h"

#include "osal/osal.h"
#include "utils/helper_macros.h"
#include "utils/platform_log.h"

#include <stdint.h>
#include <string.h>


LOG_MODNAME("byteq.cpp")

// Circular buffering. Note, kinda unsafe - don't write more than the buffer size at once.
template <typename T1, typename T2>
void inc_buf_idx(T1& idx, const T2 bufSz, const T2 nBytes) {
  idx += nBytes;
  if (idx >= bufSz) {
    idx -= bufSz;
  }
  LOG_ASSERT(idx < bufSz);
}

//-------------------------------------------------------------------------------------------------
// Insert in the queue without mutex protection
static unsigned int ByteQUnprotectedInsert(
  ByteQ_t* const pQ, const bq_t* pWrBuf, unsigned int nLen,
  unsigned int* pNewWrIdx) {
  unsigned int bytesWritten = 0U;
  auto nWrIdx               = pQ->nWrIdx;

  if (nLen) {
    bq_t* const pBuf  = pQ->pfBuf;
    const auto nBufSz = pQ->nBufSz;

    LOG_ASSERT(pQ->nWrIdx < nBufSz);

    auto bytesToWrite = nLen;
    while (bytesToWrite > 0) {
      auto bytes = MIN(bytesToWrite, (nBufSz - nWrIdx));

      memcpy(&pBuf[ nWrIdx ], &pWrBuf[ bytesWritten ], bytes * sizeof(bq_t));

      //  Circular buffering. Note, kinda unsafe - don't write more than buffer size.
      inc_buf_idx(nWrIdx, nBufSz, bytes);

      // Increment the number of bytes written.
      bytesWritten += bytes;
      bytesToWrite -= bytes;
    }
  }
  if (nullptr != pNewWrIdx) {
    *pNewWrIdx = nWrIdx;
  }
  return bytesWritten;
}

//-------------------------------------------------------------------------------------------------
// Deallocate the things in the Q that were allocated.
bool ByteQDestroy(ByteQ_t* const pQ) {
  LOG_ASSERT(nullptr != pQ);
  return true;
}

//-------------------------------------------------------------------------------------------------
// Write, but protect only the count variable.  Don't use if multiple threads might be writing.
unsigned int
  ByteQWrite(ByteQ_t* const pQ, const bq_t* pWrBuf, unsigned int nLen) {
  unsigned int bytesWritten = 0;

  LOG_ASSERT(nullptr != pQ);

  if (nLen) {
    bq_t* const pBuf  = pQ->pfBuf;
    auto nWrIdx       = pQ->nWrIdx;
    const auto nBufSz = pQ->nBufSz;

    LOG_ASSERT(pQ->nWrIdx < nBufSz);

    auto toWrite = (nLen <= (nBufSz - pQ->nCount)) ? nLen : 0;

    // We can definitely read BytesToWrite bytes.
    while (toWrite > 0) {
      // Calculate how many contiguous bytes to the end of the buffer
      auto nBytes = MIN(toWrite, (nBufSz - nWrIdx));

      // Copy that many bytes.
      memcpy(&pBuf[ nWrIdx ], &pWrBuf[ bytesWritten ], nBytes * sizeof(bq_t));

      //  Circular buffering. Note, kinda unsafe - don't write more than buffer size.
      inc_buf_idx(nWrIdx, nBufSz, nBytes);

      // Increment the number of bytes written.
      bytesWritten += nBytes;
      toWrite -= nBytes;
    }

    pQ->nWrIdx = nWrIdx;

    // Increment the count.  (protect with mutex)
    if (pQ->wrCntProt) {
      OSALEnterCritical();
    }
    pQ->nCount = pQ->nCount + bytesWritten;
    if (pQ->wrCntProt) {
      OSALExitCritical();
    }
  }
  return bytesWritten;
}

//-------------------------------------------------------------------------------------------------
// Commit a previous write
unsigned int ByteQCommitWrite(ByteQ_t* const pQ, const unsigned int nLen) {
  unsigned int bytesWritten = 0;

  LOG_ASSERT(nullptr != pQ);

  if (nLen) {
    LOG_ASSERT(pQ->nWrIdx < pQ->nBufSz);

    //  Circular buffering. Note, kinda unsafe - don't write more than buffer size.
    inc_buf_idx(pQ->nWrIdx, pQ->nBufSz, nLen);

    // Increment the number of bytes written.
    bytesWritten += nLen;

    // Increment the count.  (protect with mutex)
    if (pQ->wrCntProt) {
      OSALEnterCritical();
    }
    pQ->nCount = pQ->nCount + nLen;
    LOG_ASSERT(pQ->nCount <= pQ->nBufSz);
    if (pQ->wrCntProt) {
      OSALExitCritical();
    }
  }
  return bytesWritten;
}


//-------------------------------------------------------------------------------------------------
// Read from the queue, allows reads from multiple threads.
unsigned int ByteQRead(ByteQ_t* const pQ, bq_t* pRdBuf, unsigned int nLen) {
  unsigned int bytesRead = 0;
  LOG_ASSERT(nullptr != pQ);

  if (nLen) {
    // Calculate how many bytes can be read from the RdBuffer.
    const auto nBufSz      = pQ->nBufSz;
    auto nRdIdx            = pQ->nRdIdx;
    const bq_t* const pBuf = pQ->pfBuf;

    LOG_ASSERT(nRdIdx < nBufSz);

    // No count MUTEX needed because count is native integer (single cycle write
    // or read)
    // and can only get larger if a process writes while we are reading.
    auto toRead = MIN(pQ->nCount, nLen);

    // We can definitely read BytesToRead bytes.
    while (toRead > 0) {
      // Calculate how many contiguous bytes to the end of the buffer
      const auto nBytes = MIN(toRead, (nBufSz - nRdIdx));

      // Copy that many bytes.
      memcpy(&pRdBuf[ bytesRead ], &pBuf[ nRdIdx ], nBytes * sizeof(bq_t));

      //  Circular buffering. Note, kinda unsafe - don't write more than buffer size.
      inc_buf_idx(nRdIdx, nBufSz, nBytes);

      // Increment the number of bytes read.
      bytesRead += nBytes;
      toRead -= nBytes;
    }

    pQ->nRdIdx = nRdIdx;

    // Decrement the count.
    if (pQ->rdCntProt) {
      OSALEnterCritical();
    }
    pQ->nCount = pQ->nCount - bytesRead;
    if (pQ->rdCntProt) {
      OSALExitCritical();
    }
  }
  return bytesRead;
}

//-------------------------------------------------------------------------------------------------
// Commit a read.
unsigned int ByteQCommitRead(ByteQ_t* const pQ, const unsigned int nLen) {
  unsigned int bytesRead = 0;
  LOG_ASSERT(nullptr != pQ);

  if (nLen) {
    LOG_ASSERT(pQ->nRdIdx < pQ->nBufSz);

    // No count MUTEX needed because count is native integer (single cycle write
    // or read)
    // and can only get larger if a process writes while we are reading.
    const auto nBytes = MIN(pQ->nCount, nLen);

    //  Circular buffering. Note, kinda unsafe - don't write more than buffer size.
    inc_buf_idx(pQ->nRdIdx, pQ->nBufSz, nBytes);

    // Increment the number of bytes read.
    bytesRead += nBytes;

    // Decrement the count.
    if (pQ->rdCntProt) {
      OSALEnterCritical();
    }
    pQ->nCount = pQ->nCount - nLen;
    if (pQ->rdCntProt) {
      OSALExitCritical();
    }
  }
  return bytesRead;
}

//-------------------------------------------------------------------------------------------------
unsigned int ByteQGetWriteReady(ByteQ_t* const pQ) {
  LOG_ASSERT(nullptr != pQ);

  if (pQ->wrCntProt) {
    OSALEnterCritical();
  }

  const unsigned int rval = (pQ->nBufSz - pQ->nCount);

  if (pQ->wrCntProt) {
    OSALExitCritical();
  }

  return rval;
}


//-------------------------------------------------------------------------------------------------
unsigned int ByteQGetContiguousWriteReady(ByteQ_t* const pQ) {
  LOG_ASSERT(nullptr != pQ);

  if (pQ->wrCntProt) {
    OSALEnterCritical();
  }

  unsigned int bytesReady = (pQ->nBufSz - pQ->nCount);
  bytesReady = MIN(bytesReady, pQ->nBufSz - pQ->nWrIdx);

  if (pQ->wrCntProt) {
    OSALExitCritical();
  }
  return bytesReady;
}

//-------------------------------------------------------------------------------------------------
unsigned int ByteQGetReadReady(ByteQ_t* const pQ) {
  LOG_ASSERT(nullptr != pQ);

  if (pQ->rdCntProt) {
    OSALEnterCritical();
  }

  const unsigned int bytesReady = pQ->nCount;

  if (pQ->rdCntProt) {
    OSALExitCritical();
  }

  return bytesReady;
}

//-------------------------------------------------------------------------------------------------
unsigned int ByteQGetContiguousReadReady(ByteQ_t* const pQ) {
  LOG_ASSERT(nullptr != pQ);

  if (pQ->rdCntProt) {
    OSALEnterCritical();
  }
  const unsigned int bytesReady =
    MIN(pQ->nCount, pQ->nBufSz - pQ->nRdIdx);
  if (pQ->rdCntProt) {
    OSALExitCritical();
  }

  return bytesReady;
}

//-------------------------------------------------------------------------------------------------
void ByteQFlush(ByteQ_t* const pQ) {
  // Get the read mutex to only allow a single thread to
  // read from the queue at a time.

  LOG_ASSERT(nullptr != pQ);

  // delete the single count mutex
  if ((pQ->rdCntProt) || (pQ->wrCntProt)) {
    OSALEnterCritical();
  }

  pQ->nCount = 0;
  pQ->nRdIdx = pQ->nWrIdx = 0;

  if ((pQ->rdCntProt) || (pQ->wrCntProt)) {
    OSALExitCritical();
  }
}


//-------------------------------------------------------------------------------------------------
unsigned int ByteQPeek(ByteQ_t* const pQ, bq_t* pRdBuf, const unsigned int nLen) {
  unsigned int bytesRead = 0;

  LOG_ASSERT(nullptr != pQ);
  if (nLen) {
    auto nRdIdx = pQ->nRdIdx;

    // Calculate how many bytes can be read from the RdBuffer.
    auto bytesToRead = MIN(pQ->nCount, nLen);

    LOG_ASSERT(nRdIdx < pQ->nBufSz);

    // We can definitely read BytesToRead bytes.
    while (bytesToRead > 0) {
      // Calculate how many contiguous bytes to the end of the buffer
      const auto nBytes = MIN(bytesToRead, (pQ->nBufSz - nRdIdx));

      // Copy that many bytes.
      memcpy(&pRdBuf[ bytesRead ], &pQ->pfBuf[ nRdIdx ], nBytes * sizeof(bq_t));

      //  Circular buffering. Note, kinda unsafe - don't write more than buffer size.
      inc_buf_idx(nRdIdx, pQ->nBufSz, nBytes);

      // Increment the number of bytes read.
      bytesRead += nBytes;
      bytesToRead -= nBytes;
    }
  }
  return bytesRead;
}


//-------------------------------------------------------------------------------------------------
void* ByteQGetWritePtr(ByteQ_t* const pQ) {
  if (pQ->wrCntProt) {
    OSALEnterCritical();
  }
  void* const pRVal = (void*)&pQ->pfBuf[ pQ->nWrIdx ];
  if (pQ->wrCntProt) {
    OSALExitCritical();
  }

  return pRVal;
}

//-------------------------------------------------------------------------------------------------
void* ByteQGetReadPtr(ByteQ_t* const pQ) {
  if (pQ->rdCntProt) {
    OSALEnterCritical();
  }
  void* const pRVal = (void*)&pQ->pfBuf[ pQ->nRdIdx ];
  if (pQ->rdCntProt) {
    OSALExitCritical();
  }

  return pRVal;
}

//-------------------------------------------------------------------------------------------------
// The pointer manipulation functions only work if uint8_t is 8 bits. Check this.
void ByteQVerifySizes() {
  ASSERT_AT_COMPILE_TIME(sizeof(uint8_t) == 1);
  ASSERT_AT_COMPILE_TIME(sizeof(uint32_t) != sizeof(uint8_t));
}

//-------------------------------------------------------------------------------------------------
void ByteQSetRdIdxFromPointer(ByteQ_t* const pQ, void* pRdPtr) {
  if (pQ->rdCntProt) {
    OSALEnterCritical();
  }

  const auto pRd8 = (const uint8_t*)pRdPtr;

  // Force int here because we want this to be signed.
  intptr_t newRdIdx = pRd8 - pQ->pfBuf;

  // Check for within range.
  if ((newRdIdx >= 0) && (newRdIdx <= (int)pQ->nBufSz)) {
    // If last read advanced pointer to end of buffer,
    // this is OK, just set to beginning.
    if (newRdIdx == (int)pQ->nBufSz) {
      newRdIdx = 0;
    }

    // New count is amount write is ahead of read.
    int newCount = (int)(pQ->nWrIdx - newRdIdx);

    // Assume we are being called from consumer, so wr==rd
    // results in zero count
    if (newCount < 0) {
      newCount += pQ->nBufSz; // lint !e713 !e737
    }

    // Set read index and count.
    pQ->nRdIdx = (unsigned int)newRdIdx;
    pQ->nCount = (unsigned int)newCount;
  }

  if (pQ->rdCntProt) {
    OSALExitCritical();
  }
}

//-------------------------------------------------------------------------------------------------
unsigned int
  ByteQForceWrite(ByteQ_t* const pQ, const bq_t* const pWrBuf, unsigned int nLen) {
  unsigned int bytes = 0;
  LOG_ASSERT(nullptr != pQ);

  if (nLen) {
    int diff                    = 0;
    unsigned int writeableBytes = 0;
    unsigned int newWrIdx       = 0;

    // Lock both read and write mutexes


    // Calculate the number of bytes that can be written
    writeableBytes = pQ->nBufSz - pQ->nCount;
    diff = nLen - writeableBytes; // lint !e713 !e737

    // If more bytes should be written than there is space
    // for, force the read pointer forward
    if (diff > 0) {
      pQ->nRdIdx += diff; // lint !e713 !e737
      while (pQ->nRdIdx >= pQ->nBufSz) {
        pQ->nRdIdx -= pQ->nBufSz;
      }

      if (pQ->rdCntProt) {
        OSALEnterCritical();
      }
      pQ->nCount -= diff; // lint !e713 !e737
      if (pQ->rdCntProt) {
        OSALExitCritical();
      }
    }

    // Insert the data in the buffer.
    LOG_ASSERT_FN(nLen == ByteQUnprotectedInsert(pQ, pWrBuf, nLen, &newWrIdx));
    pQ->nWrIdx = newWrIdx;

    if (pQ->wrCntProt) {
      OSALEnterCritical();
    }
    pQ->nCount += nLen;
    if (pQ->wrCntProt) {
      OSALExitCritical();
    }

    bytes = nLen;
  }
  return bytes;
}

//-------------------------------------------------------------------------------------------------
unsigned int ByteQForceWriteUnprotected(
  ByteQ_t* const pQ, const bq_t* const pWrBuf, const int nLen) {
  LOG_ASSERT(nullptr != pQ);

  if (nLen > 0) {
    // Advance read pointer if the buffer is full.
    {
      const unsigned int writeableBytes = pQ->nBufSz - pQ->nCount;
      const int diff = nLen - writeableBytes;
      if (diff > 0) {
        pQ->nRdIdx += diff;
        while (pQ->nRdIdx >= pQ->nBufSz) {
          pQ->nRdIdx -= pQ->nBufSz;
        }
        pQ->nCount -= diff;
      }
    }

    // Write into the buffer.
    {
      int remaining = nLen;
      int written   = 0;
      while (remaining > 0) {
        const int contig = pQ->nBufSz - pQ->nWrIdx;
        const int bytes  = MIN(remaining, contig);

        memcpy(&pQ->pfBuf[ pQ->nWrIdx ], &pWrBuf[ written ], bytes);

        pQ->nWrIdx += bytes;
        if (pQ->nWrIdx >= pQ->nBufSz) {
          pQ->nWrIdx -= pQ->nBufSz;
        }

        written += bytes;
        remaining -= bytes;
      }
      pQ->nCount += nLen;
    }
  }
  return nLen;
}


unsigned int ByteQForceCommitWrite(ByteQ_t* const pQ, unsigned int nLen) {
  unsigned int bytes = 0;
  LOG_ASSERT(nullptr != pQ);

  if (nLen) {
    int diff                    = 0;
    unsigned int writeableBytes = 0;

    // Calculate the number of bytes that can be written
    writeableBytes = pQ->nBufSz - pQ->nCount;
    diff = nLen - writeableBytes; // lint !e713 !e737

    // If more bytes should be written than there is space
    // for, force the read pointer forward
    if (diff > 0) {
      pQ->nRdIdx += diff; // lint !e713 !e737
      while (pQ->nRdIdx >= pQ->nBufSz) {
        pQ->nRdIdx -= pQ->nBufSz;
      }

      if (pQ->rdCntProt) {
        OSALEnterCritical();
      }
      pQ->nCount -= diff; // lint !e713 !e737
      if (pQ->rdCntProt) {
        OSALExitCritical();
      }
    }

    // Circular buffering.
    pQ->nWrIdx += nLen;
    if (pQ->nWrIdx >= pQ->nBufSz) {
      pQ->nWrIdx -= pQ->nBufSz;
    }

    if (pQ->wrCntProt) {
      OSALEnterCritical();
    }
    pQ->nCount += nLen;
    if (pQ->wrCntProt) {
      OSALExitCritical();
    }

    bytes = nLen;
  }
  return bytes;
}

//-------------------------------------------------------------------------------------------------
unsigned int ByteQPeekRandom(
  ByteQ_t* const pQ, bq_t* pRdBuf,
  unsigned int bytesFromRdIdx, unsigned int nLen) {
  unsigned int BytesRead = 0;

  LOG_ASSERT(nullptr != pQ);
  if (nLen) {
    unsigned int BytesToRead;
    int nCount;

    unsigned int nRdIdx = pQ->nRdIdx + bytesFromRdIdx;
    if (nRdIdx >= pQ->nBufSz) {
      nRdIdx -= pQ->nBufSz;
    }
    nCount = (pQ->nCount - bytesFromRdIdx); // lint !e713 !e737
    nCount = (nCount < 0) ? 0 : nCount;

    // Calculate how many bytes can be read from the RdBuffer.
    BytesToRead = MIN(((unsigned int)nCount), nLen);

    // We can definitely read BytesToRead bytes.
    while (BytesToRead > 0) {
      // Calculate how many contiguous bytes to the end of the buffer
      unsigned int Bytes = MIN(BytesToRead, (pQ->nBufSz - nRdIdx));

      // Copy that many bytes.
      memcpy(&pRdBuf[ BytesRead ], &pQ->pfBuf[ nRdIdx ], Bytes * sizeof(bq_t));

      // Circular buffering.
      nRdIdx += Bytes;
      if (nRdIdx >= pQ->nBufSz) {
        nRdIdx -= pQ->nBufSz;
      }

      // Increment the number of bytes read.
      BytesRead += Bytes;
      BytesToRead -= Bytes;
    }
  }
  return BytesRead;
}

/** [Declaration] Inserts data somewhere into the buffer */
unsigned int ByteQPokeRandom(
  ByteQ_t* const pQ, bq_t* pWrBuf,
  unsigned int bytesFromStart, unsigned int nLen) {
  unsigned int BytesWritten = 0;

  LOG_ASSERT(nullptr != pQ);
  if (nLen) {
    unsigned int nWrIdx;
    unsigned int BytesToWrite;
    int nCount;

    nWrIdx = pQ->nWrIdx + bytesFromStart;
    if (nWrIdx >= pQ->nBufSz) {
      nWrIdx -= pQ->nBufSz;
    }

    // Can only write if it will fit within nCount
    nCount = pQ->nCount - bytesFromStart; // lint !e713 !e737
    nCount = (nCount < 0) ? 0 : nCount;

    // Calculate how many bytes can be written to the WrBuffer.
    BytesToWrite = MIN(((unsigned int)nCount), nLen);

    // We can definitely read BytesToRead bytes.
    while (BytesToWrite > 0) {
      // Calculate how many contiguous bytes to the end of the buffer
      unsigned int Bytes = MIN(BytesToWrite, (pQ->nBufSz - nWrIdx));

      // Copy that many bytes.
      memcpy(&pQ->pfBuf[ nWrIdx ], &pWrBuf[ BytesWritten ], Bytes * sizeof(bq_t));

      // Circular buffering.
      nWrIdx += Bytes;
      if (nWrIdx >= pQ->nBufSz) {
        nWrIdx -= pQ->nBufSz;
      }

      // Increment the number of bytes read.
      BytesWritten += Bytes;
      BytesToWrite -= Bytes;
    }
  }
  return BytesWritten;
}

/** [Declaration] Reads the last nLen bytes from the buffer */
int ByteQDoReadFromEnd(ByteQ_t* const pQ, bq_t* pRdBuf, int nLen) {
  int shortsRead = 0;

  if (nLen > 0) {
    // Calculate how many shorts can be read from the RdBuffer.
    int shortsToRead = 0;
    int nRdIdx       = pQ->nWrIdx - nLen;
    if (nRdIdx < 0) {
      nRdIdx += pQ->nBufSz;
    }

    shortsToRead = nLen;

    // We can definitely read ShortsToRead shorts.
    while (shortsToRead > 0) {
      // Calculate how many contiguous shorts to the end of the buffer
      int Shorts = MIN(shortsToRead, (int)((pQ->nBufSz - nRdIdx)));

      // Copy that many shorts.
      memcpy(&pRdBuf[ shortsRead ], &pQ->pfBuf[ nRdIdx ], Shorts * sizeof(bq_t));

      // Circular buffering.
      nRdIdx += Shorts;
      if (nRdIdx >= (int)pQ->nBufSz) {
        nRdIdx -= pQ->nBufSz;
      }

      // Increment the number of shorts read.
      shortsRead += Shorts;
      shortsToRead -= Shorts;
    }
  }
  return shortsRead;
}


/*
**=============================================================================
**  Abstract:
**    Public function to initialize the ByteQ_t structure.
**
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
bool ByteQCreate(
  ByteQ_t* const pQ, bq_t* pBuf, unsigned int nBufSz,
  bool lockOnWrites, bool lockOnReads) {
  LOG_ASSERT((nullptr != pQ) && (nullptr != pBuf));

  memset(pQ, 0, sizeof(ByteQ_t));

  pQ->pfBuf  = pBuf;
  pQ->nBufSz = nBufSz;

  pQ->wrCntProt = lockOnWrites;
  pQ->rdCntProt = lockOnReads;

  return true;
}
