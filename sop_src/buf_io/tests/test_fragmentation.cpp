/**
 * COPYRIGHT    (c)	Applicaudia 2018
 * @file        tone_ble_crypto.cpp
 * @brief       Test of BLE Cryptography Primitives.
 */

#include "gtest/gtest.h"
#include "tests/gtest_test_wrapper.hpp"
#include "transport/ble_rx_reassemble.h"
#include "transport/ble_tx_fragmentation.h"
#include <string>

LOG_MODNAME("test_fragmentation.cpp")

class TestCTransport : public GtestMempoolsWrapper {
protected:
  TestCTransport() {
#ifdef WIN32
    OSALMSHookToHardware(false);
#endif
  }
  virtual ~TestCTransport() {
#ifdef WIN32
    OSALMSHookToHardware(true);
#endif
  }
};

// ////////////////////////////////////////////////////////////////////////////
TEST_F(TestCTransport, test_C_seqnum) {
  EXPECT_EQ(0, BleFragGetFirstSeqMinusSecond(0, 0));
  EXPECT_EQ(4, BleFragGetFirstSeqMinusSecond(4, 0));
  EXPECT_EQ(7, BleFragGetFirstSeqMinusSecond(7, 0));
  EXPECT_EQ(-1, BleFragGetFirstSeqMinusSecond(BLESN_SEQNUM_RANGE-1, 0));
  EXPECT_EQ(-4, BleFragGetFirstSeqMinusSecond(BLESN_SEQNUM_RANGE-4, 0));
  EXPECT_EQ(-7, BleFragGetFirstSeqMinusSecond(BLESN_SEQNUM_RANGE-7, 0));

  EXPECT_EQ(0, BleFragGetFirstSeqMinusSecond(4, 4));
  EXPECT_EQ(4, BleFragGetFirstSeqMinusSecond(8, 4));
  EXPECT_EQ(7, BleFragGetFirstSeqMinusSecond(11, 4));
  EXPECT_EQ(-1, BleFragGetFirstSeqMinusSecond(3, 4));
  EXPECT_EQ(-4, BleFragGetFirstSeqMinusSecond(0, 4));
  EXPECT_EQ(-7, BleFragGetFirstSeqMinusSecond(BLESN_SEQNUM_RANGE-3, 4));

  EXPECT_EQ(0, BleFragGetFirstSeqMinusSecond(8, 8));
  EXPECT_EQ(4, BleFragGetFirstSeqMinusSecond(12, 8));
  EXPECT_EQ(7, BleFragGetFirstSeqMinusSecond(15, 8));
  EXPECT_EQ(-1, BleFragGetFirstSeqMinusSecond(7, 8));
  EXPECT_EQ(-4, BleFragGetFirstSeqMinusSecond(4, 8));
  EXPECT_EQ(-7, BleFragGetFirstSeqMinusSecond(1, 8));

  EXPECT_EQ(0, BleFragGetFirstSeqMinusSecond(12, 12));
  EXPECT_EQ(4, BleFragGetFirstSeqMinusSecond(0, 12));
  EXPECT_EQ(7, BleFragGetFirstSeqMinusSecond(3, 12));
  EXPECT_EQ(-1, BleFragGetFirstSeqMinusSecond(11, 12));
  EXPECT_EQ(-4, BleFragGetFirstSeqMinusSecond(8, 12));
  EXPECT_EQ(-7, BleFragGetFirstSeqMinusSecond(5, 12));

}

// ////////////////////////////////////////////////////////////////////////////
TEST_F(TestCTransport, test_C_BleFragmenter1) {
  BleFragmenter tx = { 0 };
  static const int PKT_SIZE = 6;
  BleFragmenterInit(&tx, PKT_SIZE+1, 0);
  const char sendString[21] = "1234567890abcdefghij";
  char receiveString[21] = { 0 };
  
  // Ensure that fragments are put together correctly.
  auto fn = [](
    void * pContext,
    const BleFragment * const pPkt,
    const size_t payloadSize
    ) 
  {
    char *pOutString = (char *)pContext;
    int seqNr = BleFragGetSeqNum(pPkt);
    int offset = seqNr * payloadSize;

    memcpy(&pOutString[offset], pPkt->payload, payloadSize);

    if (0 == offset) {
      EXPECT_TRUE(BleFragIsStartFragment(pPkt));
      EXPECT_FALSE(BleFragIsStopFragment(pPkt));
    }
    else if (15 == offset) {
      EXPECT_FALSE(BleFragIsStartFragment(pPkt));
      EXPECT_TRUE(BleFragIsStopFragment(pPkt));
    }
    else {
      EXPECT_FALSE(BleFragIsStartFragment(pPkt));
      EXPECT_FALSE(BleFragIsStopFragment(pPkt));
    }
  };
  
  BleFragmentize(&tx, (uint8_t *)sendString, sizeof(sendString)-1, fn, receiveString);
  EXPECT_EQ(0, memcmp(sendString, receiveString, sizeof(sendString)));
}

static void test_DummyRxOverride(
  void *pContext,
  const BleReassembleError err,
  const BleReassembleErrorData * const pErrData
) {
  (void)pContext;
  (void)err;
  (void)pErrData;
}

// ////////////////////////////////////////////////////////////////////////////
TEST_F(TestCTransport, test_C_BleReassembler0) {
  BleFragmenter tx = { 0 };
  BleReassembler rx = { 0 };
  static const int PKT_SIZE = 6;
  BleFragmenterInit(&tx, PKT_SIZE + 1, 0);
  const char sendString[21] = "1234567890abcdefghij";
  char receiveString[21] = { 0 };

  /// Callback function called when a whole BLE/DTLS message has been received.
  auto rxFn = [](
    void * pContext,
    const uint8_t * const pPkt,
    const size_t packetSize
    ) {
    EXPECT_EQ(pContext, pPkt);
  };

  BleReassembleInit(&rx, (uint8_t *)receiveString, sizeof(receiveString), rxFn, NULL, receiveString);
  BleReassembleSetOverride(&rx, test_DummyRxOverride, nullptr);

    
    // Ensure that fragments are put together correctly.
  auto txFn = [](
    void * pContext,
    const BleFragment * const pPkt,
    const size_t payloadSize
    )
  {
    BleReassembler * const pRx = (BleReassembler *)pContext;
    BleReassembleFragment(pRx, &pPkt->hdr.flags, payloadSize + sizeof(pPkt->hdr));
  };

  BleFragmentize(&tx, (uint8_t *)sendString, sizeof(sendString) - 1, txFn, &rx);
  EXPECT_EQ(0, memcmp(sendString, receiveString, sizeof(sendString)));
}



// ////////////////////////////////////////////////////////////////////////////
TEST_F(TestCTransport, test_C_BleReassembler1) {

  BleReassembler rx = { 0 };
  uint8_t rxBuf[256];
  std::string received;

  uint8_t pkt0[] = { BLEF_START_MASK, 0, 'h' };
  uint8_t pkt1[] = { 0,               1, 'i' };
  uint8_t pkt2[] = { BLEF_STOP_MASK,  2, '!' };

  /// Callback function called when a whole BLE/DTLS message has been received.
  auto rxFn = [](
    void * pContext,
    const uint8_t * const pPkt,
    const size_t packetSize
    ) {
    std::string &received = *(std::string *)pContext;    
    received.append((const char *)pPkt, packetSize);
  };

  BleReassembleInit(&rx, rxBuf, sizeof(rxBuf), rxFn, NULL, &received);
  BleReassembleSetOverride(&rx, test_DummyRxOverride, nullptr);

  // Send stop packet. Shall not receive anything.
  BleReassembleFragment(&rx, pkt2, sizeof(pkt2));
  EXPECT_EQ(received.length(), 0);

  // Send first and second, but skip stop
  BleReassembleFragment(&rx, pkt0, sizeof(pkt0));
  BleReassembleFragment(&rx, pkt1, sizeof(pkt1));
  EXPECT_EQ(received.length(), 0);

  // Send first and stop, but skip middle.  No receive triggered.
  BleReassembleFragment(&rx, pkt0, sizeof(pkt0));
  BleReassembleFragment(&rx, pkt2, sizeof(pkt2));
  EXPECT_EQ(received.length(), 0);

  // Send all 3, will trigger receive.
  BleReassembleFragment(&rx, pkt0, sizeof(pkt0));
  EXPECT_EQ(received.length(), 0);
  BleReassembleFragment(&rx, pkt1, sizeof(pkt1));
  EXPECT_EQ(received.length(), 0);
  BleReassembleFragment(&rx, pkt2, sizeof(pkt2));
  EXPECT_EQ(received.length(), 3);

  // Check that the packet was received.
  const uint8_t expectedRxString[] = { 'h','i', '!' };
  EXPECT_EQ(0, memcmp(expectedRxString, received.c_str(), sizeof(expectedRxString)));
  received.clear();

  // Send just stop, shall not trigger receive.
  BleReassembleFragment(&rx, pkt2, sizeof(pkt2));
  EXPECT_EQ(received.length(), 0);

  // Send all 3, will trigger receive.
  BleReassembleFragment(&rx, pkt0, sizeof(pkt0));
  EXPECT_EQ(received.length(), 0);
  BleReassembleFragment(&rx, pkt1, sizeof(pkt1));
  EXPECT_EQ(received.length(), 0);
  BleReassembleFragment(&rx, pkt2, sizeof(pkt2));
  EXPECT_EQ(received.length(), 3);

  // Check that the packet was received.
  EXPECT_EQ(0, memcmp(expectedRxString, received.c_str(), sizeof(expectedRxString)));
  received.clear();

}
