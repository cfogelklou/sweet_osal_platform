#ifndef BLE_TX_DUMMY_HPP
#define BLE_TX_DUMMY_HPP
/**
* COPYRIGHT    (c)	Applicaudia 2018
* @file        ble_tx_dummy.hpp
* @brief       Dummy transport layer for debug purposes.s
*/

#include "transport/ble_tx_fragmentation_base.hpp"

#ifdef __cplusplus

class BleTxDummy : public BleTxBase {
public:

  // Constructor
  BleTxDummy(
    BufIOQueue &physicalLayer
  )
    : BleTxBase()
    , mPhy(physicalLayer)
  {}

  virtual void dtor() override {}

  // Destructor
  virtual ~BleTxDummy() {
    dtor();
  }

  // Queues a write to the interface.
  virtual bool QueueWrite(
    BufIOQTransT *const pTrans,
    const uint32_t timeout = OSAL_WAIT_INFINITE
  ) override {
    return mPhy.QueueWrite(pTrans, timeout);
  }
private:
  BufIOQueue &mPhy;

};

#endif // __cplusplus

#endif // BLE_TX_NO_FRAGMENTATION_HPP
