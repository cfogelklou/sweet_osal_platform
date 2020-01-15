/**
 * COPYRIGHT    (c)	Applicaudia 2018
 * @file        tone_ble_crypto.cpp
 * @brief       Test of BLE Cryptography Primitives.
 */
#include <gtest/gtest.h>



#ifndef RUN_GTEST
#include <gmock/gmock.h>
using namespace testing;
#endif

#include "utils/byteq.h"
#include "gtest/gtest.h"
#include "tests/gtest_test_wrapper.hpp"
#include "transport/ble_rx_reassemble.h"
#include "transport/ble_tx_fragmentation.h"
#include "transport/ble_tx_fragmentation.hpp"
#include "transport/ble_rx_reassemble.hpp"
#include "crypto/mbed_prng_ctr.hpp"
#include "platform/cs_task_locker.hpp"
#include "utils/crc.h"

#define TRANS_TEST_POOL_ID 0x44

LOG_MODNAME("test_transport.cpp")

class TestTransport : public GtestMempoolsWrapper {
protected:
  TestTransport() {
#ifdef WIN32
    OSALMSHookToHardware(false);
#endif
  }
  virtual ~TestTransport() {
#ifdef WIN32
    OSALMSHookToHardware(true);
#endif
  }
};





class RngSingleton {
public:
  static RngSingleton & inst() {
    static RngSingleton rng;
    return rng;
  }

  int Read(uint8_t *const pBytes, const int numBytes) {
    return mRng.Read(pBytes, numBytes);
  }

private:
  RngSingleton()
    : mRng()
  {}

private:
  PrngCtr mRng;
};


class BleIoBridge : public BufIOQueue {
public:
  // //////////////////////////////////////////////////////////////////////////
  BleIoBridge(
    bool ignoreWrites = false,
    BleRx *pDstIo = NULL, 
    float lossRatio = 0,
    int writeDelayMs = 10,
    const char name[] = NULL,
    const char *pubKey = NULL,
    const int pubKeyLen = 0
  ) 
    : mIgnoreWrites(ignoreWrites)
    , mLossRatio(lossRatio)
    , mWriteDelayMs(writeDelayMs)
    , mpRemoteRx(NULL)
    , mResched(TSCHED_F_OCH_L, DummyIoCallbackC, this, TS_PRIO_BACKGROUND)
    , mName(name)
    , mMessagesToLose(0)
    , mRxNumNacks(0)
    , mRxNumAcks(0)
    , mRxNumRetrans(0)
  {
    CSTaskLocker cs;
    activeBridgeCount += 1;
  }

  void SetRemoteRx(BleRx &rx) {
    mpRemoteRx = &rx;
  }

  void LoseMessages(const int numMessages) {
    mMessagesToLose += numMessages;
  }

  virtual ~BleIoBridge() {
    {
      CSTaskLocker cs;
      activeBridgeCount -= 1;
    }
    mResched.disable();
    mResched.cancel();
  }

  // //////////////////////////////////////////////////////////////////////////
  // Queues a write to the PHYSICAL interface, typically using an internal queue.
  virtual bool QueueWrite(BufIOQTransT *const pTxTrans,
    const uint32_t timeout = OSAL_WAIT_INFINITE) override {

    BufIOQueue::QueueWrite(pTxTrans, timeout);
    if (!mIgnoreWrites) {
      mResched.doReschedule(mWriteDelayMs);
    }
    return true;
  }

private:
  TASKSCHED_CALLBACK_DECLARATION(BleIoBridge, DummyIoCallback) {
    {
      CSTaskLocker cs;
      if (activeBridgeCount <= 0){
        return;
      }
    }
    int len = 0;
    const uint8_t *pBuf = NULL;
    BufIOQTransT *pTrans = BufIOQueue::LoadQueuedTxTransaction(pBuf, len);
    if (0 != len) {
      const BleFragment &pkt = *(const BleFragment *)pBuf;
      sstring tmp;
      const int sz = len;
      BleFragment * cpy = (BleFragment *)tmp.u8DataPtr(sz,sz);
      memcpy(cpy, &pkt, sz);
      if (BleTx::IsNackPacket(*cpy)) {
        mRxNumNacks++;
      }
      if (BleTx::IsAckReplyPacket(*cpy)) {
        mRxNumAcks++;
      }
      if (BleTx::IsRetransPacket(*cpy)) {
        mRxNumRetrans++;
      }
      if (mpRemoteRx) {
        bool sendIt = true;
        
        if (mMessagesToLose > 0) {
          --mMessagesToLose;
          sendIt = false;
        }
        else if (mLossRatio >= 0.0001f) {
          uint16_t tmp;
          RngSingleton::inst().Read((uint8_t *)&tmp, sizeof(tmp));
          const uint16_t limit = (uint16_t)(65535.0 * mLossRatio);
          
          if (mName) {
            sstring info;
            BleTx::GetInfo(cpy, info);
            LOG_TRACE(("%s\t%s\r\n", mName, info.c_str()));
          }

          if (tmp <= limit) {
            sendIt = false;
          }
        }
        if (sendIt) {
          mpRemoteRx->OnBlePhysRx(pBuf, len);
        }
        else {
          const char *name = (mName) ? mName : "bridge";
          LOG_TRACE(("%s: Losing trans 0x%x with seq %d\r\n", name, pTrans, BleTx::SEQNUM_MASK & pBuf[1]));
        }
      }
      BufIOQueue::CommitTxTransaction(len, true);
    }
  }
  BleIoBridge()
    : mResched(TSCHED_F_OCH_L, DummyIoCallbackC, this, TS_PRIO_BACKGROUND)
  {}
private:
  bool mIgnoreWrites;
  float mLossRatio;
  int   mWriteDelayMs;
  BleRx * mpRemoteRx;
  TaskRescheduler mResched;
  const char *mName;
  int mMessagesToLose;
  static int activeBridgeCount;
public:
  int   mRxNumNacks;
  int   mRxNumAcks;
  int   mRxNumRetrans;
};

int BleIoBridge::activeBridgeCount = 0;

static bool test_bleRxMessageToStringCb(
  void *pUserData,
  const uint8_t * const pMsg,
  const int msgLen) {
  sstring &received = *(sstring *)pUserData;
  received.append(pMsg, msgLen);
  return true;
}


static bool test_bleRxMessageToCrc16Cb(
  void *pUserData,
  const uint8_t * const pMsg,
  const int msgLen) {
  uint16_t &crc16 = *(uint16_t *)pUserData;
  crc16 = CRC_16_CONT(pMsg, msgLen, crc16);
  return true;
}

#if 1

TEST_F(TestTransport, test_seqnum) {
  EXPECT_EQ(0, BleTx::GetFirstSeqMinusSecond(0, 0));
  EXPECT_EQ(4, BleTx::GetFirstSeqMinusSecond(4, 0));
  EXPECT_EQ(7, BleTx::GetFirstSeqMinusSecond(7, 0));
  EXPECT_EQ(-1, BleTx::GetFirstSeqMinusSecond(BleTx::SEQNUM_RANGE-1, 0));
  EXPECT_EQ(-4, BleTx::GetFirstSeqMinusSecond(BleTx::SEQNUM_RANGE-4, 0));
  EXPECT_EQ(-7, BleTx::GetFirstSeqMinusSecond(BleTx::SEQNUM_RANGE-7, 0));

  EXPECT_EQ(0, BleTx::GetFirstSeqMinusSecond(4, 4));
  EXPECT_EQ(4, BleTx::GetFirstSeqMinusSecond(8, 4));
  EXPECT_EQ(7, BleTx::GetFirstSeqMinusSecond(11, 4));
  EXPECT_EQ(-1, BleTx::GetFirstSeqMinusSecond(3, 4));
  EXPECT_EQ(-4, BleTx::GetFirstSeqMinusSecond(0, 4));
  EXPECT_EQ(-7, BleTx::GetFirstSeqMinusSecond(BleTx::SEQNUM_RANGE-3, 4));

  EXPECT_EQ(0, BleTx::GetFirstSeqMinusSecond(8, 8));
  EXPECT_EQ(4, BleTx::GetFirstSeqMinusSecond(12, 8));
  EXPECT_EQ(7, BleTx::GetFirstSeqMinusSecond(15, 8));
  EXPECT_EQ(-1, BleTx::GetFirstSeqMinusSecond(7, 8));
  EXPECT_EQ(-4, BleTx::GetFirstSeqMinusSecond(4, 8));
  EXPECT_EQ(-7, BleTx::GetFirstSeqMinusSecond(1, 8));

  EXPECT_EQ(0, BleTx::GetFirstSeqMinusSecond(12, 12));
  EXPECT_EQ(4, BleTx::GetFirstSeqMinusSecond(0, 12));
  EXPECT_EQ(7, BleTx::GetFirstSeqMinusSecond(3, 12));
  EXPECT_EQ(-1, BleTx::GetFirstSeqMinusSecond(11, 12));
  EXPECT_EQ(-4, BleTx::GetFirstSeqMinusSecond(8, 12));
  EXPECT_EQ(-7, BleTx::GetFirstSeqMinusSecond(5, 12));

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

static bool waitForNack(int *pNacks, const uint32_t timeout, const int numNacks) {
  const uint32_t startTime = OSALGetMS();
  uint32_t currTime = startTime;
  uint32_t elapsed = currTime - startTime;
  while ((*pNacks < numNacks) && (elapsed < timeout)) {
    OSALSleep(20);
    currTime = OSALGetMS();
    elapsed = currTime - startTime;
  }
  return (*pNacks >= numNacks);
}


TEST_F(TestTransport, test_BleFragmenter1) {
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

TEST_F(TestTransport, test_BleReassembler0) {
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

TEST_F(TestTransport, test_BleReassembler1) {

  BleReassembler rx = { 0 };
  uint8_t rxBuf[256];
  sstring received;

  uint8_t pkt0[] = { BleTx::FIRST_MASK, 0, 'h' };
  uint8_t pkt1[] = { 0, 1, 'i' };
  uint8_t pkt2[] = { BleTx::LAST_MASK, 2, '!' };

  /// Callback function called when a whole BLE/DTLS message has been received.
  auto rxFn = [](
    void * pContext,
    const uint8_t * const pPkt,
    const size_t packetSize
    ) {
    sstring &received = *(sstring *)pContext;
    received.trim_null();
    received.append(pPkt, packetSize);
  };

  BleReassembleInit(&rx, rxBuf, sizeof(rxBuf), rxFn, NULL, &received);
  BleReassembleSetOverride(&rx, test_DummyRxOverride, nullptr);

  BleReassembleFragment(&rx, pkt2, sizeof(pkt2));
  EXPECT_EQ(received.length(), 0);

  BleReassembleFragment(&rx, pkt0, sizeof(pkt0));
  BleReassembleFragment(&rx, pkt1, sizeof(pkt1));
  EXPECT_EQ(received.length(), 0);

  BleReassembleFragment(&rx, pkt0, sizeof(pkt0));
  EXPECT_EQ(received.length(), 0);
  BleReassembleFragment(&rx, pkt1, sizeof(pkt1));
  EXPECT_EQ(received.length(), 0);
  BleReassembleFragment(&rx, pkt2, sizeof(pkt2));
  EXPECT_EQ(received.length(), 3);

  const uint8_t expectedRxString[] = { 'h','i', '!' };
  EXPECT_EQ(0, memcmp(expectedRxString, received.u_str(), sizeof(expectedRxString)));

}


TEST_F(TestTransport, test_tx_flush) {
  {
    BleIoBridge bridge(false, NULL, 0, 0);
    BleTx tx(bridge, 20);
    EXPECT_EQ(0, tx.GetNextSeqNum());
    tx.Write((uint8_t *)"123456789012345678901234567890", 30);
    OSALSleep(50);
    EXPECT_EQ(2, tx.GetNextSeqNum());
    OSALSleep(50);
    tx.FlushTx();
    OSALSleep(50);
    EXPECT_EQ(2, tx.GetNextSeqNum());
    tx.ResetSeqNum();
    OSALSleep(50);
    EXPECT_EQ(0, tx.GetNextSeqNum());

    OSALSleep(10);
  }
}


TEST_F(TestTransport, test_rx_0) {
  {
    sstring received;
    BleRx rx(NULL, test_bleRxMessageToStringCb, &received);

    uint8_t pkt0[] = { BleTx::FIRST_MASK, 0, 'h' };
    uint8_t pkt1[] = { 0, 1, 'i' };
    uint8_t pkt2[] = { BleTx::LAST_MASK, 2, '!' };

    rx.OnBlePhysRx(pkt0, sizeof(pkt0));
    rx.OnBlePhysRx(pkt1, sizeof(pkt1));
    rx.OnBlePhysRx(pkt2, sizeof(pkt2));
    OSALSleep(20);

    EXPECT_EQ(received.length(), 3);
    const uint8_t expectedRxString[] = { 'h','i', '!' };
    EXPECT_EQ(0, memcmp(expectedRxString, received.u_str(), sizeof(expectedRxString)));
  }
}

TEST_F(TestTransport, test_flush_0) {
  {
    sstring received;
    BleRx rx(NULL, test_bleRxMessageToStringCb, &received);

    uint8_t pkt0[] = { BleTx::FIRST_MASK,0 };
    uint8_t pkt1[] = { 0,1, 'h' };

    rx.OnBlePhysRx(pkt0, sizeof(pkt0));
    rx.OnBlePhysRx(pkt1, sizeof(pkt1));
    OSALSleep(20);

    EXPECT_EQ(received.length(), 0);
    rx.Flush();
  }
  OSALSleep(100);
}




TEST_F(TestTransport, test_rx_1) {
  {
    sstring received;
    BleIoBridge phys0(false);
    BleTx tx0(phys0, 20);
    BleRx rx(&tx0, test_bleRxMessageToStringCb, &received);
    rx.EnableNacks(true);

    uint8_t pkt0[] = { BleTx::FIRST_MASK, 0 };
    uint8_t pkt2[] = { BleTx::LAST_MASK, 2, 'i' };

    rx.OnBlePhysRx(pkt0, sizeof(pkt0));
    rx.OnBlePhysRx(pkt2, sizeof(pkt2));
    bool gotOneNack = waitForNack(&phys0.mRxNumNacks, BleTx::NACK_GAP_TIMEOUT*1.8, 1);
    EXPECT_TRUE(gotOneNack);

    uint8_t pkt1[] = { BleTx::RETRANS_MASK, 1, 'h' };
    rx.OnBlePhysRx(pkt1, sizeof(pkt1));
    OSALSleep(BleTx::NACK_GAP_TIMEOUT*1.8);

    EXPECT_EQ(received.length(), 2);
    const uint8_t expectedRxString[] = { 'h','i' };
    EXPECT_EQ(0, memcmp(expectedRxString, received.u_str(), sizeof(expectedRxString)));
  }
}


TEST_F(TestTransport, test_nack_0) {
  {
    // Packet missing middle seq.

    for (int pktToSkip = 0; pktToSkip < 4; pktToSkip++) {
      {
        sstring received0;
        BleIoBridge phys0(false);
        BleTx tx0(phys0, 20);
        BleRx rx0(&tx0, test_bleRxMessageToStringCb, &received0);
        rx0.EnableNacks(true);
        uint8_t pkt0[] = { BleTx::FIRST_MASK, 0, 'h' };
        uint8_t pkt1[] = { 0, 1, 'i' };
        uint8_t pkt2[] = { 0, 2, '!' };
        uint8_t pkt3[] = { BleTx::LAST_MASK, 3, '?' };

        uint8_t *pktAry[] = { pkt0, pkt1, pkt2, pkt3 };

        // Send 2 packets, skip 1
        for (int i = 0; i < 4; i++) {
          if (i != pktToSkip) {
            rx0.OnBlePhysRx(pktAry[i], sizeof(pkt2));
          }
        }

        // Wait for the NACK to get sent
        const uint32_t sleepTime = (pktToSkip == 3) ? BleTx::NAK_TIMEOUT * 1.8 : BleTx::NACK_GAP_TIMEOUT*1.8;
        bool gotNack = waitForNack(&phys0.mRxNumNacks, sleepTime, 1);
        EXPECT_TRUE(gotNack);

        // Send the packet that went missing (it should be the second receiver that does this.)
        uint8_t pkt[sizeof(pkt2)];
        memcpy(pkt, pktAry[pktToSkip], sizeof(pkt2));
        ((BleFragment *)pkt)->hdr.flags |= BleTx::RETRANS_MASK;

        rx0.OnBlePhysRx(pkt, sizeof(pkt2));

        OSALSleep(100);

        // Check that the full packet has been received.
        EXPECT_EQ(received0.length(), 4);
        const uint8_t expectedRxString[] = { 'h','i','!', '?' };
        EXPECT_EQ(0, memcmp(expectedRxString, received0.u_str(), sizeof(expectedRxString)));

      }
      OSALSleep(100);
    }
  }
}

#endif // #if 0

#if 1

TEST_F(TestTransport, test_ack_lose_first_message_but_two_queued) {
  {

    const char RX1[] = "123456789012345678901234567890";
    const char RX2[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcd";
    sstring expectedRxString1;
    sstring expectedRxString2;
    while (expectedRxString1.length() < 512) {
      expectedRxString1.append(RX1);
    }
    while (expectedRxString2.length() < 512) {
      expectedRxString2.append(RX2);
    }
    if (true) {
      sstring received0;
      sstring received1;
      // Make phys0 very lossy.
      BleIoBridge phys0(false, nullptr, 0, 2, "phys0");
      BleIoBridge phys1(false, nullptr, 0, 2, "phys1");
      BleTx tx0(phys0, 185);
      BleRx rx0(&tx0, test_bleRxMessageToStringCb, &received0);
      rx0.EnableNacks(false);
      BleTx tx1(phys1, 185);
      BleRx rx1(&tx1, test_bleRxMessageToStringCb, &received1);
      rx1.EnableNacks(false);
      phys0.SetRemoteRx(rx1);
      phys1.SetRemoteRx(rx0);
      tx0.SetMaxAckRetries(10000);
      tx1.SetMaxAckRetries(10000);
      tx0.ChangeConnectionInterval(0);
      tx1.ChangeConnectionInterval(0);


      phys0.LoseMessages(2);
      tx0.WriteWithAck((uint8_t *)expectedRxString1.u_str(), expectedRxString1.length());
      tx0.WriteWithAck((uint8_t *)expectedRxString2.u_str(), expectedRxString2.length());
      while (phys1.mRxNumAcks < 2) {
        OSALSleep(100);
      }
      EXPECT_EQ(0, memcmp(expectedRxString1.u_str(), received1.u_str(), expectedRxString1.length()));
      const uint8_t *pRx2 = &received1.u_str()[expectedRxString1.length()];
      EXPECT_EQ(0, memcmp(expectedRxString2.u_str(), pRx2, expectedRxString2.length()));
    }
    // Allow time for destructors to catch up.
    OSALSleep(100);
  }
}


TEST_F(TestTransport, test_ack_memleak_0) {
  {

    const char RX[] = "123456789012345678901234567890";
    sstring expectedRxString;
    while (expectedRxString.length() < 512) {
      expectedRxString.append(RX);
    }
    if (true) {
      sstring received0;
      sstring received1;
      // Make phys0 very lossy.
      BleIoBridge phys0(false, nullptr, 0.7f, 2, "phys0");
      BleIoBridge phys1(false, nullptr, 0, 2, "phys1");
      BleTx tx0(phys0, 185);
      BleRx rx0(&tx0, test_bleRxMessageToStringCb, &received0);
      rx0.EnableNacks(false);
      BleTx tx1(phys1, 185);
      BleRx rx1(&tx1, test_bleRxMessageToStringCb, &received1);
      rx1.EnableNacks(false);
      phys0.SetRemoteRx(rx1);
      phys1.SetRemoteRx(rx0);
      tx0.SetMaxAckRetries(10000);
      tx1.SetMaxAckRetries(10000);
      tx0.ChangeConnectionInterval(0);
      tx1.ChangeConnectionInterval(0);


      tx0.WriteWithAck((uint8_t *)expectedRxString.u_str(), expectedRxString.length());
      while (0 == phys1.mRxNumAcks) {
        OSALSleep(100);
      }
      tx1.WriteWithAck(received1.u_str(), received1.length());
      while (0 == phys0.mRxNumAcks) {
        OSALSleep(100);
      }
      EXPECT_EQ(0, memcmp(expectedRxString.u_str(), received1.u_str(), 512));
      EXPECT_EQ(0, memcmp(expectedRxString.u_str(), received0.u_str(), 512));
    }
    // Allow time for destructors to catch up.
    OSALSleep(100);
  }
}

#endif // #if 0

typedef struct CheckCrcDataTag {
  sstring *pstr;
  uint16_t expectedCrc16;
} CheckCrcData;
static bool CheckCrcOnString(
                           void *pUserData,
                           const uint8_t * const pMsg,
                           const int msgLen)
{
  CheckCrcData &data = *(CheckCrcData *)pUserData;
  auto calc = CRC_16(pMsg, msgLen);
  if (calc == data.expectedCrc16){
    data.pstr->append(pMsg, msgLen);
    return true;
  }
  else {
    return false;
  }
};


#if 1

TEST_F(TestTransport, test_ack_0) {
  {

    const char RX[] = "123456789012345678901234567890";
    sstring expectedRxString;
    expectedRxString.append(RX);
    const int loops = 30;
    for (int i = 0; i <= loops; i+=2) {
      if (true) {
        sstring received0;
        sstring received1;
        auto crc16 = CRC_16(expectedRxString.u_str(), expectedRxString.length());
        CheckCrcData check0 = {
          &received0,
          crc16
        };
        CheckCrcData check1 = {
          &received1,
          crc16
        };

        LOG_TRACE(("Test Ack iteration %d of %d\r\n", i, loops));
        // Make phys0 lossy.
        BleIoBridge phys0(false, nullptr, 0.01f * i);
        BleIoBridge phys1(false);
        BleTx tx0(phys0, 185);
        BleRx rx0(&tx0, CheckCrcOnString, &check0);
        rx0.EnableNacks(false);
        BleTx tx1(phys1, 185);
        BleRx rx1(&tx1, CheckCrcOnString, &check1);
        rx1.EnableNacks(false);
        phys0.SetRemoteRx(rx1);
        phys1.SetRemoteRx(rx0);
        tx0.SetMaxAckRetries(10000);
        tx1.SetMaxAckRetries(10000);
        tx0.ChangeConnectionInterval(0);
        tx1.ChangeConnectionInterval(0);
        tx0.ChangeMessageTimeout(700);
        tx1.ChangeMessageTimeout(700);

        tx0.WriteWithAck((uint8_t *)expectedRxString.u_str(), expectedRxString.length());
        while (0 == phys1.mRxNumAcks) {
          OSALSleep(100);
        }
        if (received1.length() != expectedRxString.length()){
          EXPECT_EQ(received1.length(), expectedRxString.length());
        }

        tx1.WriteWithAck(received1.u_str(), received1.length());
        while (0 == phys0.mRxNumAcks) {
          OSALSleep(100);
        }
        if (0 != memcmp(expectedRxString.u_str(), received1.u_str(), expectedRxString.length())){
          EXPECT_EQ(0, memcmp(expectedRxString.u_str(), received1.u_str(), expectedRxString.length()));
        }
        if (0 != memcmp(expectedRxString.u_str(), received0.u_str(), expectedRxString.length())){
          EXPECT_EQ(0, memcmp(expectedRxString.u_str(), received0.u_str(), expectedRxString.length()));
        }
      }
      // Allow time for destructors to catch up.
      OSALSleep(100);
    }
  }
}

TEST_F(TestTransport, test_ack_1) {
  {
    
    const char RX[] = "123456789012345678901234567890";
    sstring expectedRxString;
    while (expectedRxString.length() < 1024) {
      expectedRxString.append(RX);
    }
    for (int i = 0; i <= 30; i+=2) {
      if (true) {
        sstring received0;
        sstring received1;
        auto crc16 = CRC_16(expectedRxString.u_str(), expectedRxString.length());
        CheckCrcData check0 = {
          &received0,
          crc16
        };
        CheckCrcData check1 = {
          &received1,
          crc16
        };
        
        // Make phys0 lossy.
        BleIoBridge phys0(false, nullptr, 0.01f * i);
        BleIoBridge phys1(false);
        BleTx tx0(phys0, 185);
        BleRx rx0(&tx0, CheckCrcOnString, &check0);
        rx0.EnableNacks(false);
        BleTx tx1(phys1, 185);
        BleRx rx1(&tx1, CheckCrcOnString, &check1);
        rx1.EnableNacks(false);
        phys0.SetRemoteRx(rx1);
        phys1.SetRemoteRx(rx0);
        tx0.SetMaxAckRetries(10000);
        tx1.SetMaxAckRetries(10000);
        tx0.ChangeConnectionInterval(0);
        tx1.ChangeConnectionInterval(0);
        tx0.ChangeMessageTimeout(700);
        tx1.ChangeMessageTimeout(700);

        
        tx0.WriteWithAck((uint8_t *)expectedRxString.u_str(), expectedRxString.length());
        while (0 == phys1.mRxNumAcks) {
          OSALSleep(100);
        }
        if (received1.length() != expectedRxString.length()){
          EXPECT_EQ(received1.length(), expectedRxString.length());
        }
        
        tx1.WriteWithAck(received1.u_str(), received1.length());
        while (0 == phys0.mRxNumAcks) {
          OSALSleep(100);
        }
        if (0 != memcmp(expectedRxString.u_str(), received1.u_str(), 1024)){
          EXPECT_EQ(0, memcmp(expectedRxString.u_str(), received1.u_str(), 1024));
        }
        if (0 != memcmp(expectedRxString.u_str(), received0.u_str(), 1024)){
          EXPECT_EQ(0, memcmp(expectedRxString.u_str(), received0.u_str(), 1024));
        }
      }
      // Allow time for destructors to catch up.
      OSALSleep(100);
    }
  }
}

TEST_F(TestTransport, test_no_ack) {
  {
    
    const char RX[] = "123456789012345678901234567890";
    sstring expectedRxString;
    while (expectedRxString.length() < 1024) {
      expectedRxString.append(RX);
    }
    for (int i = 0; i <= 5; i++) {
      if (true) {
        sstring received0;
        sstring received1;
        auto crc16 = CRC_16(expectedRxString.u_str(), expectedRxString.length());
        CheckCrcData check0 = {
          &received0,
          crc16
        };
        CheckCrcData check1 = {
          &received1,
          crc16
        };
        
        // Make phys0 lossy.
        BleIoBridge phys0(false);
        BleIoBridge phys1(false);
        BleTx tx0(phys0, 185);
        BleRx rx0(&tx0, CheckCrcOnString, &check0);
        rx0.EnableNacks(false);
        BleTx tx1(phys1, 185);
        BleRx rx1(&tx1, CheckCrcOnString, &check1);
        rx1.EnableNacks(false);
        phys0.SetRemoteRx(rx1);
        phys1.SetRemoteRx(rx0);
        tx0.SetMaxAckRetries(3);
        tx1.SetMaxAckRetries(3);
        tx0.ChangeConnectionInterval(0);
        tx1.ChangeConnectionInterval(0);
        tx0.ChangeMessageTimeout(1200);
        tx1.ChangeMessageTimeout(1200);
        
        tx0.Write((uint8_t *)expectedRxString.u_str(), expectedRxString.length());
        OSALSleep(1500);
        EXPECT_EQ(0, phys1.mRxNumAcks);
        if (received1.length() != expectedRxString.length()){
          EXPECT_EQ(received1.length(), expectedRxString.length());
        }
        
        tx1.Write(received1.u_str(), received1.length());
        OSALSleep(1500);
        EXPECT_EQ (0, phys0.mRxNumAcks);
        if (0 != memcmp(expectedRxString.u_str(), received1.u_str(), 1024)){
          EXPECT_EQ(0, memcmp(expectedRxString.u_str(), received1.u_str(), 1024));
        }
        if (0 != memcmp(expectedRxString.u_str(), received0.u_str(), 1024)){
          EXPECT_EQ(0, memcmp(expectedRxString.u_str(), received0.u_str(), 1024));
        }
      }
      // Allow time for destructors to catch up.
      OSALSleep(100);
    }
  }
}

#endif // #if 0


#if 1

TEST_F(TestTransport, test_ack_nack_0) {
  {
    
    const char RX[] = "123456789012345678901234567890";
    sstring expectedRxString;
    while (expectedRxString.length() < 2048) {
      expectedRxString.append(RX);
    }
    for (int i = 1; i <= 20; i+=2) {
      const float errorRate = 0.01f * i;
      LOG_TRACE(("\r\n\r\n====Testing Error Rate %d %% ====\r\n", (int)(errorRate * 100 + 0.5)));
      if (true) {
        sstring received0;
        sstring received1;
        auto crc16 = CRC_16(expectedRxString.u_str(), expectedRxString.length());
        CheckCrcData check0 = {
          &received0,
          crc16
        };
        CheckCrcData check1 = {
          &received1,
          crc16
        };
        
        // Make phys0 very lossy.
        BleIoBridge phys0(false, nullptr, 0.01f * i, 10, "phys0");
        BleIoBridge phys1(false, nullptr, 0, 10, "phys1");
        BleTx tx0(phys0, 185);
        BleRx rx0(&tx0, CheckCrcOnString, &check0);
        rx0.EnableNacks(true);
        BleTx tx1(phys1, 185);
        BleRx rx1(&tx1, CheckCrcOnString, &check1);
        rx1.EnableNacks(true);
        phys0.SetRemoteRx(rx1);
        phys1.SetRemoteRx(rx0);
        tx0.SetMaxAckRetries(10000);
        tx1.SetMaxAckRetries(10000);
        tx0.ChangeConnectionInterval(0);
        tx1.ChangeConnectionInterval(0);


        tx0.WriteWithAck((uint8_t *)expectedRxString.u_str(), expectedRxString.length());
        while (0 == phys1.mRxNumAcks) {
          OSALSleep(100);
        }
        if (0 != memcmp(expectedRxString.u_str(), received1.u_str(), 2048)){
          EXPECT_EQ(0, memcmp(expectedRxString.u_str(), received1.u_str(), 2048));
        }

        tx1.WriteWithAck(received1.u_str(), received1.length());
        while (0 == phys0.mRxNumAcks) {
          OSALSleep(100);
        }
        
        if (0 != memcmp(expectedRxString.u_str(), received0.u_str(), 2048)){
          EXPECT_EQ(0, memcmp(expectedRxString.u_str(), received0.u_str(), 2048));
        }
      }
      // Allow time for destructors to catch up.
      OSALSleep(100);
    }
  }
}



TEST_F(TestTransport, test_txrx) {
  {
    sstring received0;
    sstring received1;
    BleIoBridge phys0(false);
    BleIoBridge phys1(false);
    BleTx tx0(phys0, 20);
    BleRx rx0(&tx0, test_bleRxMessageToStringCb, &received0);
    rx0.EnableNacks(true);
    BleTx tx1(phys1, 20);
    BleRx rx1(&tx1, test_bleRxMessageToStringCb, &received1);
    tx0.ChangeConnectionInterval(0);
    tx1.ChangeConnectionInterval(0);

    rx1.EnableNacks(true);
    phys0.SetRemoteRx(rx1);
    phys1.SetRemoteRx(rx0);
    const char expectedRxString[] = "123456789012345678901234567890";
    tx0.Write((uint8_t *)expectedRxString, 30);
    OSALSleep(100);
    tx1.Write(received1.u_str(), received1.length());
    OSALSleep(100);
    EXPECT_EQ(0, memcmp(expectedRxString, received1.u_str(), 30));
    EXPECT_EQ(0, memcmp(expectedRxString, received0.u_str(), 30));
  }
}


TEST_F(TestTransport, test_txrx_huge) {
  {
    PrngCtr ctr;
    const int HUGESZ = 1024 * 256;
    uint8_t *pHugeBuf0 = (uint8_t *)malloc(HUGESZ);
    ctr.Read(pHugeBuf0, HUGESZ);
    const uint16_t crcRef = CRC_16(pHugeBuf0, HUGESZ);
    volatile uint16_t crc16_0 = 0xffff;

    sstring received1;

    BleIoBridge phys0(false, NULL, 0, 0);
    BleIoBridge phys1(false, NULL, 0, 0);
    BleTx tx0(phys0, 200);
    BleRx rx0(&tx0, test_bleRxMessageToStringCb, &received1);
    rx0.EnableNacks(true);
    BleTx tx1(phys1, 200);
    BleRx rx1(&tx1, test_bleRxMessageToCrc16Cb, (void *)&crc16_0);
    rx1.EnableNacks(true);
    tx0.ChangeConnectionInterval(0);
    tx1.ChangeConnectionInterval(0);

    phys0.SetRemoteRx(rx1);
    phys1.SetRemoteRx(rx0);

    int remaining = HUGESZ;
    int idx = 0;
    while (remaining > 0) {
      int bytes = MIN(remaining, 512);
      const uint16_t crc16Before = crc16_0;
      tx0.Write(&pHugeBuf0[idx], bytes);
      while (crc16Before == crc16_0) {
        OSALSleep(1);
      }
      remaining -= bytes;
      idx += bytes;
    }
    OSALSleep(100);

    EXPECT_EQ(crc16_0, crcRef);

    free(pHugeBuf0);
  }

  OSALSleep(100);
}
#endif

#if 1

TEST_F(TestTransport, test_txrx_huge_lossy) {
  int x = MemPoolsGetAllocationsWithAllocId(0);
  {
    PrngCtr ctr;
    const int HUGESZ = 128 * 64;
    uint8_t *pHugeBuf0 = (uint8_t *)malloc(HUGESZ);
    ctr.Read(pHugeBuf0, HUGESZ);
    const uint16_t crcRef = CRC_16(pHugeBuf0, HUGESZ);
    volatile uint16_t crc16_0 = 0xffff;

    sstring received1;

    BleIoBridge phys0(false, NULL, 0.07f, 0, "0-->1", "carkey", 5);
    BleIoBridge phys1(false, NULL, 0, 0, "1-->0", "carkey", 5);
    BleTx tx0(phys0, 128);
    BleRx rx0(&tx0, test_bleRxMessageToStringCb, &received1);
    rx0.EnableNacks(true);
    BleTx tx1(phys1, 128);
    BleRx rx1(&tx1, test_bleRxMessageToCrc16Cb, (void *)&crc16_0);
    tx0.ChangeConnectionInterval(0);
    tx1.ChangeConnectionInterval(0);
    rx1.EnableNacks(true);

    phys0.SetRemoteRx(rx1);
    phys1.SetRemoteRx(rx0);

    int remaining = HUGESZ;
    int idx = 0;
    while (remaining > 0) {
      int bytes = MIN(remaining, 512);
      const uint16_t crc16Before = crc16_0;
      tx0.Write(&pHugeBuf0[idx], bytes);
      while (crc16Before == crc16_0) {
        OSALSleep(1);
      }
      remaining -= bytes;
      idx += bytes;
    }
    OSALSleep(100);

    EXPECT_EQ(crc16_0, crcRef);

    free(pHugeBuf0);
  }

  OSALSleep(100);
  int y = MemPoolsGetAllocationsWithAllocId(0);
  LOG_TRACE(("%d - %d = %d\r\n", y, x, y - x));
}

#endif


#ifndef RUN_GTEST
int main(int argc, char** argv) {

  OSALInit();
  TaskSchedInit();

  int gtest_rval;
  {
      GTestRandomFileInitializer rndFile;

      // The following line must be executed to initialize Google Mock
      // (and Google Test) before running the tests.
      ::testing::InitGoogleMock(&argc, argv);

      gtest_rval = RUN_ALL_TESTS();

  }
  MemPoolsPrintUsage();

  TaskSchedQuit();

  return gtest_rval;

}
#endif
