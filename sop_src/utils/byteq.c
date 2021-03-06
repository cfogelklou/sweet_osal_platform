/******************************************************************************
  Copyright 2014 Chris Fogelklou, Applaud Apps (Applicaudia)

  This source code may under NO circumstances be distributed without the
  express written permission of the author.

  @author: chris.fogelklou@gmail.com
*******************************************************************************/
#include "byteq.h"
//#include <math.h>
#include <stdint.h>
#include <string.h>

#include "osal/osal.h"
#define Platform_EnterOSCritical OSALEnterCritical
#define Platform_ExitOSCritical OSALExitCritical

#include "utils/helper_macros.h"
#include "utils/platform_log.h"


#define ASSERT LOG_ASSERT
#define ASSERT_FN LOG_ASSERT_FN

LOG_MODNAME("byteq.c")


typedef int32_t msg_hdr_t;
#define MSG_ALIGN (sizeof(msg_hdr_t))
#define DOALIGN(_LEN) (((_LEN) + MSG_ALIGN - 1) & (unsigned int)(0 - MSG_ALIGN))

#if 1 // def WIN32
#define COMPLEX_QUEUE
#endif

#ifdef COMPLEX_QUEUE
/*
**=============================================================================
**=============================================================================
*/
static unsigned int ByteQUnprotectedInsert(ByteQ_t *const pQ, const bq_t *pWrBuf,
                                     unsigned int nLen, unsigned int *pNewWrIdx) {
  unsigned int BytesWritten = 0;
  unsigned int nWrIdx = pQ->nWrIdx;

  if (nLen) {
    // Write nothing if there isn't room in the buffer.
    unsigned int BytesToWrite = nLen;
    const unsigned int nBufSz = pQ->nBufSz;
    bq_t *const pBuf = pQ->pfBuf;

    while (BytesToWrite > 0) {
      unsigned int Bytes = MIN(BytesToWrite, (nBufSz - nWrIdx));

      memcpy(&pBuf[nWrIdx], &pWrBuf[BytesWritten], Bytes * sizeof(bq_t));

      // Circular buffering.
      nWrIdx += Bytes;
      if (nWrIdx >= nBufSz) {
        nWrIdx -= nBufSz;
      }

      // Increment the number of bytes written.
      BytesWritten += Bytes;
      BytesToWrite -= Bytes;
    }
  }
  if (NULL != pNewWrIdx) {
    *pNewWrIdx = nWrIdx;
  }
  return BytesWritten;
}
#endif

/*
**=============================================================================
**  Abstract:
**  Public function to deallocate the things in the
**   Q that were allocated.
**
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
bool ByteQDestroy(ByteQ_t *const pQ) {
  ASSERT(NULL != pQ);
  return true;
}

/*
**=============================================================================
**  Abstract:
**  Write function that lacks protection from multiple threads trying to write
**  at the same time.  Use this function if only one thread/task will write
**  to the queue at a time.
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
unsigned int ByteQWrite(ByteQ_t *const pQ, const bq_t *pWrBuf, unsigned int nLen) {
  unsigned int bytesWritten = 0;

  ASSERT(NULL != pQ);

  if (nLen) {
    // Write nothing if there isn't room in the buffer.
    unsigned int toWrite = 0;
    unsigned int nWrIdx = pQ->nWrIdx;
    const unsigned int nBufSz = pQ->nBufSz;
    bq_t *const pBuf = pQ->pfBuf;    

    toWrite = (nLen <= (nBufSz - pQ->nCount)) ? nLen : 0;

    // We can definitely read BytesToWrite bytes.
    while (toWrite > 0) {
      // Calculate how many contiguous bytes to the end of the buffer
      unsigned int nBytes = MIN(toWrite, (nBufSz - nWrIdx));

      // Copy that many bytes.
      memcpy(&pBuf[nWrIdx], &pWrBuf[bytesWritten], nBytes * sizeof(bq_t));

      // Circular buffering.
      nWrIdx += nBytes;
      if (nWrIdx >= nBufSz) {
        nWrIdx -= nBufSz;
      }

      // Increment the number of bytes written.
      bytesWritten += nBytes;
      toWrite -= nBytes;
    }

    pQ->nWrIdx = nWrIdx;

    // Increment the count.  (protect with mutex)
    if (pQ->wrCntProt) {
      Platform_EnterOSCritical();
    }
    pQ->nCount = pQ->nCount + bytesWritten;
    if (pQ->wrCntProt) {
      Platform_ExitOSCritical();
    }

  }
  return bytesWritten;
}



/*
**=============================================================================
**=============================================================================
*/
unsigned int ByteQCommitWrite(ByteQ_t *const pQ, unsigned int nLen) {
  unsigned int bytesWritten = 0;

  ASSERT(NULL != pQ);

  if (nLen) {

    // Circular buffering.
    pQ->nWrIdx += nLen;
    if (pQ->nWrIdx >= pQ->nBufSz) {
      pQ->nWrIdx -= pQ->nBufSz;
    }

    // Increment the number of bytes written.
    bytesWritten += nLen;

    // Increment the count.  (protect with mutex)
    if (pQ->wrCntProt) {
      Platform_EnterOSCritical();
    }
    pQ->nCount = pQ->nCount + nLen;
    LOG_ASSERT(pQ->nCount <= pQ->nBufSz);
    if (pQ->wrCntProt) {
      Platform_ExitOSCritical();
    }
  }
  return bytesWritten;
}


/*
**=============================================================================
**  Abstract:
**  Read function that lacks protection from multiple threads trying to read
**  at the same time.  Use this function if only one thread/task will read
**  from the queue at a time.
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
unsigned int ByteQRead(ByteQ_t *const pQ, bq_t *pRdBuf, unsigned int nLen) {
  unsigned int bytesRead = 0;
  ASSERT(NULL != pQ);

  if (nLen) {
    // Calculate how many bytes can be read from the RdBuffer.
    unsigned int toRead = 0;
    const unsigned int nBufSz = pQ->nBufSz;
    unsigned int nRdIdx = pQ->nRdIdx;
    const bq_t *const pBuf = pQ->pfBuf;

    

    // No count MUTEX needed because count is native integer (single cycle write
    // or read)
    // and can only get larger if a process writes while we are reading.
    toRead = MIN(pQ->nCount, nLen);

    // We can definitely read BytesToRead bytes.
    while (toRead > 0) {
      // Calculate how many contiguous bytes to the end of the buffer
      unsigned int nBytes = MIN(toRead, (nBufSz - nRdIdx));

      // Copy that many bytes.
      memcpy(&pRdBuf[bytesRead], &pBuf[nRdIdx], nBytes * sizeof(bq_t));

      // Circular buffering.
      nRdIdx += nBytes;
      if (nRdIdx >= nBufSz) {
        nRdIdx -= nBufSz;
      }

      // Increment the number of bytes read.
      bytesRead += nBytes;
      toRead -= nBytes;
    }

    pQ->nRdIdx = nRdIdx;

    // Decrement the count.
    // ByteQ_LOCKIFDEFINED( pQ->rdCntMutex );
    if (pQ->rdCntProt) {
      Platform_EnterOSCritical();
    }
    pQ->nCount = pQ->nCount - bytesRead;
    // ByteQ_UNLOCK_IFDEFINED( pQ->rdCntMutex );
    if (pQ->rdCntProt) {
      Platform_ExitOSCritical();
    }
  }
  return bytesRead;
}

/*
**=============================================================================
**  Abstract:
**  Read function that lacks protection from multiple threads trying to read
**  at the same time.  Use this function if only one thread/task will read
**  from the queue at a time.
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
unsigned int ByteQCommitRead(ByteQ_t *const pQ, unsigned int nLen) {
  unsigned int bytesRead = 0;
  ASSERT(NULL != pQ);

  if (nLen) {
    

    // No count MUTEX needed because count is native integer (single cycle write
    // or read)
    // and can only get larger if a process writes while we are reading.
    nLen = MIN(pQ->nCount, nLen);

    pQ->nRdIdx += nLen;
    if (pQ->nRdIdx >= pQ->nBufSz) {
      pQ->nRdIdx -= pQ->nBufSz;
    }

    // Increment the number of bytes read.
    bytesRead += nLen;

    // Decrement the count.
    if (pQ->rdCntProt) {
      Platform_EnterOSCritical();
    }
    pQ->nCount = pQ->nCount - nLen;
    if (pQ->rdCntProt) {
      Platform_ExitOSCritical();
    }
  }
  return bytesRead;
}

/*
**=============================================================================
**  Abstract:
**
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
unsigned int ByteQGetWriteReady(ByteQ_t *const pQ) {
  unsigned int rval = 0;
  ASSERT(NULL != pQ);

  if (pQ->wrCntProt) {
    Platform_EnterOSCritical();
  }

  rval = (pQ->nBufSz - pQ->nCount);

  if (pQ->wrCntProt) {
    Platform_ExitOSCritical();
  }

  return rval;
}


/*
**=============================================================================
**  Abstract:
**
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
unsigned int ByteQGetContiguousWriteReady(ByteQ_t *const pQ) {

  unsigned int bytesReady = 0;
  ASSERT(NULL != pQ);

  if (pQ->wrCntProt) {
    Platform_EnterOSCritical();
  }

  bytesReady = (pQ->nBufSz - pQ->nCount);
  bytesReady = MIN(bytesReady, pQ->nBufSz - pQ->nWrIdx);

  if (pQ->wrCntProt) {
    Platform_ExitOSCritical();
  }
  return bytesReady;
}

/*
**=============================================================================
**  Abstract:
**
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
unsigned int ByteQGetReadReady(ByteQ_t *const pQ) {
  unsigned int bytesReady = 0;
  ASSERT(NULL != pQ);

  if (pQ->rdCntProt) {
    Platform_EnterOSCritical();
  }

  bytesReady = pQ->nCount;

  if (pQ->rdCntProt) {
    Platform_ExitOSCritical();
  }

  return bytesReady;
}

/*
**=============================================================================
**  Abstract:
**
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
unsigned int ByteQGetContiguousReadReady(ByteQ_t *const pQ) {

  unsigned int bytesReady = 0;
  ASSERT(NULL != pQ);

  if (pQ->rdCntProt) {
    Platform_EnterOSCritical();
  }
  bytesReady = MIN(pQ->nCount, pQ->nBufSz - pQ->nRdIdx);
  if (pQ->rdCntProt) {
    Platform_ExitOSCritical();
  }

  return bytesReady;
}

/*
**=============================================================================
**  Abstract:
**
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
void ByteQFlush(ByteQ_t *const pQ) {
  // Get the read mutex to only allow a single thread to read from the
  // queue at a time.

  ASSERT(NULL != pQ);

  // delete the single count mutex
  if ((pQ->rdCntProt) || (pQ->wrCntProt)) {
    Platform_EnterOSCritical();
  }

  pQ->nCount = 0;
  pQ->nRdIdx = pQ->nWrIdx = 0;

  if ((pQ->rdCntProt) || (pQ->wrCntProt)) {
    Platform_ExitOSCritical();
  }

}

#ifdef COMPLEX_QUEUE
/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
unsigned int ByteQPeek(ByteQ_t *const pQ, bq_t *pRdBuf, unsigned int nLen) {
  unsigned int bytesRead = 0;

  ASSERT(NULL != pQ);
  if (nLen) {

    unsigned int nRdIdx = pQ->nRdIdx;
    
    // Calculate how many bytes can be read from the RdBuffer.
    unsigned int bytesToRead = MIN(pQ->nCount, nLen);

    // We can definitely read BytesToRead bytes.
    while (bytesToRead > 0) {
      // Calculate how many contiguous bytes to the end of the buffer
      unsigned int Bytes = MIN(bytesToRead, (pQ->nBufSz - nRdIdx));

      // Copy that many bytes.
      memcpy(&pRdBuf[bytesRead], &pQ->pfBuf[nRdIdx], Bytes * sizeof(bq_t));

      // Circular buffering.
      nRdIdx += Bytes;
      if (nRdIdx >= pQ->nBufSz) {
        nRdIdx -= pQ->nBufSz;
      }

      // Increment the number of bytes read.
      bytesRead += Bytes;
      bytesToRead -= Bytes;
    }
  }
  return bytesRead;
}

#endif // #ifdef COMPLEX_QUEUE

/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
void *ByteQGetWritePtr(ByteQ_t *const pQ) {
  void *pRVal = 0;

  if (pQ->wrCntProt) {
    Platform_EnterOSCritical();
  }
  pRVal = (void *)&pQ->pfBuf[pQ->nWrIdx];
  if (pQ->wrCntProt) {
    Platform_ExitOSCritical();
  }

  return pRVal;
}

/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
void *ByteQGetReadPtr(ByteQ_t *const pQ) {
  void *pRVal = 0;

  if (pQ->rdCntProt) {
    Platform_EnterOSCritical();
  }
  pRVal = (void *)&pQ->pfBuf[pQ->nRdIdx];
  if (pQ->rdCntProt) {
    Platform_ExitOSCritical();
  }

  return pRVal;
}

#ifdef COMPLEX_QUEUE
/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
void ByteQSetRdIdxFromPointer(ByteQ_t *const pQ, void *pRdPtr) {
  if (pQ->rdCntProt) {
    Platform_EnterOSCritical();
  }
  {
    const bq_t *const pRd8 = (const bq_t *)pRdPtr;

    int newRdIdx = pRd8 - pQ->pfBuf; // lint !e946

    // Check for within range.
    if ((newRdIdx >= 0) && (newRdIdx <= (int)pQ->nBufSz)) // lint !e574 !e737
    {
      int newCount;

      // If last read advanced pointer to end of buffer, this is OK, just set to
      // beginning.
      if (newRdIdx == (int)pQ->nBufSz) // lint !e737
      {
        newRdIdx = 0;
      }

      // New count is amount write is ahead of read.
      newCount = (int)(pQ->nWrIdx - newRdIdx);

      // Assume we are being called from consumer, so wr==rd results in zero
      // count
      if (newCount < 0) {
        //(Lint):Info 737: Loss of sign in promotion from int to unsigned int
        //(Lint):Info 713: Loss of precision (assignment) (unsigned int to int)
        newCount += pQ->nBufSz; // lint !e713 !e737
      }

      // Set read index and count.
      pQ->nRdIdx = (unsigned int)newRdIdx;
      pQ->nCount = (unsigned int)newCount;
    }
  }
  if (pQ->rdCntProt) {
    Platform_ExitOSCritical();
  }
}

/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
unsigned int ByteQUnread(ByteQ_t *const pQ, unsigned int nLen) {
  unsigned int bytesUnread = 0;
  ASSERT(NULL != pQ);

  if (nLen) {
    // Calculate how many bytes can be read from the RdBuffer.
    unsigned int BytesToUnRead = 0;
    int nReadIdx = 0;

    

    // No count MUTEX needed because count is native integer (single cycle write
    // or read)
    // and can only get larger if a process writes while we are reading.
    BytesToUnRead = MIN((pQ->nBufSz - pQ->nCount), nLen);

    // We can definitely read BytesToRead bytes.
    nReadIdx = (int)pQ->nRdIdx - BytesToUnRead; // lint !e713 !e737
    if (nReadIdx < 0) {
      nReadIdx += pQ->nBufSz; // lint !e713 !e737
    }
    pQ->nRdIdx = (unsigned int)nReadIdx;

    // Decrement the count.
    if (pQ->rdCntProt) {
      Platform_EnterOSCritical();
    }
    pQ->nCount = pQ->nCount + BytesToUnRead;
    if (pQ->rdCntProt) {
      Platform_ExitOSCritical();
    }
    bytesUnread = BytesToUnRead;
  }
  return bytesUnread;
}

/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
unsigned int ByteQForceWrite(ByteQ_t *const pQ, const bq_t *const pWrBuf,
                       unsigned int nLen) {
  unsigned int bytes = 0;
  ASSERT(NULL != pQ);

  if (nLen) {
    int diff = 0;
    unsigned int writeableBytes = 0;
    unsigned int newWrIdx = 0;

    // Lock both read and write mutexes
    
    

    // Calculate the number of bytes that can be written
    writeableBytes = pQ->nBufSz - pQ->nCount;
    diff = nLen - writeableBytes; // lint !e713 !e737

    // If more bytes should be written than there is space for,
    // force the read pointer forward
    if (diff > 0) {
      pQ->nRdIdx += diff; // lint !e713 !e737
      while (pQ->nRdIdx >= pQ->nBufSz) {
        pQ->nRdIdx -= pQ->nBufSz;
      }

      if (pQ->rdCntProt) {
        Platform_EnterOSCritical();
      }
      pQ->nCount -= diff; // lint !e713 !e737
      if (pQ->rdCntProt) {
        Platform_ExitOSCritical();
      }
    }

    // Insert the data in the buffer.
    ASSERT_FN(nLen == ByteQUnprotectedInsert(pQ, pWrBuf, nLen, &newWrIdx));
    pQ->nWrIdx = newWrIdx;

    if (pQ->wrCntProt) {
      Platform_EnterOSCritical();
    }
    pQ->nCount += nLen;
    if (pQ->wrCntProt) {
      Platform_ExitOSCritical();
    }

    bytes = nLen;
    
    
  }
  return bytes;
}

/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
unsigned int ByteQForceWriteUnprotected(
  ByteQ_t *const pQ,
  const bq_t *const pWrBuf, const int nLen) {
  LOG_ASSERT(NULL != pQ);

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
      int written = 0;
      while (remaining > 0) {
        const int contig = pQ->nBufSz - pQ->nWrIdx;
        const int bytes = MIN(remaining, contig);

        memcpy(&pQ->pfBuf[pQ->nWrIdx], &pWrBuf[written], bytes);

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


unsigned int ByteQForceCommitWrite(ByteQ_t *const pQ, unsigned int nLen) {
  unsigned int bytes = 0;
  ASSERT(NULL != pQ);

  if (nLen) {
    int diff = 0;
    unsigned int writeableBytes = 0;

    // Calculate the number of bytes that can be written
    writeableBytes = pQ->nBufSz - pQ->nCount;
    diff = nLen - writeableBytes; // lint !e713 !e737

                                  // If more bytes should be written than there is space for,
                                  // force the read pointer forward
    if (diff > 0) {
      pQ->nRdIdx += diff; // lint !e713 !e737
      while (pQ->nRdIdx >= pQ->nBufSz) {
        pQ->nRdIdx -= pQ->nBufSz;
      }

      if (pQ->rdCntProt) {
        Platform_EnterOSCritical();
      }
      pQ->nCount -= diff; // lint !e713 !e737
      if (pQ->rdCntProt) {
        Platform_ExitOSCritical();
      }
    }

    // Circular buffering.
    pQ->nWrIdx += nLen;
    if (pQ->nWrIdx >= pQ->nBufSz) {
      pQ->nWrIdx -= pQ->nBufSz;
    }

    if (pQ->wrCntProt) {
      Platform_EnterOSCritical();
    }
    pQ->nCount += nLen;
    if (pQ->wrCntProt) {
      Platform_ExitOSCritical();
    }

    bytes = nLen;


  }
  return bytes;
}

/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
unsigned int ByteQPeekRandom(ByteQ_t *const pQ, bq_t *pRdBuf, unsigned int bytesFromRdIdx,
                       unsigned int nLen) {
  unsigned int BytesRead = 0;

  ASSERT(NULL != pQ);
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
      memcpy(&pRdBuf[BytesRead], &pQ->pfBuf[nRdIdx], Bytes * sizeof(bq_t));

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
unsigned int ByteQPokeRandom(ByteQ_t *const pQ, bq_t *pWrBuf, unsigned int bytesFromStart,
                       unsigned int nLen) {
  unsigned int BytesWritten = 0;

  ASSERT(NULL != pQ);
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
      memcpy(&pQ->pfBuf[nWrIdx], &pWrBuf[BytesWritten], Bytes * sizeof(bq_t));

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
int ByteQDoReadFromEnd(ByteQ_t *const pQ, bq_t *pRdBuf, int nLen) {
  int shortsRead = 0;

  if (nLen > 0) {
    // Calculate how many shorts can be read from the RdBuffer.
    int shortsToRead = 0;
    int nRdIdx = pQ->nWrIdx - nLen;
    if (nRdIdx < 0) {
      nRdIdx += pQ->nBufSz;
    }

    shortsToRead = nLen;

    // We can definitely read ShortsToRead shorts.
    while (shortsToRead > 0) {
      // Calculate how many contiguous shorts to the end of the buffer
      int Shorts = MIN(shortsToRead, (int)((pQ->nBufSz - nRdIdx)));

      // Copy that many shorts.
      memcpy(&pRdBuf[shortsRead], &pQ->pfBuf[nRdIdx], Shorts * sizeof(bq_t));

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

#endif // #ifdef COMPLEX_QUEUE

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
bool ByteQCreate(ByteQ_t *const pQ, bq_t *pBuf, unsigned int nBufSz,
                   bool lockOnWrites, bool lockOnReads) {

  ASSERT((NULL != pQ) && (NULL != pBuf));

  memset(pQ, 0, sizeof(ByteQ_t));

  pQ->pfBuf = pBuf;
  pQ->nBufSz = nBufSz;

  pQ->wrCntProt = lockOnWrites;
  pQ->rdCntProt = lockOnReads;

  return true;
}
