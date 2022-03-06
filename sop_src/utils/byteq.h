#ifndef __ByteQ
#define __ByteQ

/******************************************************************************
  Copyright 2014 Chris Fogelklou, Applaud Apps (Applicaudia)

  This source code may under NO circumstances be distributed
without the express written permission of the author.

  @author: chris.fogelklou@gmail.com
*******************************************************************************/

#include <stdint.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef uint8_t bq_t;

typedef struct _ByteQ_t {
  bq_t* pfBuf;
  unsigned int nWrIdx;
  unsigned int nRdIdx;
  unsigned int nCount;
  unsigned int nBufSz;
  bool rdCntProt;
  bool wrCntProt;
} ByteQ_t;


#ifdef __cplusplus
extern "C" {
#endif

/** [Declaration] Initialize a queue */
bool ByteQCreate(
  ByteQ_t* const pQ, bq_t* pBuf, unsigned int nBufSz,
  bool lockOnWrites, bool lockOnReads);

/** [Declaration] Destroy a queue */
bool ByteQDestroy(ByteQ_t* const pQ);

/** [Declaration] Write to a queue */
unsigned int
  ByteQWrite(ByteQ_t* const pQ, const bq_t* pWrBuf, unsigned int nLen);

/** [Declaration] Generic queue read function */
unsigned int ByteQRead(ByteQ_t* const pQ, bq_t* pRdBuf, unsigned int nLen);

/** [Declaration] Generic queue function for retrieving the
 * amount of space in the buffer. */
unsigned int ByteQGetWriteReady(ByteQ_t* const pQ);

/** [Declaration] Generic queue function for retrieving the
 * amount of space in the buffer. */
unsigned int ByteQGetContiguousWriteReady(ByteQ_t* const pQ);

/** [Declaration] Generic queue function for retrieving the
 * number of bytes that can be read from the buffer. */
unsigned int ByteQGetReadReady(ByteQ_t* const pQ);

/** [Declaration] Generic queue function for retrieving the
 * number of bytes that can be read from the buffer. */
unsigned int ByteQGetContiguousReadReady(ByteQ_t* const pQ);

/** [Declaration] Flushes the buffer. */
void ByteQFlush(ByteQ_t* const pQ);

/** [Declaration] Generic queue peek function */
unsigned int ByteQPeek(ByteQ_t* const pObj, bq_t* pRdBuf, unsigned int nLen);

/** [Declaration] Commit an external write to the queue
 * (simply increments the pointers and counters) */
unsigned int ByteQCommitWrite(ByteQ_t* const pQ, unsigned int nLen);

/** [Declaration] Commit an external read to the queue
 * (simply increments the pointers and counters) */
unsigned int ByteQCommitRead(ByteQ_t* const pQ, unsigned int nLen);

/** [Declaration] Gets the write pointer */
void* ByteQGetWritePtr(ByteQ_t* const pQ);

/** [Declaration] Gets the read pointer */
void* ByteQGetReadPtr(ByteQ_t* const pQ);

/** [Declaration] Sets the read index based on a pointer
 * into the internal buffer. */
void ByteQSetRdIdxFromPointer(ByteQ_t* const pQ, void* pRdPtr);

/** [Declaration] "Unreads" some data */
unsigned int ByteQUnread(ByteQ_t* const pQ, unsigned int nLen);

/** [Declaration] Forces the write of some data.  Advances the read pointer
in the case that the buffer would have filled. */
unsigned int
  ByteQForceWrite(ByteQ_t* const pQ, const bq_t* const pWrBuf, unsigned int nLen);

/** [Declaration] Unprotected (no mutex) fast write of data. */
unsigned int ByteQForceWriteUnprotected(
  ByteQ_t* const pQ, const bq_t* const pWrBuf, const int nLen);

unsigned int ByteQForceCommitWrite(ByteQ_t* const pQ, unsigned int nLen);

/** [Declaration] Peeks at data that resides somewhere inside the buffer */
unsigned int ByteQPeekRandom(
  ByteQ_t* const pQ, bq_t* pRdBuf,
  unsigned int bytesFromStart, unsigned int nLen);

/** [Declaration] Inserts data somewhere into the buffer */
unsigned int ByteQPokeRandom(
  ByteQ_t* const pQ, bq_t* pWrBuf,
  unsigned int bytesFromStart, unsigned int nLen);

/** [Declaration] Reads the last nLen words from the buffer */
int ByteQDoReadFromEnd(ByteQ_t* const pQ, bq_t* pRdBuf, int nLen);

#ifdef __cplusplus
}
#endif

#endif
