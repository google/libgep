#include "gtest/gtest.h"

#include <atomic>
#include <stddef.h>  // for NULL
#include <string>  // for string
#include <unistd.h>  // for usleep

#include "gep_protocol.h"
#include "gep_server.h"
#include "gep_client.h"
#include "gep_utils.h"
#include "test.pb.h"
#include "test_protocol.h"
#include "utils.h"

using namespace libgep_utils;

const int kMaxChannels = 8;

const int kNumWriters = 20;

const char kOriginalCommand1Str[] = R"EOF(a: 1111111111111111
b: 17688477
)EOF";

const char kOriginalCommand3Str[] = R"EOF(id: 123456789
)EOF";

const char kOriginalCommand4Str[] = R"EOF(id: 123456789
)EOF";
const char kRawCommand4[] = "geppcmd4\000\000\000\015id: 123456789";

const char kControlMessagePingStr[] = R"EOF(
command: COMMAND_PING
)EOF";

const char kControlMessagePongStr[] = R"EOF(
command: COMMAND_PONG
)EOF";

const char kInvalidMessage[] = "geppcmd3\000\000\000\001x";
const char kHugeInvalidMessage[] = "geppcmd3\377\377\377\377yy";
const char kUnsupportedMessage[] = "geppxyza\000\000\000\001x";

static Command1 kOriginalCommand1;
static Command3 kOriginalCommand3;
static Command4 kOriginalCommand4;
static ControlMessage kControlMessagePing;
static ControlMessage kControlMessagePong;

class GepTest : public ::testing::Test {
 public:
  static void DoSync();
  void ServerPusherThread();

  // protocol object callbacks: These are object (non-static) callback
  // methods, which is handy for the callers.
  virtual bool Recv(const Command1 &msg);
  virtual bool Recv(const Command3 &msg);
  virtual bool Recv(const Command4 &msg);
  virtual bool Recv(const ControlMessage &msg);

  // maximum wait for any busy loop
  static int64_t kWaitTimeoutUsecs;

 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

  void SetUp();
  void TearDown();
  static void WaitForSync(int number);

  TestProtocol *cproto_;
  TestProtocol *sproto_;

  GepServer *server_;
  GepClient *client_;
  static void *context_;

  // ready flag
  static std::atomic<bool> ready_;

 private:
  static std::atomic<int> synced_;
};

int64_t GepTest::kWaitTimeoutUsecs = secs_to_usecs(6);
void* GepTest::context_ = nullptr;
std::atomic<int> GepTest::synced_(0);
std::atomic<bool> GepTest::ready_(false);

const GepVFT kGepTestOps = {
  {TestProtocol::MSG_TAG_COMMAND_1, &RecvMessage<GepTest, Command1>},
  {TestProtocol::MSG_TAG_COMMAND_3, &RecvMessage<GepTest, Command3>},
  {TestProtocol::MSG_TAG_COMMAND_4, &RecvMessage<GepTest, Command4>},
  {TestProtocol::MSG_TAG_CONTROL, &RecvMessage<GepTest, ControlMessage>},
};

void GepTest::DoSync() {
  synced_++;
}

bool GepTest::Recv(const Command1 &msg) {
  // check the msg received in the client
  EXPECT_TRUE(ProtobufEqual(kOriginalCommand1, msg));
  DoSync();
  return true;
}

bool GepTest::Recv(const Command3 &msg) {
  // check the msg received in the client
  EXPECT_TRUE(ProtobufEqual(kOriginalCommand3, msg));
  DoSync();
  return true;
}

bool GepTest::Recv(const Command4 &msg) {
  // check the msg received in the client
  EXPECT_TRUE(ProtobufEqual(kOriginalCommand4, msg));
  DoSync();
  return true;
}

bool GepTest::Recv(const ControlMessage &msg) {
  // check the msg received in the server
  if (ProtobufEqual(kControlMessagePing, msg)) {
    // send pong message
    GepChannelArray *eca = server_->GetGepChannelArray();
    eca->SendMessage(kControlMessagePong);
  }
  DoSync();
  return true;
}

void GepTest::WaitForSync(int number) {
  // ensure both sides can receive their messages
  int64_t max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
  while (synced_ < number && GetUnixTimeUsec() < max_time)
    usleep(1000);
  // ensure we did not timeout
  EXPECT_GE(synced_, number);
}

void GepTest::SetUpTestCase() {
}

void GepTest::TearDownTestCase() {
  // clean up protobuf library
  google::protobuf::ShutdownProtobufLibrary();
}

void GepTest::SetUp() {
  // create Test protocol instances
  cproto_ = new TestProtocol(0);
  sproto_ = new TestProtocol(0);

  // ensure the protocol objects answer fast
  cproto_->SetSelectTimeoutUsec(msecs_to_usecs(10));
  sproto_->SetSelectTimeoutUsec(msecs_to_usecs(10));

  context_ = reinterpret_cast<void *>(this);

  // create GEP server
  server_ = new GepServer("gep_test_server", kMaxChannels, context_, sproto_,
                          &kGepTestOps);
  ASSERT_TRUE(server_);

  // create GEP client
  client_ = new GepClient("gep_test_client", context_, cproto_, &kGepTestOps);
  ASSERT_TRUE(client_);

  // reset sync flag
  synced_ = 0;

  // create a Command1 message
  EXPECT_TRUE(cproto_->Unserialize(kOriginalCommand1Str, &kOriginalCommand1));
  // create a Command3 message
  EXPECT_TRUE(cproto_->Unserialize(kOriginalCommand3Str, &kOriginalCommand3));

  // create a Command4 message
  EXPECT_TRUE(cproto_->Unserialize(kOriginalCommand4Str, &kOriginalCommand4));

  // create ControlMessage messages
  EXPECT_TRUE(cproto_->Unserialize(kControlMessagePingStr,
                                   &kControlMessagePing));
  EXPECT_TRUE(cproto_->Unserialize(kControlMessagePongStr,
                                   &kControlMessagePong));

  // start server
  server_->GetProto()->SetPort(0);
  EXPECT_EQ(0, server_->Start());
  int port = server_->GetProto()->GetPort();
  EXPECT_GT(port, 0);

  // start client
  client_->GetProto()->SetPort(port);
  EXPECT_EQ(0, client_->Start());

  // ensure the server has seen the client
  int64_t max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
  while (server_->GetNumClients() == 0 && GetUnixTimeUsec() < max_time)
    usleep(1000);
  // ensure we did not timeout
  EXPECT_NE(server_->GetNumClients(), 0);
}

void GepTest::TearDown() {
  // stop client and server
  client_->Stop();
  server_->Stop();
  // delete them
  delete client_;
  delete server_;
}

TEST_F(GepTest, Serialize) {
  // use the client GepChannel to test Serialize
  std::string capture_str;
  EXPECT_TRUE(cproto_->Serialize(kOriginalCommand1, &capture_str));
  EXPECT_STREQ(kOriginalCommand1Str, capture_str.c_str());
}

TEST_F(GepTest, Unserialize) {
  // use the client GepChannel to test Unserialize
  Command1 msg;
  EXPECT_TRUE(cproto_->Unserialize(kOriginalCommand1Str, &msg));
  EXPECT_TRUE(ProtobufEqual(kOriginalCommand1, msg));
  // try an invalid message
  EXPECT_FALSE(cproto_->Unserialize("invalid text protobuf", &msg));
}

TEST_F(GepTest, EndToEnd) {
  // push message in the client
  GepChannel *ec = client_->GetGepChannel();
  ec->SendMessage(kOriginalCommand1);

  // push message in the server
  GepChannelArray *eca = server_->GetGepChannelArray();
  eca->SendMessage(kOriginalCommand3);

  WaitForSync(2);
}

void GepTest::ServerPusherThread() {
  // wait for the ready signal
  while (!ready_)
    std::this_thread::yield();

  // push message in the server
  GepChannelArray *eca = server_->GetGepChannelArray();
  eca->SendMessage(kOriginalCommand3);
}

TEST_F(GepTest, ParallelEndToEnd) {
  // push message in the client
  GepChannel *ec = client_->GetGepChannel();
  ec->SendMessage(kOriginalCommand1);

  // start the server pusher threads
  ready_ = false;
  std::vector<std::thread> threads;
  for (int i = 0; i < kNumWriters; ++i)
    threads.push_back(std::thread(&GepTest::ServerPusherThread, this));
  ready_ = true;

  // wait for all the clients to finish
  for (auto &th : threads)
    th.join();

  WaitForSync(1 + kNumWriters);
  EXPECT_EQ(0, client_->GetReconnectCount());
}

TEST_F(GepTest, ClientReconnect) {
  GepChannel *ec = client_->GetGepChannel();
  // initially we are connected
  EXPECT_NE(-1, ec->GetSocket());
  // stop the server
  server_->Stop();
  // check that the client channel becomes disconnected
  int64_t max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
  while (ec->GetSocket() != -1 && GetUnixTimeUsec() < max_time)
    usleep(1000);
  EXPECT_EQ(-1, ec->GetSocket());
  // restart the server
  server_->Start();
  // check that the client reconnected
  max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
  while (ec->GetSocket() == -1 && GetUnixTimeUsec() < max_time)
    usleep(1000);
  EXPECT_NE(-1, ec->GetSocket());
}

TEST_F(GepTest, ClientReconnectOnGarbageData) {
  GepChannel *ec = client_->GetGepChannel();
  // initially we are connected
  EXPECT_NE(-1, ec->GetSocket());
  EXPECT_EQ(0, client_->GetReconnectCount());
  // get server socket so we can send the invalid data
  int server_socket = server_->GetGepChannelArray()->GetVectorSocket(0);
  // send an invalid message
  int size_msg = sizeof(kInvalidMessage) - 1;  // do not send the '\0'
  EXPECT_EQ(size_msg, write(server_socket, kInvalidMessage, size_msg));
  // check that the client reconnected
  int64_t max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
  while (client_->GetReconnectCount() < 1 && GetUnixTimeUsec() < max_time)
    usleep(1000);
  // ensure we did not timeout
  EXPECT_LE(1, client_->GetReconnectCount());
  EXPECT_NE(-1, ec->GetSocket());
}

TEST_F(GepTest, ClientReconnectOnHugeMessageData) {
  GepChannel *ec = client_->GetGepChannel();
  // initially we are connected
  EXPECT_NE(-1, ec->GetSocket());
  EXPECT_EQ(0, client_->GetReconnectCount());
  // get server socket so we can send the invalid data
  int server_socket = server_->GetGepChannelArray()->GetVectorSocket(0);
  // send a huge message
  int size_msg = sizeof(kHugeInvalidMessage) - 1;  // do not send the '\0'
  EXPECT_EQ(size_msg, write(server_socket, kHugeInvalidMessage, size_msg));
  // check that the client reconnected
  int64_t max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
  while (client_->GetReconnectCount() < 1 && GetUnixTimeUsec() < max_time)
    usleep(1000);
  // ensure we did not timeout
  EXPECT_LE(1, client_->GetReconnectCount());
  EXPECT_NE(-1, ec->GetSocket());
  // ensure the server is seeing the client
  max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
  while (server_->GetNumClients() == 0 && GetUnixTimeUsec() < max_time)
    usleep(1000);
  EXPECT_EQ(1, server_->GetNumClients());
  // push message in the server
  GepChannelArray *eca = server_->GetGepChannelArray();
  eca->SendMessage(kOriginalCommand3);

  WaitForSync(1);
}

TEST_F(GepTest, ClientRestart) {
  GepChannel *ec = client_->GetGepChannel();
  // initially we are connected
  ASSERT_NE(-1, ec->GetSocket());
  for (int i = 0; i < 20; ++i) {
    // stop the client
    client_->Stop();
    // ensure the client is stopped
    ASSERT_FALSE(client_->GetThreadCtrl());
    // restart the client
    ASSERT_EQ(0, client_->Start());
    // ensure the server has seen the client
    int64_t max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
    while (server_->GetNumClients() == 0 && GetUnixTimeUsec() < max_time)
      usleep(1000);
    // ensure we did not timeout
    ASSERT_NE(0, server_->GetNumClients());
    // check that the client reconnected
    max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
    while (ec->GetSocket() == -1 && GetUnixTimeUsec() < max_time)
      usleep(1000);
    ASSERT_NE(-1, ec->GetSocket());
  }
}

TEST_F(GepTest, DropUnsupportedMessage) {
  GepChannel *ec = client_->GetGepChannel();
  // initially we are connected
  EXPECT_NE(-1, ec->GetSocket());
  EXPECT_EQ(0, client_->GetReconnectCount());
  // get server socket so we can send the unknown message
  int server_socket = server_->GetGepChannelArray()->GetVectorSocket(0);
  // send an unknown message
  int size_msg = sizeof(kUnsupportedMessage) - 1;  // do not send the '\0'
  EXPECT_EQ(size_msg, write(server_socket, kUnsupportedMessage, size_msg));
  // push message in the client
  ec->SendMessage(kOriginalCommand1);
  // push message in the server
  GepChannelArray *eca = server_->GetGepChannelArray();
  eca->SendMessage(kOriginalCommand3);
  // ensure we did not reconnect
  EXPECT_EQ(0, client_->GetReconnectCount());
  WaitForSync(2);
}

TEST_F(GepTest, MultipleMessagesAreAllReceived) {
  // push 2x messages in the client
  GepChannel *ec = client_->GetGepChannel();
  ec->SendMessage(kOriginalCommand1);
  ec->SendMessage(kOriginalCommand1);

  // push message in the server
  GepChannelArray *eca = server_->GetGepChannelArray();
  eca->SendMessage(kOriginalCommand3);

  // note that WaitForSync() will wait for 3 messages or fail
  WaitForSync(3);
}

TEST_F(GepTest, Fragmentation) {
  GepChannel *ec = client_->GetGepChannel();
  // initially we are connected
  EXPECT_NE(-1, ec->GetSocket());
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
  size_msg = sizeof(kRawCommand4) - 1;  // do not send the '\0'
  for (int i = 0; i < total; ++i) {
    memcpy(kSeveralMessages + bi, kRawCommand4, size_msg);
    bi += size_msg;
  }
  EXPECT_EQ(bi, write(server_socket, kSeveralMessages, bi));

  // push another message in the client
  ec->SendMessage(kOriginalCommand1);
  ec->SendMessage(kOriginalCommand1);

  // push message in the server
  GepChannelArray *eca = server_->GetGepChannelArray();
  eca->SendMessage(kOriginalCommand3);

  WaitForSync(total + 2);
}

TEST_F(GepTest, CallbackDeadlock) {
  // push message in the client
  GepChannel *ec = client_->GetGepChannel();
  ec->SendMessage(kControlMessagePing);

  WaitForSync(2);
}
