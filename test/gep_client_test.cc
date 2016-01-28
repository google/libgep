#include "gep_client.h"

#include <string.h>  // for memcpy
#include <unistd.h>  // for write
#include <string>  // for string

#include "gep_channel.h"  // for GepChannel
#include "gep_channel_array.h"  // for GepChannelArray
#include "gep_client.h"  // for GepClient
#include "gep_test_lib.h"  // for TestServer, etc
#include "gtest/gtest.h"  // for EXPECT_EQ, ASSERT_TRUE, etc
#include "utils.h"  // for SET_UINT32


class GepClientTest : public GepTest {
};

TEST_F(GepClientTest, ClientReconnect) {
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

TEST_F(GepClientTest, ClientReconnectOnGarbageData) {
  GepChannel *gc = client_->GetGepChannel();
  // initially we are connected
  EXPECT_NE(-1, gc->GetSocket());
  EXPECT_EQ(0, client_->GetReconnectCount());
  // get server socket so we can send the invalid data
  int server_socket = server_->GetGepChannelArray()->GetVectorSocket(0);
  // send an invalid message
  int size_msg = sizeof(kInvalidMessage) - 1;  // do not send the '\0'
  EXPECT_EQ(size_msg, write(server_socket, kInvalidMessage, size_msg));
  // check that the client reconnected
  ASSERT_TRUE(WaitForTrue([=]() {return client_->GetReconnectCount() >= 1;}));
  EXPECT_NE(-1, gc->GetSocket());
}

TEST_F(GepClientTest, ClientReconnectOnHugeMessageData) {
  GepChannel *gc = client_->GetGepChannel();
  // initially we are connected
  EXPECT_NE(-1, gc->GetSocket());
  EXPECT_EQ(0, client_->GetReconnectCount());
  // get server socket so we can send the invalid data
  int server_socket = server_->GetGepChannelArray()->GetVectorSocket(0);
  // send a huge message
  int size_msg = sizeof(kHugeInvalidMessage) - 1;  // do not send the '\0'
  EXPECT_EQ(size_msg, write(server_socket, kHugeInvalidMessage, size_msg));
  // check that the client reconnected
  ASSERT_TRUE(WaitForTrue([=]() {return client_->GetReconnectCount() >= 1;}));
  EXPECT_NE(-1, gc->GetSocket());
  // ensure the server is seeing the client
  ASSERT_TRUE(WaitForTrue([=]() {return server_->GetNumClients() == 1;}));
  // push message in the server
  server_->Send(command3_);

  WaitForSync(1);
}

TEST_F(GepClientTest, ClientRestart) {
  GepChannel *gc = client_->GetGepChannel();
  // initially we are connected
  ASSERT_NE(-1, gc->GetSocket());
  for (int i = 0; i < 20; ++i) {
    // stop the client
    client_->Stop();
    // ensure the client is stopped
    ASSERT_FALSE(client_->GetThreadCtrl());
    // restart the client
    ASSERT_EQ(0, client_->Start());
    // ensure the server has seen the client
    ASSERT_TRUE(WaitForTrue([=]() {return server_->GetNumClients() != 0;}));
    // check that the client reconnected
    ASSERT_TRUE(WaitForTrue([=]() {return gc->GetSocket() != -1;}));
  }
}

TEST_F(GepClientTest, ClientDropUnsupportedMessage) {
  GepChannel *gc = client_->GetGepChannel();
  // initially we are connected
  EXPECT_NE(-1, gc->GetSocket());
  EXPECT_EQ(0, client_->GetReconnectCount());
  // get server socket so we can send the unknown message
  int server_socket = server_->GetGepChannelArray()->GetVectorSocket(0);
  // send an unknown message
  int size_msg = sizeof(kUnsupportedMessage) - 1;  // do not send the '\0'
  EXPECT_EQ(size_msg, write(server_socket, kUnsupportedMessage, size_msg));
  // push a message in the client
  gc->SendMessage(command1_);
  // push another message in the server
  server_->Send(command3_);
  // ensure we did not reconnect
  EXPECT_EQ(0, client_->GetReconnectCount());
  WaitForSync(2);
}

TEST_F(GepClientTest, ClientDropUnsupportedMagicMessage) {
  GepChannel *gc = client_->GetGepChannel();
  // initially we are connected
  EXPECT_NE(-1, gc->GetSocket());
  EXPECT_EQ(0, client_->GetReconnectCount());
  // get server socket so we can send the unknown message
  int server_socket = server_->GetGepChannelArray()->GetVectorSocket(0);
  // send a message with an unsupported magic
  int size_msg = sizeof(kInvalidMagic) - 1;  // do not send the '\0'
  EXPECT_EQ(size_msg, write(server_socket, kInvalidMagic, size_msg));
  // ensure we reconnect (invalid magic IDs cause the connection to reset)
  ASSERT_TRUE(WaitForTrue([=]() {return client_->GetReconnectCount() == 1;}));
  // push a message in the client
  gc->SendMessage(command1_);
  // push another message in the server
  server_->Send(command3_);
  WaitForSync(1);
}

TEST_F(GepClientTest, ClientSupportsFragmentation) {
  GepChannel *gc = client_->GetGepChannel();
  // initially we are connected
  EXPECT_NE(-1, gc->GetSocket());
  EXPECT_EQ(0, client_->GetReconnectCount());
  // get server socket so we can send the unknown message
  int server_socket = server_->GetGepChannelArray()->GetVectorSocket(0);
  // push several valid messages in the client
  char kSeveralMessages[1024];
  int bi = 0;
  // start with an unsupported message
  int size_msg = sizeof(kUnsupportedMessage) - 1;  // do not send the '\0'
  memcpy(kSeveralMessages + bi, kUnsupportedMessage, size_msg);
  bi += size_msg;
  // then add several supported messages
  int total = 10;
  int hdr_size_msg = sizeof(kRawCommand4Header) - 1;  // do not send '\0'
  int body_size_msg = command4_str_.length();
  for (int i = 0; i < total; ++i) {
    memcpy(kSeveralMessages + bi, kRawCommand4Header, hdr_size_msg);
    bi += hdr_size_msg;
    SET_UINT32(kSeveralMessages + bi, body_size_msg);
    bi += 4;
    memcpy(kSeveralMessages + bi, command4_str_.c_str(), body_size_msg);
    bi += body_size_msg;
  }
  EXPECT_EQ(bi, write(server_socket, kSeveralMessages, bi));

  // push another message in the client
  gc->SendMessage(command1_);

  // push message in the server
  server_->Send(command3_);

  WaitForSync(total + 2);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
