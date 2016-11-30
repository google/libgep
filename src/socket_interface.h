// Copyright Google Inc. Apache 2.0.

#ifndef _SOCKET_INTERFACE_H_
#define _SOCKET_INTERFACE_H_

#include <stddef.h>
#include <stdint.h>  // for int64_t
#include <sys/types.h>
#include <sys/socket.h>

#include <memory>  // for unique_ptr

#include "raw_socket_interface.h"

class SocketInterface {
 public:
  SocketInterface() {
    raw_socket_interface_.reset(new RawSocketInterface());
  }
  virtual ~SocketInterface() = default;

  virtual int Socket(int domain, int type, int protocol) {
    return raw_socket_interface_->Socket(domain, type, protocol);
  }
  virtual int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return raw_socket_interface_->Bind(sockfd, addr, addrlen);
  }
  virtual int Listen(int sockfd, int backlog) {
    return raw_socket_interface_->Listen(sockfd, backlog);
  }
  virtual int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return raw_socket_interface_->Accept(sockfd, addr, addrlen);
  }

  // non-blocking send/recv
  // Sends the given data until the socket accepted it all or timeout was hit.
  // Returns the number of bytes sent (=size) for success, else
  //  0 for timeout
  // -1 for error
  // -2 if the connection was orderly shutdown
  virtual int FullSend(int fd, const uint8_t* buf, int size,
                       int64_t timeout_ms);
  // TODO(chema): replace with FullRecv()
  virtual ssize_t Recv(int sockfd, void *buf, size_t len, int flags) {
    return raw_socket_interface_->Recv(sockfd, buf, len, flags);
  }

  // Sets or gets various settings on the given socket.
  // Returns -1 for error, else 0.
  virtual int SetNonBlocking(const char *log_module, int sock);
  virtual int SetPriority(const char *log_module, int sock, int prio);
  virtual int SetNoDelay(const char *log_module, int sock);
  virtual int SetReuseAddr(const char *log_module, int sock);
  virtual int GetPort(const char *log_module, int sock, int *port);
  // other socket-related functions
  virtual char *GetPeerIP(int sock, char *buf, int size);

 private:
  friend class TestableSocketInterface;

  std::unique_ptr<RawSocketInterface> raw_socket_interface_;
};

#endif  // _SOCKET_INTERFACE_H_
