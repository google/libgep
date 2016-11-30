// Copyright Google Inc. Apache 2.0.

#include "socket_interface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>  // for AssertHelper, TEST_F, etc
#include <stdint.h>  // for uint8_t, int64_t

#include "mock_raw_socket_interface.h"  // for MockRawSocketInterface
#include "mock_time_manager.h"  // for MockTimeManager

using ::testing::_;
using ::testing::Return;
using ::testing::SetErrnoAndReturn;


class TestableSocketInterface : public SocketInterface {
 public:
  TestableSocketInterface() : SocketInterface() {};
  using SocketInterface::raw_socket_interface_;
  using SocketInterface::time_manager_;
};

class SocketInterfaceTest : public ::testing::Test {
 public:
  void SetUp() override {
    socket_interface_.reset(new TestableSocketInterface());
    mock_raw_socket_interface_ = new MockRawSocketInterface();
    socket_interface_->raw_socket_interface_.reset(mock_raw_socket_interface_);
    mock_time_manager_ = new MockTimeManager();
    socket_interface_->time_manager_.reset(mock_time_manager_);
  }
  void TearDown() override {
    socket_interface_.reset(nullptr);
  }

 protected:
  std::unique_ptr<TestableSocketInterface> socket_interface_;
  MockRawSocketInterface* mock_raw_socket_interface_;
  MockTimeManager* mock_time_manager_;
};


TEST_F(SocketInterfaceTest, FullSendOK) {
  EXPECT_CALL(*mock_raw_socket_interface_, Send(_, _, _, _))
      .WillOnce(Return(1024));
  EXPECT_CALL(*mock_time_manager_, ms_elapse(_))
      .WillOnce(Return(1));

  int fd = 1;
  const uint8_t buf[1024] = {};
  int size = 1024;
  int64_t timeout_ms = 10;
  EXPECT_EQ(1024, socket_interface_->FullSend(fd, buf, size, timeout_ms));
}

TEST_F(SocketInterfaceTest, FullSendShutdown) {
  EXPECT_CALL(*mock_raw_socket_interface_, Send(_, _, _, _))
      .WillOnce(Return(0));
  EXPECT_CALL(*mock_time_manager_, ms_elapse(_))
      .WillOnce(Return(1));

  int fd = 1;
  const uint8_t buf[1024] = {};
  int size = 1024;
  int64_t timeout_ms = 10;
  EXPECT_EQ(-2, socket_interface_->FullSend(fd, buf, size, timeout_ms));
}

TEST_F(SocketInterfaceTest, FullSendError) {
  EXPECT_CALL(*mock_raw_socket_interface_, Send(_, _, _, _))
      .WillOnce(SetErrnoAndReturn(EADDRINUSE, -1));
  EXPECT_CALL(*mock_time_manager_, ms_elapse(_))
      .WillOnce(Return(1));

  int fd = 1;
  const uint8_t buf[1024] = {};
  int size = 1024;
  int64_t timeout_ms = 10;
  EXPECT_EQ(-1, socket_interface_->FullSend(fd, buf, size, timeout_ms));
}

TEST_F(SocketInterfaceTest, FullSendTimeout) {
  EXPECT_CALL(*mock_raw_socket_interface_, Send(_, _, _, _))
      .WillOnce(SetErrnoAndReturn(EAGAIN, -1));
  EXPECT_CALL(*mock_time_manager_, ms_elapse(_))
      .WillOnce(Return(1))
      .WillOnce(Return(11));

  int fd = 1;
  const uint8_t buf[1024] = {};
  int size = 1024;
  int64_t timeout_ms = 10;
  EXPECT_EQ(0, socket_interface_->FullSend(fd, buf, size, timeout_ms));
}
