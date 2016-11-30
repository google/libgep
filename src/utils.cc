// Copyright Google Inc. Apache 2.0.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS
#endif

#include "utils.h"

#include <ctype.h>  // for isprint
#include <stdarg.h>  // for va_list
#include <stdio.h>  // for printf, snprintf, fflush, etc
#include <string.h>  // for memset, strerror_r
#include <sys/time.h>  // for timeval, gettimeofday, etc
#include <time.h>  // for NULL, strftime, tm, etc
#include <string>  // for string, operator==, etc

#include "gep_common.h"  // for GepProtobufMessage

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

bool ProtobufEqual(const GepProtobufMessage &msg1,
                   const GepProtobufMessage &msg2) {
  std::string str_msg1;
  std::string str_msg2;
  msg1.SerializeToString(&str_msg1);
  msg2.SerializeToString(&str_msg2);
  return str_msg1 == str_msg2;
}

int64_t GetUnixTimeUsec() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((int64_t) tv.tv_sec * kUsecsPerSec) + tv.tv_usec;
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

}  // namespace libgep_utils
