#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>  // for int64_t, uint8_t, uint16_t, etc
#include <sys/time.h>  // for timeval
#include <google/protobuf/message.h>


#define UINT64(x)                                                  \
    (((uint64_t)*((x) + 0) << 56) | ((uint64_t)*((x) + 1) << 48) |   \
     ((uint64_t)*((x) + 2) << 40) | ((uint64_t)*((x) + 3) << 32) |   \
     ((uint64_t)*((x) + 4) << 24) | ((uint64_t)*((x) + 5) << 16) |   \
     ((uint64_t)*((x) + 6) <<  8) | ((uint64_t)*((x) + 7)))
#define INT64(x) (int64_t)UINT64(x)
#define UINT32(x)                                                 \
    (((uint32_t)*((x) + 0) << 24) | ((uint32_t)*((x) + 1) << 16) |  \
     ((uint32_t)*((x) + 2) <<  8) | ((uint32_t)*((x) + 3)))
#define INT32(x) (int32_t)UINT32(x)
#define UINT16(x) (((uint32_t)*(x) << 8) | ((uint32_t)*((x) + 1)))
#define INT16(x) (int16_t)UINT16(x)

#define SET_UINT8(x, y)       \
  {                           \
    *((x)) = (uint8_t)((y));  \
  }
#define SET_UINT16(x, y)           \
  {                                \
    *((x)) = (uint8_t)((y) >> 8);  \
    *((x) + 1) = (uint8_t)((y));   \
  }
#define SET_UINT32(x, y)               \
  {                                    \
    *((x)) = (uint8_t)((y) >> 24);     \
    *((x) + 1) = (uint8_t)((y) >> 16); \
    *((x) + 2) = (uint8_t)((y) >> 8);  \
    *((x) + 3) = (uint8_t)((y));       \
  }
#define SET_UINT64(x, y)               \
  {                                    \
    *((x)) = (uint8_t)((y) >> 56);     \
    *((x) + 1) = (uint8_t)((y) >> 48); \
    *((x) + 2) = (uint8_t)((y) >> 40); \
    *((x) + 3) = (uint8_t)((y) >> 32); \
    *((x) + 4) = (uint8_t)((y) >> 24); \
    *((x) + 5) = (uint8_t)((y) >> 16); \
    *((x) + 6) = (uint8_t)((y) >> 8);  \
    *((x) + 7) = (uint8_t)((y));       \
  }

typedef enum {
  LOG_ERROR = 1,
  LOG_WARNING = 2,
  LOG_DEBUG = 3,
} LogLevel;

void gep_log_set_level(LogLevel level);
LogLevel gep_log_get_level();

void gep_log(LogLevel level, const char* cstr, ...)
  __attribute__((format(printf, 2, 3)));

void gep_perror(int err, const char* cstr, ...)
  __attribute__((format(printf, 2, 3)));


bool ProtobufEqual(const ::google::protobuf::Message &msg1,
                   const ::google::protobuf::Message &msg2);

const int64_t kMsecsPerSec = 1000LL;
const int64_t kUsecsPerSec = 1000000LL;
const int64_t kUsecsPerMsec = 1000LL;
const int64_t kNsecsPerSec = 1000000000LL;
const int64_t kNsecsPerUsec = 1000LL;
const int64_t kNsecsPerMsec = 1000000LL;
const int64_t kOneDayInSec = 24 * 60 * 60;

const int64_t kUnixTimeInvalid = -1;

static inline int64_t secs_to_msecs(int64_t secs) {
  return secs * kMsecsPerSec;
}

static inline int64_t msecs_to_secs(int64_t msecs) {
  return msecs / kMsecsPerSec;
}

static inline int64_t secs_to_usecs(int64_t secs) {
  return secs * kUsecsPerSec;
}

static inline int64_t usecs_to_secs(int64_t usecs) {
  return usecs / kUsecsPerSec;
}

static inline int64_t msecs_to_usecs(int64_t msecs) {
  return msecs * kUsecsPerMsec;
}

static inline int64_t usecs_to_msecs(int64_t usecs) {
  return usecs / kUsecsPerMsec;
}

static inline int64_t usecs_to_nsecs(int64_t usecs) {
  return usecs * kNsecsPerUsec;
}

static inline int64_t nsecs_to_usecs(int64_t nsecs) {
  return nsecs / kNsecsPerUsec;
}

static inline int64_t timeval_to_usecs(const struct timeval *tv) {
  return (secs_to_usecs(tv->tv_sec) + tv->tv_usec);
}

static inline struct timeval usecs_to_timeval(int64_t usecs) {
  struct timeval out;
  out.tv_sec = usecs / kUsecsPerSec;
  out.tv_usec = (usecs % kUsecsPerSec);
  return out;
}

// Returns the current timestamp as an int64_t (microseconds since unix epoch).
int64_t GetUnixTimeUsec();

// Returns the current timestamp as an int64_t (seconds since unix epoch).
int64_t GetUnixTimeSec();

// Fills the given buffer with a character string of the peer IP address
// of the given socket. On error it inserts the string "unknown". In both
// cases it returns a pointer to the beginning of the buffer.
char *get_peer_ip(int sock, char *buf, int size);

// A version of snprintf() that, when it cannot write enough characters,
// returns the number of characters actually printed. Compare with
// (vanilla) snprintf(), which returns "the number of characters (not
// including the trailing '\0') which would have been written to the
// final string if enough space had been available." [printf(3)]
// See http://stackoverflow.com/a/28295939.
int nice_snprintf(char *str, size_t size, const char *format, ...);

// Prints a printable string into a buffer.
int snprintf_printable(char *buf, int bufsize, const uint8_t *data, int len);

const int kDateStringLen = 64;
// Prints date (use tv=NULL for current) in a standard format (iso 8601).
// Set "full" to true to print the whole date, false for a concise version.
int snprintf_date(char *buf, int bufsize, const struct timeval *tv, bool full);

// non-blocking send/recv
// Sends the given data until the socket accepted it all or timeout was hit.
// Returns the number of bytes sent (=size) for success, else
//  0 for timeout
// -1 for error
// -2 if the connection was orderly shutdown
int FullSend(int fd, const uint8_t* buf, int size, int64_t timeout_ms);

// Sets or gets various settings on the given socket.
// Returns -1 for error, else 0.
int set_socket_non_blocking(const char *log_module, int sock);
int set_socket_priority(const char *log_module, int sock, int prio);
int set_socket_no_delay(const char *log_module, int sock);
int set_socket_reuse_addr(const char *log_module, int sock);
int set_socket_timeout(const char *log_module, int sock, int ms);
int set_socket_sndbuf_size(const char *log_module, int sock, int size,
                           int *actual_size);
int get_socket_sndbuf_size(const char *log_module, int sock, int *actual_size);
int set_socket_rcvbuf_size(const char *log_module, int sock, int size,
                           int *actual_size);
int get_socket_rcvbuf_size(const char *log_module, int sock, int *actual_size);
int get_socket_port(const char *log_module, int sock, int *port);

#endif  // _UTILS_H_
