#ifndef BLE_RX_DUMMY_HPP
#define BLE_RX_DUMMY_HPP
/**
* COPYRIGHT    (c)	Applicaudia 2018
* @file        ble_rx_no_fragmentation.hpp
* @brief       Dummy transport layer for debug purposes
*/

#include "transport/ble_rx_reassemble_base.hpp"

#ifdef __cplusplus

class BleRxDummy : public BleRxBase {
public:
  // Constructor
  BleRxDummy(OnBleRxCallback cb, void *pData) 
    : BleRxBase()
    , mCb(cb)
    , mpCbData(pData)
  {
  }

  // Destructor
  virtual void dtor() override {}

  // Destructor
  virtual ~BleRxDummy() {
    dtor();
  }

  // The physical BLE layer shall call OnBlePhysRx() when packets arrive via BLE.
  virtual void OnBlePhysRx(
    const uint8_t * const pMsg,
    const size_t sz) override {
    if (mCb) {
      mCb(mpCbData, pMsg, sz);
    }
  }

private:
  OnBleRxCallback mCb;
  void * mpCbData;

};

#endif // __cplusplus

#endif // BLE_TX_DUMMY_HPP
