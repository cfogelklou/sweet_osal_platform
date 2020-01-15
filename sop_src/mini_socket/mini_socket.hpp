#ifndef MINI_SOCKET_HPP
#define MINI_SOCKET_HPP
/**
* COPYRIGHT	(c)	Applicaudia 2019
* @file     mini_socket.hpp
* @brief    Mini cross-platform UDP sender for use by roughtime.
*/

#ifdef __cplusplus
class BufIOQueue;

// ////////////////////////////////////////////////////////////////////////////
BufIOQueue *CreateUdpClient(const char *szAddr, const int port);

// ////////////////////////////////////////////////////////////////////////////
void DeleteUdpClient(BufIOQueue *p);

#endif

#endif
