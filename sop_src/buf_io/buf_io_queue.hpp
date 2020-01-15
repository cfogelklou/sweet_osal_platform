/**
* COPYRIGHT	(c)	Applicaudia 2017
* @file        buf_io_queue.hpp
* 
* This module serves as a superset of functionality for BufIO.
* Queued Read/Write API for issueing queued write commands of pre-allocated buffers,
* and queued read commands of pre-allocated buffers.
* A callback event is triggered after the queued write / queued read is completed.
* Operationally, it should operate much like DMA does.  The caller specifies
*  a block to transfer, and that transfer is queued to be transferred.  Once the
* block is queued, it is owned by the BufIOQueue, and should not be accessed until the
* completion callback is called.
* 
* When the transfer is completed or times out, a callback attached to the
* transfer is called, allowing the application to do whatever it needs to do upon 
* the completion of a transfer.
*
* Drivers implementing BufIOQueue can make use of the base class functionality
* to simplify their own code.  Calling applications will be reliant on the
* completion callback, so ensure that it is called in case of completion, error, timeout,
* etc, to drive events forward.
*/

#ifndef BLEIO_QUEUE_HPP
#define BLEIO_QUEUE_HPP

#include "utils/sl_list.h"
#include <stdint.h>
#include <stddef.h>

struct BufIOQTransTag;

// ////////////////////////////////////////////////////////////////////////////
// Callback called when a transaction is completed.
// Called by the physical layer (UART/BLE/etc) when a read or write transaction
// is finished.  Gives ownership of the transaction back to the caller.
// @param pTransaction: The transaction that has been completed.

typedef void (*BufIOQueue_TransactionCompleteCb)(
    struct BufIOQTransTag *pTransaction);

// ////////////////////////////////////////////////////////////////////////////
// Allows for transactions to be queued.  The completedCb will be called when
// the transaction is completed.
// Once a transaction is queued, it is owned by the queue until passed back in
// the TranasactionCompleteCb.
typedef struct BufIOQTransTag {

  /// Allows this structure to be inserted in a list inside the queue.
  SLLNode listNode;
  
  /// This private field is initialized by QueueRead() or QueueWrite()
  uint32_t expiryTime;
  
  /// Pointer to the buffer to send.  Must remain "allocated" until the
  /// completion callback is called.
  uint8_t *pBuf8;
  
  /// Lenth of the transaction, in bytes.
  uint16_t transactionLen;
  
  /// Progress fof the transaction.  If the completedCb is called and
  /// transferredIdx < transactionLen, this could indicate a
  /// smalle-than-expected packet was received or that the transaction has timed
  /// out.
  uint16_t transferredIdx;
  
  /// Callback to call when the transaction is completed
  BufIOQueue_TransactionCompleteCb completedCb;
  
  /// Data included to pass back to the completedCb.
  void *pUserData;
} BufIOQTransT;

// Utility class that makes allocating memory with the buffer appended easier.
// Just do
//  const int payloadSize = 200;
//  BufIOQTransWithPayloadT *p =
//      (BufIOQTransWithPayloadT *)OSALMALLOC(sizeof(BufIOQTransT) + payloadSize);
//  BufIOQueue_InitTransaction(&p->trans, p->payload, payloadSize, onReadComplete,
//                             pThis);
//  pNode->d.pAppIf->QueueRead(&p->trans);
typedef struct BufIOQTransWithPayloadTag {
  BufIOQTransT trans;
  /// This payload size is never used, as all allocations are carefully made
  /// to be exactly the required size. However, Xcode runtime checks will trigger
  /// out of bounds warnings if the payload size here isn't made huge.
  uint8_t payload[8192];
}BufIOQTransWithPayloadT;



#ifdef __cplusplus
extern "C" {
#endif


// Initializes a BufIOQTransT object.
// @param pTransaction: Pointer to the transaction object to initialize.
// @param pBuf: Pointer to the buffer to send/receive.  MUST REMAIN IN MEMORY
// UNTIL THE CALLBACK IS CALLED!
// @param bufLen: Length of the buffer in pBuf.
// @param cb: Pointer to the callback to be called when the transaction is
// completed.
// @param pUserData: Pointer to some user defined data.
bool BufIOQueue_InitTransaction(BufIOQTransT *const pTransaction,
                                uint8_t *const pBuf, const int bufLen,
#ifdef __cplusplus
                                BufIOQueue_TransactionCompleteCb cb = NULL,
                                void *const pUserData = NULL
#else
                                BufIOQueue_TransactionCompleteCb cb,
                                void *const pUserData
#endif
);

#ifdef __cplusplus
}
#endif

// If bit 31 is 1, then the message never expires.
#define NO_EXPIRY_TIME_MASK (0x01u << 31)
#define EXPIRY_TIME_MASK (~NO_EXPIRY_TIME_MASK)

#ifdef __cplusplus
#include "buf_io.hpp"
#include "osal/osal.h"
#include "osal/cs_locker.hpp"
#include "utils/platform_log.h"
#include "utils/helper_macros.h"
#include "utils/sl_list.hpp"

#ifdef ccs
#define override
#endif

enum class CriticalSectionType {
  TaskCritical,
  Critical
};

#define BLEIOQUEUE_CALLBACK_DECLARATION(ClassName, MemberFunction) \
  static void MemberFunction##C(struct BufIOQTransTag *pTransaction){ \
    ((ClassName *)pTransaction->pUserData)->MemberFunction(pTransaction); \
  } \
  void MemberFunction(struct BufIOQTransTag *pTransaction)

///////////////////////////////////////////////////////////////////////////////
//
class BufIOQueue : public BufIO {

public:
  // //////////////////////////////////////////////////////////////////////////
  // Constructor for a BufIOQueue.
  // pHalNodeData is passed down all the way to the base classes, defined by the
  // application.
  BufIOQueue(CriticalSectionType criticalType = CriticalSectionType::TaskCritical);

  // //////////////////////////////////////////////////////////////////////////
  virtual ~BufIOQueue();

  // //////////////////////////////////////////////////////////////////////////
  // Adds an array of buffers which can be quickly allocated and initialized
  // using
  // AllocAndInitTransaction.  This avoids requiring mempools for queued
  // transactions.
  void AddFreeTransactions(BufIOQTransT arr[], const int arrLen);

  // //////////////////////////////////////////////////////////////////////////
  // Frees a completed transaction.  The pointer pMsg will be set to NULL upon
  // completion of the call to indicate that the pointer is owned by the
  // BufIOQueue.
  void FreeTransaction(BufIOQTransT *&pMsg);

  // //////////////////////////////////////////////////////////////////////////
  // Allocates a transaction from the internal free queue.
  BufIOQTransT *AllocTransaction();

  // //////////////////////////////////////////////////////////////////////////
  // Allocates a transfer, and initializes it if the allocation was successful.
  // @ref BufIOQueue_InitTransaction()
  BufIOQTransT *AllocAndInitTransaction(uint8_t *const pBuf, const int bufLen,
                                        BufIOQueue_TransactionCompleteCb cb,
                                        void *const pUserData);

  // //////////////////////////////////////////////////////////////////////////
  // Queries a transaction to see if it is expired.  Returns true if expired,
  // else
  // false.
  static bool TransactionIsExpired(BufIOQTransT *const pTrans,
                                   const uint32_t timeOrTicks = OSALGetMS());

  // //////////////////////////////////////////////////////////////////////////
  // Write function, returns amount written.  Does not trigger a callback as
  // it is typically implemented using a FIFO.
  virtual int Write(const uint8_t *const pBytes, const int numBytes) override;

  // //////////////////////////////////////////////////////////////////////////
  // Get Write Ready.  Returns amount that can be written.
  virtual int GetWriteReady() override;

  // //////////////////////////////////////////////////////////////////////////
  // Read function - returns amount available.  Typically uses an internal FIFO.
  virtual int Read(uint8_t *const pBytes, const int numBytes) override;

  // //////////////////////////////////////////////////////////////////////////
  // Get the number of bytes that can be read.
  virtual int GetReadReady() override;

  // //////////////////////////////////////////////////////////////////////////
  // Queues a write to the interface, typically using an internal queue.
  virtual bool QueueWrite(BufIOQTransT *const pTxTrans,
                          const uint32_t timeout = OSAL_WAIT_INFINITE);

  // //////////////////////////////////////////////////////////////////////////
  // Queues a read from the interface, typically using an internal queue.
  virtual bool QueueRead(BufIOQTransT *const pRxTrans,
                         const uint32_t timeout = OSAL_WAIT_INFINITE);

  // //////////////////////////////////////////////////////////////////////////
  // Locks the object so only one thread can use it at a time.
  virtual void LockResource();

  // //////////////////////////////////////////////////////////////////////////
  // Unlocks the object, so other threads can use it.
  virtual void UnlockResource();

  // //////////////////////////////////////////////////////////////////////////
  // Purges all expired TX messages (or all TX messages if purgeAll is true)
  // from the transmit queue.  Calls the callback for all purged messages so
  // The calling application can
  void PurgeOldTxMessages(bool purgeAll = false);

  // //////////////////////////////////////////////////////////////////////////
  // Purges all expired RX messages (or all RX messages if purgeAll is true)
  // from the receive queue.
  void PurgeOldRxMessages(bool purgeAll = false);

  // //////////////////////////////////////////////////////////////////////////
  // Flushes the active TX transaction, pointed to by mpTxCurr, and calls the
  // attached callback.  Does not start a new transaction, however; it is up
  // to the calling class to call LoadQueuedTxTransaction to start the next
  // transaction.
  bool FlushActiveTxTransaction();

  // //////////////////////////////////////////////////////////////////////////
  // Flushes the active RX transaction, pointed to by mpRxCurr, and calls the
  // attached callback.  Does not start a new transaction, however; it is up
  // to the calling class to call LoadQueuedRxTransaction to start the next
  // transaction.
  bool FlushActiveRxTransaction();

protected:
  // //////////////////////////////////////////////////////////////////////////
  // Loads the next transaction or remainder of the current transaction.
  // If mpTxCurr (the current transaction) is not complete yet, then
  // pTxBuf will be loaded with a pointer to where the current transaction
  // shall continue from, and len with the number of remaining buts.
  // If mpTxCurr is NULL, then it will be loaded with the contents of the next
  // queued transaction.
  BufIOQTransT * LoadQueuedTxTransaction(const uint8_t *&pTxBuf, int &len);

  // //////////////////////////////////////////////////////////////////////////
  // Loads the next transaction or remainder of the current transaction.
  // If mpRxCurr (the current transaction) is not complete yet, then
  // pRxBuf will be loaded with a pointer to where the current transaction
  // shall continue from, and len with the number of remaining buts.
  // If mpRxCurr is NULL, then it will be loaded with the contents of the next
  // queued transaction.
  BufIOQTransT * LoadQueuedRxTransaction(uint8_t *&pRxBuf, int &len);

  // //////////////////////////////////////////////////////////////////////////
  // Commits part of a completed transaction.
  // This increments the transferredIdx index in the current transaction, clears
  // the current transaction IFF it is completed, and calls the callback if
  // callCallbackIfCompleted is true.
  // Returns true if the transaction was complete (in which case the caller
  // should call the
  // transaction's callback function if callCallbackIfCompleted was false.)
  bool CommitTxTransaction(const int len, const bool callCallbackIfCompleted);

  // //////////////////////////////////////////////////////////////////////////
  // Commits part of a completed transaction.
  // This increments the transferredIdx index in the current transaction, clears
  // the current transaction IFF it is completed, and calls the callback if
  // callCallbackIfCompleted is true.
  // Returns true if the transaction was complete (in which case the caller
  // should call the
  // transaction's callback function if callCallbackIfCompleted was false.)
  bool CommitRxTransaction(const int len, const bool callCallbackIfCompleted,
                           const bool forceCompleted = false);

  typedef enum {
    RX = false,
    TX = true,
  } Dir;

  // //////////////////////////////////////////////////////////////////////////
  bool CommitTransaction(const Dir dir, const int len,
                         const bool callCallbackIfCompleted,
                         const bool forceCompleted = false);

  // //////////////////////////////////////////////////////////////////////////
  BufIOQTransT * LoadNextTransaction(const Dir dir, uint8_t **ppTxBuf, int &len);

  // //////////////////////////////////////////////////////////////////////////
  void SetCurrentTxPtr(BufIOQTransT * const pTrans);

  // //////////////////////////////////////////////////////////////////////////
  // Gets the current transmit pointer.
  BufIOQTransT * GetCurrentTxPtr();

  // //////////////////////////////////////////////////////////////////////////
  void SetCurrentRxPtr(BufIOQTransT * const pTrans);

  // //////////////////////////////////////////////////////////////////////////
  BufIOQTransT * GetCurrentRxPtr();

private:
  // //////////////////////////////////////////////////////////////////////////
  // Go through all old messages and purge any which have a time older than the
  // expiry time.  The list "purged" receives the messages that have been
  // purged.
  // The calling (inheriting) class should then call the callback on purged.
  void PurgeOldMessages(bool purgeAll, sll::list &list, sll::list &purged);

  // //////////////////////////////////////////////////////////////////////////
  // Cancels the current transaction and clears pCurr.
  // Be careful to never do this if the inheriting class has a task scheduled
  // which relies on mpXXCurr being non-null!
  bool FlushActiveTransaction(const Dir dir);

  // //////////////////////////////////////////////////////////////////////////
  // Queues a transaction, used internally.
  bool Queue(const Dir dir, BufIOQTransT *const pTxTrans,
             const uint32_t timeout);


protected:
  /// Transmission queue.  mpTxCurr points to the active transmission
  BufIOQTransT *mpTxCurr;

  /// Transmission queue.  mTxQueue contains queued transactions.
  sll::list mTxQueue;

  /// Reception queue.  mpRxCurr points to the active reception
  BufIOQTransT *mpRxCurr;
  /// Reception queue.  mRxQueue contains queued transactions.
  sll::list mRxQueue;

  // Used only if the application wants to add some free transfers before
  // calling the Alloc() functions.
  sll::list mFreeQueue;
  
  // Ensures that no functions are called after destruction.
  uint32_t mIsValidTag;

  // Enter/ExitCritical
  typedef void (*voidFunc)(void);
  const voidFunc mpEnterCritical;
  const voidFunc mpExitCritical;
};

// Malloc's a transaction with payload.
// If no payload is specified, leaves it uninitialized (used for RX transactions.)
// If no cb is specifies, uses an internal callback that simply frees the transaction
// when it has completed (set and forget.)
BufIOQTransWithPayloadT * BufIOQueue_MallocWithPayload(
  const int numPayloadBytes, 
  const void *const pPayload = NULL,
  BufIOQueue_TransactionCompleteCb cb = NULL,
  void * const pUserData = NULL
);

// Utility class which calls LockResource() in the constructor, and UnlockResource() 
// in the destructor. Use to prevent forgetting to unlock a locked resource.
class BufIOQueueLocker {
public:
  // Construct the locker on the stack, with a pointer to the resource to lock.
  BufIOQueueLocker(BufIOQueue * const pQ);

  // Destrct the locker, unlocking the resource.
  ~BufIOQueueLocker();
private:
  BufIOQueue * const mpQ;
};

#endif

#endif
