// Copyright Google Inc. Apache 2.0.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS
#endif

#include "socket_interface.h"

#include <arpa/inet.h>  // for inet_ntop
#include <errno.h>  // for errno, EAGAIN, EWOULDBLOCK
#include <fcntl.h>  // for fcntl
#include <netinet/in.h>  // for sockaddr_in, IPPROTO_TCP, etc
#include <netinet/tcp.h>  // for TCP_NODELAY
#include <stdint.h>  // for int64_t, uint8_t
#include <stdio.h>  // for NULL
#include <string.h>  // for memset
#include <sys/select.h>  // for FD_SET, FD_ZERO, etc
#include <sys/socket.h>  // for SOL_SOCKET, etc
#include <sys/time.h>  // for timeval

#include "raw_socket_interface.h"
#include "utils.h"

using namespace libgep_utils;

int SocketInterface::FullSend(int fd, const uint8_t* buf, int size,
                              int64_t timeout_ms) {
  int64_t started_ms = time_manager_->ms_elapse(0);
  int total_sent = 0;

  while (total_sent < size) {
    int count = raw_socket_interface_->Send(fd, buf + total_sent,
                                            size - total_sent, MSG_DONTWAIT);
    if (count > 0) {
      total_sent += count;
      continue;
    }
    if (count == 0) {
      // orderly shutdown of the remote side
      return -2;
    }
    if (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
      return -1;

    // EAGAIN/EWOULDBLOCK, use select to sleep until the socket has space
    int64_t sleeptime_ms = timeout_ms - time_manager_->ms_elapse(started_ms);
    if (sleeptime_ms < 0)
      return 0;  // timed out

    struct timeval tv;
    memset(&tv, 0, sizeof(tv));
    tv.tv_sec = sleeptime_ms / 1000;
    tv.tv_usec = (sleeptime_ms % 1000) * 1000;
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(fd, &write_fds);

    int num = raw_socket_interface_->Select(fd + 1, NULL, &write_fds, NULL,
                                            &tv);
    if (num == 0)
      return 0;  // timed out
  }
  return total_sent;
}

int SocketInterface::SetNonBlocking(const char *log_module, int sock) {
  if (!log_module) log_module = "unknown";

  int flags = fcntl(sock, F_GETFL, 0);
  if (flags < 0) {
    gep_perror(errno, "%s():Error-Cannot GETFL on socket (%d)-",
                 log_module, sock);
    return -1;
  }

  if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
    gep_perror(errno, "%s():Error-Cannot SETFL on socket (%d)-", log_module,
                 sock);
    return -1;
  }
  return 0;
}

int SocketInterface::SetPriority(const char *log_module, int sock, int prio) {
  if (!log_module) log_module = "unknown";

  if (raw_socket_interface_->SetSockOpt(sock, SOL_SOCKET, SO_PRIORITY,
                                        &prio, sizeof(prio)) == -1) {
    gep_perror(errno, "%s():Error-Cannot set SO_PRIORITY to %d on socket "
                 "(%d)", log_module, prio, sock);
    return -1;
  }
  return 0;
}

int SocketInterface::SetNoDelay(const char *log_module, int sock) {
  if (!log_module) log_module = "unknown";

  int flags = 1;
  if (raw_socket_interface_->SetSockOpt(sock, IPPROTO_TCP, TCP_NODELAY,
                                        &flags, sizeof(flags)) < 0) {
    gep_perror(errno, "%s():Error-Cannot set TCP_NODELAY on socket (%d)-",
                 log_module, sock);
    return -1;
  }
  return 0;
}

int SocketInterface::SetReuseAddr(const char *log_module, int sock) {
  if (!log_module) log_module = "unknown";

  int flags = 1;
  if (raw_socket_interface_->SetSockOpt(sock, SOL_SOCKET, SO_REUSEADDR,
                                        &flags, sizeof(flags)) < 0) {
    gep_perror(errno, "%s():Error-Cannot set SO_REUSEADDR on socket (%d)-",
                 log_module, sock);
    return -1;
  }
  return 0;
}

int SocketInterface::GetPort(const char *log_module, int sock, int *port) {
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);

  if (raw_socket_interface_->GetSockName(sock, (struct sockaddr*)&addr,
                                         &addrlen) < 0) {
    gep_perror(errno, "%s(*):Error-getsockname failed on socket %d-",
                 log_module, sock);
    return -1;
  }

  *port = ntohs(addr.sin_port);
  return 0;
}

char *SocketInterface::GetPeerIP(int sock, char *buf, int size) {
  struct sockaddr_in sock_addr;
  socklen_t sockaddr_size = sizeof(sock_addr);

  if (raw_socket_interface_->GetPeerName(sock, (struct sockaddr *)&sock_addr,
                                         &sockaddr_size) == 0) {
    if (inet_ntop(AF_INET, &sock_addr.sin_addr, buf, size)) {
      return buf;
    } else {
      gep_perror(errno, "util():Error-Cannot determine peer-IP-");
    }
  }

  // error case
  nice_snprintf(buf, size, "%s", "unknown");
  return buf;
}
