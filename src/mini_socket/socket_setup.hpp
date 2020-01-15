#ifndef SOCKET_SETUP_HPP
#define SOCKET_SETUP_HPP

/**
* COPYRIGHT	(c)	Applicaudia 2019
* @file     socket_setup.hpp
* @brief    Assists in setting up a cross-platform socket interface for applications.
*           to send/receive roughtime messages.
*/


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501  /* Windows XP. */
#endif
#include <winsock2.h>
#include <WS2tcpip.h>
#else
/* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */
typedef int SOCKET;
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#define GetLastError() (-1)
#endif

#ifndef WIN32
#define WSAGetLastError() errno
#endif

#ifdef __cplusplus

// ////////////////////////////////////////////////////////////////////////////
class SerSocket {
public:
  static SerSocket &inst();

  void Init();

  // Constructor.  Init sockets.
  SerSocket();

  // Destructor, deinit sockets.
  ~SerSocket();

  // Close a socket.
  int sockClose(const SOCKET sock);
};

#endif //__cplusplus

#endif
