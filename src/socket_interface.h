// Copyright Google Inc. Apache 2.0.

#ifndef _SOCKET_INTERFACE_H_
#define _SOCKET_INTERFACE_H_

#include <stddef.h>
#include <stdint.h>  // for int64_t
#include <sys/types.h>
#include <sys/socket.h>

class SocketInterface {
 public:
  virtual ~SocketInterface() {};
  virtual int Socket(int domain, int type, int protocol) {
    return socket(domain, type, protocol);
  }
  virtual int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return bind(sockfd, addr, addrlen);
  }
  virtual int Listen(int sockfd, int backlog) {
    return listen(sockfd, backlog);
  }
  virtual int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return accept(sockfd, addr, addrlen);
  }
  virtual ssize_t Recv(int sockfd, void *buf, size_t len, int flags) {
    return recv(sockfd, buf, len, flags);
  }
  virtual ssize_t Send(int sockfd, const void *buf, size_t len, int flags) {
    return send(sockfd, buf, len, flags);
  }
  virtual int Select(int nfds, fd_set *readfds, fd_set *writefds,
                     fd_set *exceptfds, struct timeval *timeout) {
    return select(nfds, readfds, writefds, exceptfds, timeout);
  }

  // non-blocking send/recv
  // Sends the given data until the socket accepted it all or timeout was hit.
  // Returns the number of bytes sent (=size) for success, else
  //  0 for timeout
  // -1 for error
  // -2 if the connection was orderly shutdown
  virtual int FullSend(int fd, const uint8_t* buf, int size,
                       int64_t timeout_ms);
  // Sets or gets various settings on the given socket.
  // Returns -1 for error, else 0.
  virtual int SetNonBlocking(const char *log_module, int sock);
  virtual int SetPriority(const char *log_module, int sock, int prio);
  virtual int SetNoDelay(const char *log_module, int sock);
  virtual int SetReuseAddr(const char *log_module, int sock);
  virtual int GetPort(const char *log_module, int sock, int *port);
  // other socket-related functions
  virtual char *GetPeerIP(int sock, char *buf, int size);
};

#endif  // _SOCKET_INTERFACE_H_
