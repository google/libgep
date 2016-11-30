// Copyright Google Inc. Apache 2.0.

#include "time_manager.h"

#include <stdint.h>  // for uint64_t
#include <sys/time.h>  // for CLOCK_MONOTONIC
#include <time.h>
#include <unistd.h>

#include "utils.h"  // for kNsecsPerSec


using namespace libgep_utils;

static inline uint64_t get_now_ns() {
  struct timespec tv;
  clock_gettime(CLOCK_MONOTONIC, &tv);
  return tv.tv_sec * kNsecsPerSec + tv.tv_nsec;
}

uint64_t TimeManager::ms_elapse(uint64_t start_time_ms) {
  return (get_now_ns() - start_time_ms * kNsecsPerMsec) / kNsecsPerMsec;
}

void TimeManager::ms_sleep(uint64_t msecs) {
  ns_sleep(kNsecsPerMsec * msecs);
}

void TimeManager::us_sleep(uint64_t usecs) {
  ns_sleep(kNsecsPerUsec * usecs);
}

void TimeManager::ns_sleep(uint64_t nsecs) {
  struct timespec tv;
  // tv_nsec field must be [0..kNsecsPerSec-1]
  tv.tv_sec = nsecs / kNsecsPerSec;
  tv.tv_nsec = nsecs % kNsecsPerSec;
  nanosleep(&tv, NULL);
}
