#include <stdio.h>  // for remove
#include <string.h>  // for memset
#include <unistd.h>  // for usleep

#include "gep_client.h"
#include "gep_test_lib.h"
#include "socket_interface.h"
#include "test_protocol.h"

#include "gtest/gtest.h"

class FailingSocketInterface: public SocketInterface {
 public:
  virtual int Socket(int domain, int type, int protocol) { return -1; }
  virtual ssize_t Recv(int sockfd, void *buf, size_t len, int flags) {
    return -2;
  }
  int send_error_code_;
  virtual int FullSend(int fd, const uint8_t* buf, int size,
                       int64_t timeout_ms) {
    return send_error_code_;
  }
};

class FlakySocketInterface: public SocketInterface {
 public:
  FlakySocketInterface(int counter) : counter_(counter) {};
  virtual int Socket(int domain, int type, int protocol) { return -1; }
  virtual ssize_t Recv(int sockfd, void *buf, size_t len, int flags) {
    return -2;
  }
  int counter_;
  virtual int FullSend(int fd, const uint8_t* buf, int size,
                       int64_t timeout_ms) {
    if ((counter_++ % 2) == 0)
      return SocketInterface::FullSend(fd, buf, size, timeout_ms);
    else
      return 0;
  }
};

class GepChannelTest : public GepTest {
};

TEST_F(GepChannelTest, SetSocket) {
  // increase the log verbosity
  gep_log_set_level(LOG_DEBUG);

  // push message in the client
  GepChannel *gc = client_->GetGepChannel();
  int socket = gc->GetSocket();
  gc->SetSocket(socket);
  gc->SendMessage(command1_);

  // push message in the server
  GepChannelArray *gca = server_->GetGepChannelArray();
  gca->SendMessage(command3_);

  WaitForSync(2);
}

TEST_F(GepChannelTest, RecvDataInvalidSocket) {
  // set an invalid socket
  GepChannel *gc = client_->GetGepChannel();
  gc->SetSocket(-1);
  EXPECT_EQ(-1, gc->RecvData());
}

TEST_F(GepChannelTest, RecvDataBufferFull) {
  // add too much read data
  GepChannel *gc = client_->GetGepChannel();
  gc->SetLen(GepProtocol::kMaxMsgLen + 1);
  EXPECT_EQ(-1, gc->RecvData());
}

TEST_F(GepChannelTest, FailingRecvSocket) {
  // set a failing socket interface
  GepChannel *gc = client_->GetGepChannel();
  FailingSocketInterface failing_socket_interface{};
  SocketInterface *old_socket_interface = gc->GetSocketInterface();
  gc->SetSocketInterface(&failing_socket_interface);

  // check socket()
  EXPECT_EQ(-1, gc->OpenClientSocket());

  // check recv()
  EXPECT_EQ(-1, gc->RecvData());

  // reinstante the socket interface
  gc->SetSocketInterface(old_socket_interface);
}

TEST_F(GepChannelTest, FailingSendSocket) {
  // set a failing socket interface
  GepChannel *gc = client_->GetGepChannel();
  FailingSocketInterface failing_socket_interface{};
  SocketInterface *old_socket_interface = gc->GetSocketInterface();
  gc->SetSocketInterface(&failing_socket_interface);

  // check different failure modes
  failing_socket_interface.send_error_code_ = 0;
  EXPECT_EQ(-1, gc->SendMessage(command1_));

  failing_socket_interface.send_error_code_ = -1;
  EXPECT_EQ(-1, gc->SendMessage(command1_));

  failing_socket_interface.send_error_code_ = -2;
  EXPECT_EQ(-1, gc->SendMessage(command1_));

  // reinstante the socket interface
  gc->SetSocketInterface(old_socket_interface);
}

TEST_F(GepChannelTest, FlakySendSocket) {
  // set a flaky socket interface
  GepChannel *gc = client_->GetGepChannel();
  FlakySocketInterface flaky_socket_interface{0};
  SocketInterface *old_socket_interface = gc->GetSocketInterface();
  gc->SetSocketInterface(&flaky_socket_interface);

  // check failure mode
  EXPECT_EQ(-1, gc->SendMessage(command1_));

  // reinstante the socket interface
  gc->SetSocketInterface(old_socket_interface);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
