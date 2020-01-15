/**
* COPYRIGHT	(c)	Applicaudia 2019
* @file     mini_socket.cpp
* @brief    Mini cross-platform UDP sender/receiver for use by roughtime.
*/

#include "mini_socket.hpp"
#include "utils/byteq.hpp"
#include "buf_io/buf_io_queue.hpp"
#include "utils/platform_log.h"
#include "osal/osal.h"
#include "osal/platform_type.h"
#include "socket_setup.hpp"

#include <cstdio>
#include <cstring>
#include <queue>
#include <thread>
#include <mutex>          // std::mutex, std::unique_lock, std::defer_lock

class UdpClient: public BufIOQueue {
public:
  
public:
  UdpClient(const char *szAddr, const int port)
  : BufIOQueue(CriticalSectionType::Critical)
  , mSocket(INVALID_SOCKET)
  {
    SerSocket::inst().Init();
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family     = AF_INET; // AF_INIT == internetwork: UDP, TCP, etc.
    hints.ai_socktype   = SOCK_DGRAM; /* datagram socket */
    hints.ai_protocol   = IPPROTO_UDP;

#ifdef TARGET_OS_ANDROID
    hints.ai_flags = AI_CANONNAME;
#else
    hints.ai_flags |= AI_ALL; // Query both IP6 and IP4 with AI_V4MAPPED
#endif
    
    // Resolve the server address and port
    char szPort[10];
    snprintf(szPort, sizeof(szPort), "%d", port);
    int result = getaddrinfo(szAddr, szPort, &hints, &mpAddrInfo);
    if (result != 0) {
      LOG_TRACE(("getaddrinfo: %s\n", gai_strerror(result)));
      //exit(EXIT_FAILURE);

    }
    
    //Create a socket
    if (mpAddrInfo){
      if((mSocket = socket(mpAddrInfo->ai_family, mpAddrInfo->ai_socktype, mpAddrInfo->ai_protocol )) == INVALID_SOCKET){
        auto err = WSAGetLastError();
        LOG_TRACE(("socket: %s\n", gai_strerror(err)));
        //exit(-1);
      }
    }
    if (mSocket != INVALID_SOCKET){
#ifndef WIN32
      struct timeval tv;
      tv.tv_sec = 5;
      tv.tv_usec = 0;
      if (setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        //perror("Error");
        auto err = WSAGetLastError();
        LOG_TRACE(("setsockopt: %s\n", gai_strerror(err)));
      }
#endif
    }
  }
  
  // //////////////////////////////////////////////////////////////////////////
  ~UdpClient(){
    freeaddrinfo(mpAddrInfo);
    SerSocket::inst().sockClose(mSocket);
  }
  
  // //////////////////////////////////////////////////////////////////////////
  bool QueueWrite(
    BufIOQTransT *const pTxTrans,
    const uint32_t timeout = OSAL_WAIT_INFINITE) override 
  {
    bool ok = true;
    if (INVALID_SOCKET == mSocket) return false;

    // Send to socket.
    int stat = sendto(
      mSocket,
      (const char *)pTxTrans->pBuf8,
      pTxTrans->transactionLen,
      0, mpAddrInfo->ai_addr, mpAddrInfo->ai_addrlen);

    // Verify send.
    ok = (stat == pTxTrans->transactionLen);
    pTxTrans->transactionLen = (ok) ? stat : 0;

    // Call completion callback 
    if (pTxTrans->completedCb){
      pTxTrans->completedCb(pTxTrans);
    }

    // Print error, if there was one.
    if (!ok) {
      auto err = WSAGetLastError();
      LOG_TRACE(("send: %s\n", gai_strerror(err)));
    }
    
#ifdef WIN32
    if (ok) {
      static int timeout = 1000;
      stat = setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
      ok = (stat != SOCKET_ERROR);
    }
#endif
    
    if (ok){
      // Use thread to read from socket to prevent blocking.
      std::thread t(ReadFromSocketThreadC, this);
      t.detach();
    }
    return true;
  }
  
private:
  // //////////////////////////////////////////////////////////////////////////
  // Use thread to read from socket to prevent blocking.
  static void ReadFromSocketThreadC(void *pThis) {
    UdpClient &th = *(UdpClient *)pThis;
    if (INVALID_SOCKET == th.mSocket) return;

    uint8_t *pRxBuf = nullptr;
    int rxBufLen = 0;
    th.LoadQueuedRxTransaction(pRxBuf, rxBufLen);
    if ((pRxBuf) && (rxBufLen > 0)) {
      socklen_t alen = th.mpAddrInfo->ai_addrlen;
      const int rx = recvfrom(th.mSocket, (char *)pRxBuf, rxBufLen, 0, th.mpAddrInfo->ai_addr, &alen);
      OSALEnterCritical();
      if (SOCKET_ERROR != rx) {
        th.CommitRxTransaction(rx, true, true);
      }
      else {
        th.CommitRxTransaction(0, true, true);
        const int err = WSAGetLastError();
        LOG_TRACE(("Error on recv: %s\n", gai_strerror(err)));
      }
      OSALExitCritical();
    }
    else {
      const int err = WSAGetLastError();
      LOG_TRACE(("recv: %s\n", gai_strerror(err)));
    }
  };

private:

  struct addrinfo  *mpAddrInfo;
  SOCKET            mSocket;
  
};

// ////////////////////////////////////////////////////////////////////////////
BufIOQueue *CreateUdpClient(const char *szAddr, const int port){
  auto p = new UdpClient(szAddr, port);
  return p;
}

// ////////////////////////////////////////////////////////////////////////////
void DeleteUdpClient(BufIOQueue *p){
  UdpClient *ps = (UdpClient *)p;
  delete ps;
}
