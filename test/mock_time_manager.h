// Copyright Google Inc. Apache 2.0.

#ifndef _TEST_MOCK_TIME_MANAGER_H_
#define _TEST_MOCK_TIME_MANAGER_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stddef.h>
#include <stdint.h>  // for uint64_t

#include "time_manager.h"


// Mock version of TimeManager for testing.
class MockTimeManager : public TimeManager {
 public:
  MockTimeManager() : TimeManager() {}
  ~MockTimeManager() override {}

  MOCK_METHOD1(ms_elapse, uint64_t(uint64_t msecs));
};

#endif  // _TEST_MOCK_TIME_MANAGER_H_
