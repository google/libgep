#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS
#endif

#include "utils.h"

#include <arpa/inet.h>                  // for inet_ntop
#include <ctype.h>                      // for isprint
#include <errno.h>                      // for errno, EAGAIN, EWOULDBLOCK
#include <fcntl.h>                      // for fcntl
#include <google/protobuf/message.h>    // for Message
#include <netinet/in.h>                 // for sockaddr_in, IPPROTO_TCP, etc
#include <netinet/tcp.h>                // for TCP_NODELAY
#include <stdarg.h>                     // for va_list
#include <stdio.h>                      // for printf, snprintf, fflush, etc
#include <string.h>                     // for memset, strerror_r
#include <sys/select.h>                 // for select, FD_SET, FD_ZERO, etc
#include <sys/socket.h>                 // for setsockopt, SOL_SOCKET, etc
#include <sys/time.h>                   // for timeval, gettimeofday, etc
#include <time.h>                       // for NULL, strftime, tm, etc
#include <string>                       // for string, operator==, etc


namespace libgep_utils {

LogLevel gep_log_level = LOG_WARNING;

void gep_log_set_level(LogLevel level) {
  gep_log_level = level;
}

LogLevel gep_log_get_level() {
  return gep_log_level;
}

void gep_log(LogLevel level, const char* cstr, ...) {
  va_list va;

  if (level > gep_log_level)
    return;

  char date_str[kDateStringLen];
  snprintf_date(date_str, kDateStringLen, NULL, false);
  printf("%s ", date_str);

  const char *fmt = cstr;
  va_start(va, cstr);
  vprintf(fmt, va);
  va_end(va);

  printf("\n");
  fflush(stdout);
}

void gep_perror(int err, const char* cstr, ...) {
  va_list va;

  char date_str[kDateStringLen];
  snprintf_date(date_str, kDateStringLen, NULL, false);
  printf("%s ", date_str);

  const char *fmt = cstr;
  va_start(va, cstr);
  vprintf(fmt, va);
  va_end(va);

  char strerrbuf[1024] = {'\0'};
  printf(" '%s'[%d]", strerror_r(err, strerrbuf, sizeof(strerrbuf)), err);

  printf("\n");
  fflush(stdout);
}

bool ProtobufEqual(const ::google::protobuf::Message &msg1,
                   const ::google::protobuf::Message &msg2) {
  return msg1.DebugString() == msg2.DebugString();
}

int64_t GetUnixTimeUsec() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((int64_t) tv.tv_sec * kUsecsPerSec) + tv.tv_usec;
}

static inline uint64_t get_now_ns() {
  struct timespec tv;
  clock_gettime(CLOCK_MONOTONIC, &tv);
  return tv.tv_sec * kNsecsPerSec + tv.tv_nsec;
}

uint64_t ms_elapse(uint64_t start_time_ms) {
  return (get_now_ns() - start_time_ms * kNsecsPerMsec) / kNsecsPerMsec;
}

int nice_snprintf(char *str, size_t size, const char *format, ...) {
  va_list ap;
  int bi;
  va_start(ap, format);
  // http://stackoverflow.com/a/100991
  bi = vsnprintf(str, size, format, ap);
  va_end(ap);
  if (bi > size) {
    // From printf(3):
    // "snprintf() [returns] the number of characters (not including the
    // trailing '\0') which would have been written to the final string
    // if enough space had been available" [printf(3)]
    bi = size;
  }
  return bi;
}

/* snprintf's <len> bytes in hex */
int snprintf_hex(char *buf, int bufsize, const uint8_t *data, int len) {
  int i;
  int bi = 0;

  // ensure we always return something valid
  buf[0] = '\0';
  for (i = 0; i < len && bi < bufsize; ++i) {
    bi += nice_snprintf(buf+bi, bufsize-bi, "%02x", *(data + i));
  }
  return bi;
}

/* snprintf's <len> bytes in printable characters */
int snprintf_printable(char *buf, int bufsize, const uint8_t *data, int len) {
  int i;
  int bi = 0;

  // ensure we always return something valid
  buf[0] = '\0';
  for (i = 0; i < len && bi < bufsize; ++i) {
    if (isprint(*(data + i)))
      bi += nice_snprintf(buf+bi, bufsize-bi, "%c", *(data + i));
    else
      bi += nice_snprintf(buf+bi, bufsize-bi, "\\x%02x", *(data + i));
  }
  return bi;
}

/* snprintf's date (use tv=NULL for current) in a standard format (iso 8601) */
int snprintf_date(char *buf, int bufsize, const struct timeval *tvin,
                  bool full) {
  int bi = 0;
  struct timeval tv;

  if (tvin == NULL) {
    // get the current time
    gettimeofday(&tv, NULL);
  } else {
    tv.tv_sec = tvin->tv_sec;
    tv.tv_usec = tvin->tv_usec;
  }

  // get the timezone
  struct tm ltm;
  localtime_r(&tv.tv_sec, &ltm);
  bool is_dst = (ltm.tm_isdst > 0);

  // adjust timeval to Pacific Time (PST or UTC-08, PDT or UTC-07)
  if (is_dst)
    tv.tv_sec -= 3600 * 7;
  else
    tv.tv_sec -= 3600 * 8;

  // convert it to a calendar time (UTC-based)
  gmtime_r(&tv.tv_sec, &ltm);

  // adjust the time zone
  if (is_dst)
    ltm.tm_gmtoff -= 3600 * 7;
  else
    ltm.tm_gmtoff -= 3600 * 8;

  // convert it to a string
  int ret;
  if (full)
    ret = strftime(buf+bi, bufsize-bi, "%FT%T", &ltm);
  else
    ret = strftime(buf+bi, bufsize-bi, "%d,%T", &ltm);
  if (ret <= 0) {
    // note that strftime return code is not like snprintf's: "If the
    // length of the result string (including the terminating null byte)
    // would exceed max bytes, then strftime() returns 0, and the contents
    // of the array are undefined."
    buf[bi] = '\0';
    return bi;
  }
  bi += ret;

  bi += nice_snprintf(buf+bi, bufsize-bi, ".%03d",
                      static_cast<int>(tv.tv_usec / 1000));
  if (full) {
    ret = strftime(buf+bi, bufsize-bi, "%z", &ltm);
    if (ret <= 0)
      buf[bi] = '\0';
    else
      bi += ret;
  }

  return bi;
}

int FullSend(int fd, const uint8_t* buf, int size, int64_t timeout_ms) {
  int64_t started_ms = ms_elapse(0);
  int total_sent = 0;

  while (total_sent < size) {
    int count = send(fd, buf + total_sent, size - total_sent, MSG_DONTWAIT);
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
    int64_t sleeptime_ms = timeout_ms - ms_elapse(started_ms);
    if (sleeptime_ms < 0)
      return 0;  // timed out

    struct timeval tv;
    memset(&tv, 0, sizeof(tv));
    tv.tv_sec = sleeptime_ms / 1000;
    tv.tv_usec = (sleeptime_ms % 1000) * 1000;
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(fd, &write_fds);

    int num = select(fd + 1, NULL, &write_fds, NULL, &tv);
    if (num == 0)
      return 0;  // timed out
  }
  return total_sent;
}

int set_socket_non_blocking(const char *log_module, int sock) {
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

int set_socket_priority(const char *log_module, int sock, int prio) {
  if (!log_module) log_module = "unknown";

  if (setsockopt(sock, SOL_SOCKET, SO_PRIORITY, &prio, sizeof(prio)) == -1) {
    gep_perror(errno, "%s():Error-Cannot set SO_PRIORITY to %d on socket "
                 "(%d)", log_module, prio, sock);
    return -1;
  }
  return 0;
}

int set_socket_no_delay(const char *log_module, int sock) {
  if (!log_module) log_module = "unknown";

  int flags = 1;
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags)) < 0) {
    gep_perror(errno, "%s():Error-Cannot set TCP_NODELAY on socket (%d)-",
                 log_module, sock);
    return -1;
  }
  return 0;
}

int set_socket_reuse_addr(const char *log_module, int sock) {
  if (!log_module) log_module = "unknown";

  int flags = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags)) < 0) {
    gep_perror(errno, "%s():Error-Cannot set SO_REUSEADDR on socket (%d)-",
                 log_module, sock);
    return -1;
  }
  return 0;
}

int get_socket_port(const char *log_module, int sock, int *port) {
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);

  if (getsockname(sock, (struct sockaddr*)&addr, &addrlen) < 0) {
    gep_perror(errno, "%s(*):Error-getsockname failed on socket %d-",
                 log_module, sock);
    return -1;
  }

  *port = ntohs(addr.sin_port);
  return 0;
}

char *get_peer_ip(int sock, char *buf, int size) {
  struct sockaddr_in sock_addr;
  socklen_t sockaddr_size = sizeof(sock_addr);

  if (getpeername(sock, (struct sockaddr *)&sock_addr, &sockaddr_size) == 0) {
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

}  // namespace libgep_utils
