// Copyright Google Inc. Apache 2.0.

#include <stdio.h>  // for remove
#include <string.h>  // for memset
#include <unistd.h>  // for usleep

#include "gep_server.h"
#include "gep_test_lib.h"
#include "test_protocol.h"

#include "gtest/gtest.h"

class GepServerTest : public GepTest {
};

TEST_F(GepServerTest, ServerReconnect) {
  GepChannel *gc = client_->GetGepChannel();
  // initially we are connected
  EXPECT_NE(-1, gc->GetSocket());
  // stop the server
  server_->Stop();
  // check that the client channel becomes disconnected
  ASSERT_TRUE(WaitForTrue([=]() {return gc->GetSocket() == -1;}));
  // restart the server
  server_->Start();
  // check that the client reconnected
  ASSERT_TRUE(WaitForTrue([=]() {return gc->GetSocket() != -1;}));
}

TEST_F(GepServerTest, ServerDropUnsupportedMessage) {
  GepChannel *gc = client_->GetGepChannel();
  // initially we are connected
  EXPECT_NE(-1, gc->GetSocket());
  EXPECT_EQ(0, client_->GetReconnectCount());
  // get client socket so we can send the unknown message
  int client_socket = gc->GetSocket();
  // send an unknown message
  int size_msg = sizeof(kUnsupportedMessage) - 1;  // do not send the '\0'
  EXPECT_EQ(size_msg, write(client_socket, kUnsupportedMessage, size_msg));
  // push another message in the client
  gc->SendMessage(command1_);
  // push a message in the server
  GepChannelArray *gca = server_->GetGepChannelArray();
  gca->SendMessage(command3_);
  // ensure we did not reconnect
  EXPECT_EQ(0, client_->GetReconnectCount());
  WaitForSync(2);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
