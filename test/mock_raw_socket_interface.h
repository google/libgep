// Copyright Google Inc. Apache 2.0.

#ifndef _TEST_MOCK_RAW_SOCKET_INTERFACE_H_
#define _TEST_MOCK_RAW_SOCKET_INTERFACE_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stddef.h>
#include <stdint.h>  // for int64_t
#include <sys/types.h>
#include <sys/socket.h>

#include "raw_socket_interface.h"


// Mock version of RawSocketInterface for testing.
class MockRawSocketInterface : public RawSocketInterface {
 public:
  MockRawSocketInterface() : RawSocketInterface() {}
  ~MockRawSocketInterface() override {}

  MOCK_METHOD4(Send, ssize_t(int sockfd, const void *buf, size_t len,
                             int flags));
};

#endif  // _TEST_MOCK_RAW_SOCKET_INTERFACE_H_
