#include "gtest/gtest.h"

#include <algorithm>
#include <atomic>
#include <gep_protocol.h>
#include <gep_client.h>
#include <gep_server.h>
#include <gep_utils.h>
#include <mutex>
#include <stddef.h>  // for NULL
#include <string>  // for string
#include <unistd.h>  // for usleep

#include "../src/utils.h"
#ifndef GEP_LITE
#include "test.pb.h"
#else
#include "test_lite.pb.h"
#endif
#include "test_protocol.h"

using namespace libgep_utils;

const int kMaxChannels = 8;

const int kNumWriters = 20;

const char kRawCommand4Header[] = "geppcmd4";
const char kInvalidMessage[] = "geppcmd3\000\000\000\001x";
const char kHugeInvalidMessage[] = "geppcmd3\377\377\377\377yy";
const char kUnsupportedMessage[] = "geppxyza\000\000\000\001x";
const char kInvalidMagic[] = "abcdcmd4\000\000\000\015id: 123456789";

static Command1 kOriginalCommand1;
static Command3 kOriginalCommand3;
static Command4 kOriginalCommand4;
static ControlMessage kControlMessagePing;
static ControlMessage kControlMessagePong;
static ControlMessage kControlMessageGetLock;

static std::string kOriginalCommand1Str;
static std::string kOriginalCommand3Str;
static std::string kOriginalCommand4Str;
static std::string kControlMessagePingStr;
static std::string kControlMessagePongStr;
static std::string kControlMessageGetLockStr;

class TestServer : public GepServer {
 public:
  TestServer(const std::string &name, int max_channels, void *context,
             GepProtocol *proto, const GepVFT* ops)
    : GepServer(name, max_channels, context, proto, ops) {
  }

  virtual int Start() {
    ids_.clear();
    return GepServer::Start();
  }

  std::vector<int> ids_;

  virtual void AddClient(int id) {
    ids_.push_back(id);
  }
  virtual void DelClient(int id) {
    auto it = find(ids_.begin(), ids_.end(), id);
    // ensure the client exists
    EXPECT_NE(ids_.end(), it);
    // remove it
    ids_.erase(it);
  }
};

class GepTest : public ::testing::Test {
 public:
  static void DoSync();
  void SenderPusherThread();
  void SenderLockThread();

  // protocol object callbacks: These are object (non-static) callback
  // methods, which is handy for the callers.
  virtual bool Recv(const Command1 &msg, int id);
  virtual bool Recv(const Command3 &msg, int id);
  virtual bool Recv(const Command4 &msg, int id);
  virtual bool Recv(const ControlMessage &msg, int id);

  // maximum wait for any busy loop
  static int64_t kWaitTimeoutUsecs;

 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

  void SetUp();
  void TearDown();
  static bool WaitForSync(int number);

  TestProtocol *cproto_;
  TestProtocol *sproto_;

  TestServer *server_;
  GepClient *client_;
  static void *context_;

  // ready flag
  static std::atomic<bool> ready_;

  // other sync flags
  static std::atomic<bool> stage1_;
  static std::atomic<bool> stage2_;

  // lock for the CallbackCrossed test
  std::mutex non_gep_lock_;

 private:
  static std::atomic<int> synced_;
};

int64_t GepTest::kWaitTimeoutUsecs = secs_to_usecs(6);
void* GepTest::context_ = nullptr;
std::atomic<int> GepTest::synced_(0);
std::atomic<bool> GepTest::ready_(false);
std::atomic<bool> GepTest::stage1_(false);
std::atomic<bool> GepTest::stage2_(false);

const GepVFT kGepTestOps = {
  {TestProtocol::MSG_TAG_COMMAND_1, &RecvMessageId<GepTest, Command1>},
  {TestProtocol::MSG_TAG_COMMAND_3, &RecvMessageId<GepTest, Command3>},
  {TestProtocol::MSG_TAG_COMMAND_4, &RecvMessageId<GepTest, Command4>},
  {TestProtocol::MSG_TAG_CONTROL, &RecvMessageId<GepTest, ControlMessage>},
};

void GepTest::DoSync() {
  synced_++;
}

bool GepTest::Recv(const Command1 &msg, int id) {
  // check the msg received in the client
  EXPECT_TRUE(ProtobufEqual(kOriginalCommand1, msg));
  DoSync();
  return true;
}

bool GepTest::Recv(const Command3 &msg, int id) {
  // check the msg received in the client
  EXPECT_TRUE(ProtobufEqual(kOriginalCommand3, msg));
  DoSync();
  return true;
}

bool GepTest::Recv(const Command4 &msg, int id) {
  // check the msg received in the client
  EXPECT_TRUE(ProtobufEqual(kOriginalCommand4, msg));
  DoSync();
  return true;
}

bool GepTest::Recv(const ControlMessage &msg, int id) {
  // check the msg received in the server
  if (ProtobufEqual(kControlMessagePing, msg)) {
    // send pong message
    GepChannelArray *gca = server_->GetGepChannelArray();
    gca->SendMessage(kControlMessagePong);
  } else if (ProtobufEqual(kControlMessageGetLock, msg)) {
    // let the main thread know the server thread is in the callback
    stage1_ = true;
    // wait until the server thread allows us to proceed
    while (!stage2_)
      std::this_thread::yield();
    // get the non-GEP mutex
    non_gep_lock_.lock();
    // release the non-GEP mutex
    non_gep_lock_.unlock();
  }
  DoSync();
  return true;
}

bool GepTest::WaitForSync(int number) {
  // ensure both sides can receive their messages
  int64_t max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
  while (synced_ < number && GetUnixTimeUsec() < max_time)
    usleep(1000);
  // ensure we did not timeout
  EXPECT_GE(synced_, number);
  return (synced_ >= number);
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
  server_ = new TestServer("gep_test_server", kMaxChannels, context_, sproto_,
                           &kGepTestOps);
  ASSERT_TRUE(server_);

  // create GEP client
  client_ = new GepClient("gep_test_client", context_, cproto_, &kGepTestOps);
  ASSERT_TRUE(client_);

  // reset sync flag
  synced_ = 0;

  // create some messages
  kOriginalCommand1.set_a(0xaaaaaaaaaaaaaaaa);
  kOriginalCommand1.set_b(0xbbbbbbbb);
  kOriginalCommand3.set_id(123456789);
  kOriginalCommand4.set_id(123456789);
  kControlMessagePing.set_command(ControlMessage::COMMAND_PING);
  kControlMessagePong.set_command(ControlMessage::COMMAND_PONG);
  kControlMessageGetLock.set_command(ControlMessage::COMMAND_GET_LOCK);

  // create text versions
  EXPECT_TRUE(cproto_->Serialize(kOriginalCommand1, &kOriginalCommand1Str));
  EXPECT_TRUE(cproto_->Serialize(kOriginalCommand3, &kOriginalCommand3Str));
  EXPECT_TRUE(cproto_->Serialize(kOriginalCommand4, &kOriginalCommand4Str));
  EXPECT_TRUE(cproto_->Serialize(kControlMessagePing, &kControlMessagePingStr));
  EXPECT_TRUE(cproto_->Serialize(kControlMessagePong, &kControlMessagePongStr));
  EXPECT_TRUE(cproto_->Serialize(kControlMessageGetLock,
                                 &kControlMessageGetLockStr));

  // start server in a random port
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
  // ensure all clients have been deleted
  EXPECT_TRUE(server_->ids_.empty());
  // delete them
  delete client_;
  delete server_;
}

TEST_F(GepTest, Serialize) {
  // use the client GepChannel to test Serialize
  static Command1 empty_command1;
  struct gep_serialize_test {
    int line;
    Command1 *command1;
    std::string expected_command1_str;
  } test_arr[] = {
    {__LINE__, &kOriginalCommand1, kOriginalCommand1Str},
    {__LINE__, &empty_command1, ""},
  };

  for (const auto &test_item : test_arr) {
    std::string capture_str;
    EXPECT_TRUE(cproto_->Serialize(*test_item.command1, &capture_str)) <<
        "Error on line " << test_item.line;
    EXPECT_STREQ(test_item.expected_command1_str.c_str(),
                 capture_str.c_str()) <<
        "Error on line " << test_item.line;
  }
}

TEST_F(GepTest, Unserialize) {
  // use the client GepChannel to test Unserialize
  static Command1 empty_command1;
  struct gep_unserialize_test {
    int line;
    bool success;
    Command1 *expected_command1;
    std::string command1_str;
  } test_arr[] = {
    {__LINE__, true, &kOriginalCommand1, kOriginalCommand1Str},
    {__LINE__, true, &empty_command1, ""},
    // invalid message
    {__LINE__, false, &empty_command1, "invalid text protobuf"},
  };

  for (const auto &test_item : test_arr) {
    Command1 msg;
    EXPECT_EQ(test_item.success,
              cproto_->Unserialize(test_item.command1_str, &msg)) <<
        "Error on line " << test_item.line;
    if (test_item.success)
      EXPECT_TRUE(ProtobufEqual(*test_item.expected_command1, msg)) <<
          "Error on line " << test_item.line;
  }
}

TEST_F(GepTest, EndToEnd) {
  // push message in the client
  GepChannel *gc = client_->GetGepChannel();
  gc->SendMessage(kOriginalCommand1);

  // push message in the server
  GepChannelArray *gca = server_->GetGepChannelArray();
  gca->SendMessage(kOriginalCommand3);

  WaitForSync(2);
}

void GepTest::SenderPusherThread() {
  // wait for the ready signal
  while (!ready_)
    std::this_thread::yield();

  // push message in the server
  GepChannelArray *gca = server_->GetGepChannelArray();
  gca->SendMessage(kOriginalCommand3);
}

TEST_F(GepTest, ParallelEndToEnd) {
  // push message in the client
  GepChannel *gc = client_->GetGepChannel();
  gc->SendMessage(kOriginalCommand1);

  // start the sender pusher threads
  ready_ = false;
  std::vector<std::thread> threads;
  for (int i = 0; i < kNumWriters; ++i)
    threads.push_back(std::thread(&GepTest::SenderPusherThread, this));
  ready_ = true;

  // wait for all the sender pushers to finish
  for (auto &th : threads)
    th.join();

  WaitForSync(1 + kNumWriters);
  EXPECT_EQ(0, client_->GetReconnectCount());
}

TEST_F(GepTest, ClientReconnect) {
  GepChannel *gc = client_->GetGepChannel();
  // initially we are connected
  EXPECT_NE(-1, gc->GetSocket());
  // stop the server
  server_->Stop();
  // check that the client channel becomes disconnected
  int64_t max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
  while (gc->GetSocket() != -1 && GetUnixTimeUsec() < max_time)
    usleep(1000);
  EXPECT_EQ(-1, gc->GetSocket());
  // restart the server
  server_->Start();
  // check that the client reconnected
  max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
  while (gc->GetSocket() == -1 && GetUnixTimeUsec() < max_time)
    usleep(1000);
  EXPECT_NE(-1, gc->GetSocket());
}

TEST_F(GepTest, ClientReconnectOnGarbageData) {
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
  int64_t max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
  while (client_->GetReconnectCount() < 1 && GetUnixTimeUsec() < max_time)
    usleep(1000);
  // ensure we did not timeout
  EXPECT_LE(1, client_->GetReconnectCount());
  EXPECT_NE(-1, gc->GetSocket());
}

TEST_F(GepTest, ClientReconnectOnHugeMessageData) {
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
  int64_t max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
  while (client_->GetReconnectCount() < 1 && GetUnixTimeUsec() < max_time)
    usleep(1000);
  // ensure we did not timeout
  EXPECT_LE(1, client_->GetReconnectCount());
  EXPECT_NE(-1, gc->GetSocket());
  // ensure the server is seeing the client
  max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
  while (server_->GetNumClients() == 0 && GetUnixTimeUsec() < max_time)
    usleep(1000);
  EXPECT_EQ(1, server_->GetNumClients());
  // push message in the server
  GepChannelArray *gca = server_->GetGepChannelArray();
  gca->SendMessage(kOriginalCommand3);

  WaitForSync(1);
}

TEST_F(GepTest, ClientRestart) {
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
    int64_t max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
    while (server_->GetNumClients() == 0 && GetUnixTimeUsec() < max_time)
      usleep(1000);
    // ensure we did not timeout
    ASSERT_NE(0, server_->GetNumClients());
    // check that the client reconnected
    max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
    while (gc->GetSocket() == -1 && GetUnixTimeUsec() < max_time)
      usleep(1000);
    ASSERT_NE(-1, gc->GetSocket());
  }
}

TEST_F(GepTest, DropUnsupportedMessage) {
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
  gc->SendMessage(kOriginalCommand1);
  // push another message in the server
  GepChannelArray *gca = server_->GetGepChannelArray();
  gca->SendMessage(kOriginalCommand3);
  // ensure we did not reconnect
  usleep(1000);
  EXPECT_EQ(0, client_->GetReconnectCount());
  WaitForSync(2);
}

TEST_F(GepTest, EndToEndDifferentMagic) {
  // use a different magic number
  uint32_t new_magic = MakeTag('r', 'f', 'l', 'a');
  cproto_->SetMagic(new_magic);
  sproto_->SetMagic(new_magic);

  // push message in the client
  GepChannel *gc = client_->GetGepChannel();
  gc->SendMessage(kOriginalCommand1);

  // push message in the server
  GepChannelArray *gca = server_->GetGepChannelArray();
  gca->SendMessage(kOriginalCommand3);

  // ensure we did not reconnect
  usleep(1000);
  EXPECT_EQ(0, client_->GetReconnectCount());
  WaitForSync(2);
}

TEST_F(GepTest, DropUnsupportedMagicMessage) {
  GepChannel *gc = client_->GetGepChannel();
  // initially we are connected
  EXPECT_NE(-1, gc->GetSocket());
  EXPECT_EQ(0, client_->GetReconnectCount());
  // get server socket so we can send the unknown message
  int server_socket = server_->GetGepChannelArray()->GetVectorSocket(0);
  // send a message with an unsupported magic
  int size_msg = sizeof(kInvalidMagic) - 1;  // do not send the '\0'
  EXPECT_EQ(size_msg, write(server_socket, kInvalidMagic, size_msg));
  // wait until we reconnect (invalid magic IDs cause the connection
  // to reset)
  while (client_->GetReconnectCount() == 0)
    usleep(1000);
  EXPECT_EQ(1, client_->GetReconnectCount());
  // push a message in the client
  gc->SendMessage(kOriginalCommand1);
  // push another message in the server
  GepChannelArray *gca = server_->GetGepChannelArray();
  gca->SendMessage(kOriginalCommand3);
  WaitForSync(1);
}

TEST_F(GepTest, MultipleMessagesAreAllReceived) {
  // push 2x messages in the client
  GepChannel *gc = client_->GetGepChannel();
  gc->SendMessage(kOriginalCommand1);
  gc->SendMessage(kOriginalCommand1);

  // push message in the server
  GepChannelArray *gca = server_->GetGepChannelArray();
  gca->SendMessage(kOriginalCommand3);

  // note that WaitForSync() will wait for 3 messages or fail
  WaitForSync(3);
}

TEST_F(GepTest, Fragmentation) {
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
  int body_size_msg = kOriginalCommand4Str.length();
  for (int i = 0; i < total; ++i) {
    memcpy(kSeveralMessages + bi, kRawCommand4Header, hdr_size_msg);
    bi += hdr_size_msg;
    SET_UINT32(kSeveralMessages + bi, body_size_msg);
    bi += 4;
    memcpy(kSeveralMessages + bi, kOriginalCommand4Str.c_str(), body_size_msg);
    bi += body_size_msg;
  }
  EXPECT_EQ(bi, write(server_socket, kSeveralMessages, bi));

  // push another message in the client
  gc->SendMessage(kOriginalCommand1);
  gc->SendMessage(kOriginalCommand1);

  // push message in the server
  GepChannelArray *gca = server_->GetGepChannelArray();
  gca->SendMessage(kOriginalCommand3);

  WaitForSync(total + 2);
}

TEST_F(GepTest, CallbackDeadlock) {
  // push message in the client
  GepChannel *gc = client_->GetGepChannel();
  gc->SendMessage(kControlMessagePing);

  WaitForSync(2);
}

void GepTest::SenderLockThread() {
  // ensure the sender locks the non-GEP mutex
  std::lock_guard<std::mutex> lock(non_gep_lock_);

  // wait for the server thread to get into the callback
  while (!stage1_)
    std::this_thread::yield();

  // let the server thread proceed
  stage2_ = true;

  // push any message in the server
  GepChannelArray *gca = server_->GetGepChannelArray();
  gca->SendMessage(kOriginalCommand3);
}

TEST_F(GepTest, CallbackCrossed) {
  // init sync mechanism
  stage1_ = false;
  stage2_ = false;

  // push message in the client
  GepChannel *gc = client_->GetGepChannel();
  gc->SendMessage(kControlMessageGetLock);

  // start the sender thread
  auto th = std::thread(&GepTest::SenderLockThread, this);

  // wait for all the sender thread to finish
  if (WaitForSync(1))
    th.join();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST_F(GepTest, EndToEndBinaryProtocol) {
  // use binary mode
  cproto_->SetMode(GepProtocol::MODE_BINARY);
  sproto_->SetMode(GepProtocol::MODE_BINARY);

  // push message in the client
  GepChannel *gc = client_->GetGepChannel();
  gc->SendMessage(kOriginalCommand1);

  // push message in the server
  GepChannelArray *gca = server_->GetGepChannelArray();
  gca->SendMessage(kOriginalCommand3);

  // ensure we did not reconnect
  usleep(1000);
  EXPECT_EQ(0, client_->GetReconnectCount());
  WaitForSync(2);
}
