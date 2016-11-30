// Copyright Google Inc. Apache 2.0.

#ifndef _RAW_SOCKET_INTERFACE_H_
#define _RAW_SOCKET_INTERFACE_H_

#include <stddef.h>
#include <stdint.h>  // for int64_t
#include <sys/types.h>
#include <sys/socket.h>

class RawSocketInterface {
 public:
  virtual ~RawSocketInterface() = default;

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
  virtual int GetSockOpt(int sockfd, int level, int optname,
                         void *optval, socklen_t *optlen) {
    return getsockopt(sockfd, level, optname, optval, optlen);
  }
  virtual int SetSockOpt(int sockfd, int level, int optname,
                         const void *optval, socklen_t optlen) {
    return setsockopt(sockfd, level, optname, optval, optlen);
  }
  virtual int GetSockName(int sockfd, struct sockaddr *addr,
                          socklen_t *addrlen) {
    return getsockname(sockfd, addr, addrlen);
  }
  virtual int GetPeerName(int sockfd, struct sockaddr *addr,
                          socklen_t *addrlen) {
    return getpeername(sockfd, addr, addrlen);
  }
};

#endif  // _RAW_SOCKET_INTERFACE_H_
