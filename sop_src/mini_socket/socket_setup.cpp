#include "socket_setup.hpp"

/**
* COPYRIGHT	(c)	Applicaudia 2019
* @file     socket_setup.cpp
* @brief    see header file.
*/

// ////////////////////////////////////////////////////////////////////////////
SerSocket &SerSocket::inst() {
  static SerSocket local;
  return local;
}

// ////////////////////////////////////////////////////////////////////////////
void SerSocket::Init() {
}

// ////////////////////////////////////////////////////////////////////////////
// Constructor.  Init sockets.
SerSocket::SerSocket() {
#ifdef _WIN32
    static WSADATA wsa_data;
    WSAStartup(MAKEWORD(1, 1), &wsa_data);
#endif
  }

// ////////////////////////////////////////////////////////////////////////////
// Destructor, deinit sockets.
SerSocket::~SerSocket() {
#ifdef _WIN32
  WSACleanup();
#endif
}

// ////////////////////////////////////////////////////////////////////////////
// Close a socket.
int SerSocket::sockClose(const SOCKET sock) {
  if (sock == INVALID_SOCKET) return 0;
  int status = 0;
#ifdef _WIN32
  status = shutdown(sock, SD_BOTH);
  if (status == 0) { status = closesocket(sock); }
#else
  status = shutdown(sock, SHUT_RDWR);
  if (status == 0) { status = close(sock); }
#endif
  return status;
}

