/**
* COPYRIGHT	(c)	Applicaudia 2018
* @file        buf_io_queue.cpp
* @brief       See header file.
*/
#include "buf_io_queue.hpp"
#include "osal/osal.h"
#include "utils/dl_list.h"
#include <string.h>
LOG_MODNAME("buf_io_queue")

extern "C" {

// //////////////////////////////////////////////////////////////////////////
void buf_ioqueue_tx_free(BufIOQTransT * const pTrans) {
  OSALFREE(pTrans);
}

// //////////////////////////////////////////////////////////////////////////
bool BufIOQueue_InitTransaction(BufIOQTransT *const pTransaction,
                                uint8_t *const pBuf, const int bufLen,
                                BufIOQueue_TransactionCompleteCb cb,
                                void *const pUserData) {
  bool rval = false;
  if ((pTransaction) && (bufLen < 65535)) {
    memset(pTransaction, 0, sizeof(*pTransaction));
    SLL_NodeInit(&pTransaction->listNode);
    pTransaction->pBuf8 = pBuf;
    pTransaction->transactionLen = bufLen;
    pTransaction->completedCb = (cb) ? cb : buf_ioqueue_tx_free;
    pTransaction->expiryTime = OSAL_WAIT_INFINITE;
    pTransaction->pUserData = pUserData;
    rval = true;
  }
  return rval;
}

} // extern "C" {



// //////////////////////////////////////////////////////////////////////////
BufIOQTransWithPayloadT * BufIOQueue_MallocWithPayload(
  const int numPayloadBytes, const void *const pPayload,
  BufIOQueue_TransactionCompleteCb cb,
  void * const pUserData
) {
  BufIOQTransWithPayloadT * const p =
     (BufIOQTransWithPayloadT *)OSALMALLOC(sizeof(BufIOQTransT) + numPayloadBytes);
  if (p){
    const BufIOQueue_TransactionCompleteCb fn = (cb) ? cb : buf_ioqueue_tx_free;
    BufIOQueue_InitTransaction(&p->trans, p->payload, numPayloadBytes, fn, pUserData);
    if (pPayload) {
      memcpy(p->payload, pPayload, numPayloadBytes);
    }
  }
  return p;
}

// //////////////////////////////////////////////////////////////////////////
BufIOQueue::BufIOQueue(CriticalSectionType criticalType)
  : BufIO()
  , mpTxCurr(NULL)
  , mTxQueue()
  , mpRxCurr(NULL)
  , mRxQueue()
  , mFreeQueue()
  , mIsValidTag(0x87654321)
  , mpEnterCritical((criticalType==CriticalSectionType::TaskCritical)?OSALEnterTaskCritical:OSALEnterCritical)
  , mpExitCritical((criticalType==CriticalSectionType::TaskCritical)?OSALExitTaskCritical:OSALExitCritical)
{


}

// //////////////////////////////////////////////////////////////////////////
BufIOQueue::~BufIOQueue() {
  mIsValidTag = 0;
}

// //////////////////////////////////////////////////////////////////////////
void BufIOQueue::AddFreeTransactions(BufIOQTransT arr[], const int arrLen) {
  for (int i = 0; i < arrLen; i++) {
    mFreeQueue.push_back(&arr[i].listNode);
  }
}

// //////////////////////////////////////////////////////////////////////////
void BufIOQueue::FreeTransaction(BufIOQTransT *&pMsg) {
  if (0x87654321 != mIsValidTag) return;
  CSLocker lock;
  mFreeQueue.push_back(&pMsg->listNode);
  pMsg = NULL;
}

// //////////////////////////////////////////////////////////////////////////
BufIOQTransT *BufIOQueue::AllocTransaction() {
  if (0x87654321 != mIsValidTag) return nullptr;
  CSLocker lock;
  return (BufIOQTransT *)mFreeQueue.pop_front();
}

// //////////////////////////////////////////////////////////////////////////
BufIOQTransT *BufIOQueue::AllocAndInitTransaction(
    uint8_t *const pBuf, const int bufLen, BufIOQueue_TransactionCompleteCb cb,
    void *const pUserData) {
  if (0x87654321 != mIsValidTag) return nullptr;
  BufIOQTransT *const pTrans = AllocTransaction();
  if (pTrans) {
    BufIOQueue_InitTransaction(pTrans, pBuf, bufLen, cb, pUserData );
  }
  return pTrans;
}

// //////////////////////////////////////////////////////////////////////////
bool BufIOQueue::TransactionIsExpired(
    BufIOQTransT *const pTrans,
    const uint32_t timeOrTicks) {
  bool rval = false;
  if (pTrans){
    const uint32_t time = EXPIRY_TIME_MASK & timeOrTicks;
    if (!(NO_EXPIRY_TIME_MASK & pTrans->expiryTime)) {

      const uint32_t timeDiff = (((uint32_t)pTrans->expiryTime - (uint32_t)time)
                                 << 1u); // MSB is now sign bit.

      // Interpret as int32 now to make cmp easy.
      const int32_t sTimeDiff = (int32_t)timeDiff;
      rval = (sTimeDiff < 0);
    }
  }
  return rval;
}



// //////////////////////////////////////////////////////////////////////////
// Queues a write to the interface, typically using an internal queue.
bool BufIOQueue::QueueWrite(BufIOQTransT *const pTxTrans,
                            const uint32_t timeout) {
  if (0x87654321 != mIsValidTag) return false;
  return Queue(TX, pTxTrans, timeout);
}

// //////////////////////////////////////////////////////////////////////////
// Queues a write to the interface, typically using an internal queue.
bool BufIOQueue::QueueRead(BufIOQTransT *const pRxTrans,
                           const uint32_t timeout) {
  if (0x87654321 != mIsValidTag) return false;
  return Queue(RX, pRxTrans, timeout);
}

// //////////////////////////////////////////////////////////////////////////
// Locks the object
void BufIOQueue::LockResource() {
  OSALEnterTaskCritical();
}

// //////////////////////////////////////////////////////////////////////////
// Unlocks the object
void BufIOQueue::UnlockResource() {
  OSALExitTaskCritical();
}

// //////////////////////////////////////////////////////////////////////////
// Supports the Bleio API by allocating a transaction and freeing it when complete.
typedef struct BufIOQueueDummyTransTag {
  BufIOQTransT trans;
  uint8_t payload[1];
} BufIOQueueTransT;


// //////////////////////////////////////////////////////////////////////////
// Write function, returns amount written.  Typically uses a FIFO.
int BufIOQueue::Write(const uint8_t *const pBytes, const int numBytes){
  if (0x87654321 != mIsValidTag) return 0;
  int rval = 0;
  if ((pBytes) && (numBytes > 0)) {
    BufIOQTransWithPayloadT * const pTrans = BufIOQueue_MallocWithPayload(numBytes, pBytes);
    if (pTrans) {
      if (QueueWrite(&pTrans->trans)) {
        rval = numBytes;
      }
      else {
        OSALFREE(pTrans);
      }
    }
  }
  return rval;
}

  // //////////////////////////////////////////////////////////////////////////
  // Get Write Ready.  Returns amount that can be written.
int BufIOQueue::GetWriteReady(){
  return 2048;
}

  // //////////////////////////////////////////////////////////////////////////
  // Read function - returns amount available.  Typically uses an internal FIFO.
int BufIOQueue::Read(uint8_t *const pBytes, const int numBytes){
  (void)pBytes;
  (void)numBytes;
  return 0;
}

  // //////////////////////////////////////////////////////////////////////////
  // Get the number of bytes that can be read.
int BufIOQueue::GetReadReady(){
  return 0;
}

// //////////////////////////////////////////////////////////////////////////
// END public
// START protected
// //////////////////////////////////////////////////////////////////////////

// //////////////////////////////////////////////////////////////////////////
// Go through all old messages and purge any which have a time older than
// the expiry time.
void BufIOQueue::PurgeOldMessages(bool purgeAll, sll::list &list, sll::list &purged) {
  if (0x87654321 != mIsValidTag) return;
  sll::list tmp;
  const uint32_t time = EXPIRY_TIME_MASK & OSALGetMS();

  mpEnterCritical();
  SLL_AppendListToBack (&tmp.sll, &list.sll);
  SLLNode *pIter = tmp.pop_front();
  while (pIter) {
    BufIOQTransT *const pMsg = (BufIOQTransT *)pIter;
    if ((purgeAll) || (TransactionIsExpired(pMsg, time))) {
      purged.push_back(pIter);
      if (pMsg == this->mpRxCurr){
        LOG_ASSERT(false);
      }
      else if (pMsg == this->mpTxCurr){
        LOG_ASSERT(false);
      }
    }
    else {
      list.push_back(pIter);
    }
    pIter = tmp.pop_front();
  }
  mpExitCritical();
}

// //////////////////////////////////////////////////////////////////////////
static bool buf_ioq_PurgeOldMessagesCb(void *ParamPtr, SLLNode *pNode){
  BufIOQTransT *pTrans = (BufIOQTransT *)pNode;
  (void)ParamPtr;
  if (pTrans->completedCb) {
    pTrans->completedCb(pTrans);
  }
  return true;
}

// //////////////////////////////////////////////////////////////////////////
void BufIOQueue::PurgeOldTxMessages( bool purgeAll ) {
  if (0x87654321 != mIsValidTag) return;
  sll::list purged;
  PurgeOldMessages( purgeAll, mTxQueue, purged );
  if ((mpTxCurr) && ((purgeAll) || (TransactionIsExpired(mpTxCurr)))){
    BufIOQueue::FlushActiveTransaction(TX);
  }
  purged.for_each_pop(buf_ioq_PurgeOldMessagesCb, NULL );
}

// //////////////////////////////////////////////////////////////////////////
void BufIOQueue::PurgeOldRxMessages( bool purgeAll ){
  if (0x87654321 != mIsValidTag) return;
  sll::list purged;
  PurgeOldMessages( purgeAll, mRxQueue, purged );
  if ((mpRxCurr) && ((purgeAll) || (TransactionIsExpired(mpRxCurr)))){
    BufIOQueue::FlushActiveTransaction(RX);
  }
  if ((!purgeAll) && (!purged.isEmpty())){
    LOG_TRACE(("%s:Purged %d RX messages!\r\n", dbgModId, purged.count()));
  }
  purged.for_each_pop(buf_ioq_PurgeOldMessagesCb, NULL );
}

// //////////////////////////////////////////////////////////////////////////
bool BufIOQueue::FlushActiveTxTransaction(){
  return FlushActiveTransaction(TX);
}

// //////////////////////////////////////////////////////////////////////////
bool BufIOQueue::FlushActiveRxTransaction(){
  return FlushActiveTransaction(RX);
}


// //////////////////////////////////////////////////////////////////////////
BufIOQTransT * BufIOQueue::LoadQueuedTxTransaction(const uint8_t *&pTxBuf, int &len) {
  return LoadNextTransaction(TX, (uint8_t **)&pTxBuf, len);
}

// //////////////////////////////////////////////////////////////////////////
BufIOQTransT * BufIOQueue::LoadQueuedRxTransaction(uint8_t *&pRxBuf, int &len) {
  return LoadNextTransaction(RX, &pRxBuf, len);
}

// //////////////////////////////////////////////////////////////////////////
bool BufIOQueue::CommitTxTransaction(const int len,
                                     const bool callCallbackIfCompleted) {
  return CommitTransaction(TX, len, callCallbackIfCompleted);
}

// //////////////////////////////////////////////////////////////////////////
bool BufIOQueue::CommitRxTransaction(const int len,
                                     const bool callCallbackIfCompleted,
                                     const bool forceCompleted) {
  return CommitTransaction(RX, len, callCallbackIfCompleted, forceCompleted);
}

// //////////////////////////////////////////////////////////////////////////
// END protected
// START private
// //////////////////////////////////////////////////////////////////////////
bool BufIOQueue::Queue(const Dir dir, BufIOQTransT *const pTrans,
                       const uint32_t timeout) {
  if (0x87654321 != mIsValidTag) return false;
  bool rval = false;
  LOG_ASSERT(pTrans && pTrans->pBuf8);
  if (pTrans && pTrans->pBuf8 && pTrans->transactionLen > 0) {
    pTrans->expiryTime = ((0 == timeout) || (OSAL_WAIT_INFINITE == timeout))
                             ? OSAL_WAIT_INFINITE
                             : EXPIRY_TIME_MASK & (OSALGetMS() + timeout);
    sll::list &queue = (dir == TX) ? mTxQueue : mRxQueue;
    mpEnterCritical();
    queue.push_back(&pTrans->listNode);
    mpExitCritical();
    rval = true;
  }

  return rval;
}

// //////////////////////////////////////////////////////////////////////////
bool BufIOQueue::CommitTransaction(const Dir dir, const int len,
                                   const bool callCallbackIfCompleted,
                                   const bool forceCompleted) {
  if (0x87654321 != mIsValidTag) return false;
  bool doCallCallbacks = false;
  mpEnterCritical();
  BufIOQTransT *&pCurr = (dir == TX) ? mpTxCurr : mpRxCurr;
  if (NULL == pCurr) {
    mpExitCritical();
  }
  else {
    pCurr->transferredIdx += len;
    LOG_ASSERT(pCurr->transferredIdx <= pCurr->transactionLen);

    if (!((forceCompleted) || (pCurr->transferredIdx >= pCurr->transactionLen))) {
      mpExitCritical();
    }
    else {
      doCallCallbacks = true;
      BufIOQTransT *pC = pCurr;
      pCurr = NULL;
      mpExitCritical();

      // Call the callbacks if the implementation thinks it's OK to do that.
      if (callCallbackIfCompleted) {
        if (pC->completedCb) {
          pC->completedCb(pC);
        }
      }
    }
  }
  return doCallCallbacks;
}

// //////////////////////////////////////////////////////////////////////////
bool BufIOQueue::FlushActiveTransaction(const Dir dir ) {
  LOG_ASSERT(NULL != this);
  if (0x87654321 != mIsValidTag) return false;
  bool rval = false;
  mpEnterCritical();
  BufIOQTransT *&pCurr = (dir == TX) ? mpTxCurr : mpRxCurr;
  if (NULL == pCurr) {
    mpExitCritical();
  }
  else {
    BufIOQTransT *pC = pCurr;
    pCurr = NULL;
    mpExitCritical();
    LOG_ASSERT(pC->transferredIdx <= pC->transactionLen);
    rval = true;
    // Call the callbacks if the implementation thinks it's OK to do that.
    if (pC->completedCb) {
      pC->completedCb(pC);
    }
  }
  return rval;
}


// //////////////////////////////////////////////////////////////////////////
BufIOQTransT * BufIOQueue::LoadNextTransaction(const Dir dir, uint8_t **ppBuf,
                                    int &len) {
  BufIOQTransT * rval = NULL;
  if (0x87654321 != mIsValidTag) return nullptr;
  sll::list &queue = (dir == TX) ? mTxQueue : mRxQueue;
  mpEnterCritical();
  BufIOQTransT *&pCurr = (dir == TX) ? mpTxCurr : mpRxCurr;
  if (NULL == pCurr) {
    pCurr = (BufIOQTransT *)queue.pop_front();
    if (pCurr) {
      pCurr->transferredIdx = 0;
    }
  }
  if (NULL == pCurr) {
    mpExitCritical();
  }
  else {
    if (ppBuf) {
      *ppBuf = &pCurr->pBuf8[pCurr->transferredIdx];
    }
    LOG_ASSERT(pCurr->transferredIdx < pCurr->transactionLen);
    len = pCurr->transactionLen - pCurr->transferredIdx;
    LOG_ASSERT((len >= 0) && (len <= pCurr->transactionLen));
    rval = pCurr;
    mpExitCritical();
  }
  return rval;
}

  // //////////////////////////////////////////////////////////////////////////
  void BufIOQueue::SetCurrentTxPtr(BufIOQTransT * const pTrans){
    mpTxCurr = pTrans;
  }

  // //////////////////////////////////////////////////////////////////////////
  BufIOQTransT * BufIOQueue::GetCurrentTxPtr(){
    return mpTxCurr;
  }

  // //////////////////////////////////////////////////////////////////////////
  void BufIOQueue::SetCurrentRxPtr(BufIOQTransT * const pTrans){
    mpRxCurr = pTrans;
  }

  // //////////////////////////////////////////////////////////////////////////
  BufIOQTransT * BufIOQueue::GetCurrentRxPtr(){
    return mpRxCurr;
  }

  // //////////////////////////////////////////////////////////////////////////
  BufIOQueueLocker::BufIOQueueLocker(BufIOQueue * const pQ)
    : mpQ(pQ)
  {
    LOG_ASSERT(mpQ);
    mpQ->LockResource();
  }

  // //////////////////////////////////////////////////////////////////////////
  BufIOQueueLocker::~BufIOQueueLocker() {
    LOG_ASSERT(mpQ);
    mpQ->UnlockResource();
  }

// //////////////////////////////////////////////////////////////////////////
//  END private
// //////////////////////////////////////////////////////////////////////////
