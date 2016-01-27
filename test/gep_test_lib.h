#ifndef _TEST_GEP_TEST_LIB_H_
#define _TEST_GEP_TEST_LIB_H_

#include "gtest/gtest.h"

#include <algorithm>
#include <atomic>
#include <functional>
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

const char kRawCommand4Header[] = "geppcmd4";
const char kInvalidMessage[] = "geppcmd3\000\000\000\001x";
const char kHugeInvalidMessage[] = "geppcmd3\377\377\377\377yy";
const char kUnsupportedMessage[] = "geppxyza\000\000\000\001x";
const char kInvalidMagic[] = "abcdcmd4\000\000\000\015id: 123456789";


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
  virtual bool Recv(const Command2 &msg, int id);
  virtual bool Recv(const Command3 &msg, int id);
  virtual bool Recv(const Command4 &msg, int id);
  virtual bool Recv(const ControlMessage &msg, int id);

  // maximum wait for any busy loop
  static int64_t kWaitTimeoutUsecs;

  // messages for testing
  Command1 command1_;
  Command2 command2_;
  Command3 command3_;
  Command4 command4_;
  ControlMessage control_message_ping_;
  ControlMessage control_message_pong_;
  ControlMessage control_message_get_lock_;

  // text version
  std::string command1_str_;
  std::string command2_str_;
  std::string command3_str_;
  std::string command4_str_;
  std::string control_message_ping_str_;
  std::string control_message_pong_str_;
  std::string control_message_get_lock_str_;

  void InitData();

 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

  void SetUp();
  void TearDown();
  static bool WaitForSync(int number);
  static bool WaitForTrue(std::function<bool()> fun);

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
  static int GetSynced() { return synced_; }
};

const GepVFT kGepTestOps = {
  {TestProtocol::MSG_TAG_COMMAND_1, &RecvMessageId<GepTest, Command1>},
  {TestProtocol::MSG_TAG_COMMAND_2, &RecvMessageId<GepTest, Command2>},
  {TestProtocol::MSG_TAG_COMMAND_3, &RecvMessageId<GepTest, Command3>},
  {TestProtocol::MSG_TAG_COMMAND_4, &RecvMessageId<GepTest, Command4>},
  {TestProtocol::MSG_TAG_CONTROL, &RecvMessageId<GepTest, ControlMessage>},
};

#endif  // _TEST_GEP_TEST_LIB_H_
