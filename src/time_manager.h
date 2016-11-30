// Copyright Google Inc. Apache 2.0.

#ifndef _SRC_TIME_MANAGER_H_
#define _SRC_TIME_MANAGER_H_

#include <stdint.h>  // for uint64_t

// Time manager object, providing *elapse() and *_sleep() functions that
// can be faked/mocked.
class TimeManager {
 public:
  TimeManager() = default;
  virtual ~TimeManager() = default;

  virtual uint64_t ms_elapse(uint64_t start_time_ms);

  virtual void ms_sleep(uint64_t msecs);
  virtual void ns_sleep(uint64_t nsecs);
  virtual void us_sleep(uint64_t usecs);
};

#endif  // _SRC_TIME_MANAGER_H_
