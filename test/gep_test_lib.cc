// Copyright Google Inc. Apache 2.0.

#include "gep_test_lib.h"
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


const int kMaxChannels = 8;

int64_t GepTest::kWaitTimeoutUsecs = secs_to_usecs(6);
void* GepTest::context_ = nullptr;
std::atomic<int> GepTest::synced_(0);
std::atomic<bool> GepTest::ready_(false);
std::atomic<bool> GepTest::stage1_(false);
std::atomic<bool> GepTest::stage2_(false);

void GepTest::DoSync() {
  synced_++;
}

bool GepTest::Recv(const Command1 &msg, int id) {
  // check the msg received in the client
  EXPECT_TRUE(ProtobufEqual(rcommand1_, msg));
  DoSync();
  return true;
}

bool GepTest::Recv(const Command2 &msg, int id) {
  // check the msg received in the client
  EXPECT_TRUE(ProtobufEqual(rcommand2_, msg));
  DoSync();
  return false;
}

bool GepTest::Recv(const Command3 &msg, int id) {
  // check the msg received in the client
  EXPECT_TRUE(ProtobufEqual(rcommand3_, msg));
  DoSync();
  return true;
}

bool GepTest::Recv(const Command4 &msg, int id) {
  // check the msg received in the client
  EXPECT_TRUE(ProtobufEqual(rcommand4_, msg));
  DoSync();
  return true;
}

bool GepTest::Recv(const ControlMessage &msg, int id) {
  // check the msg received in the server
  if (ProtobufEqual(rcontrol_message_ping_, msg)) {
    // send pong message
    GepChannelArray *gca = server_->GetGepChannelArray();
    gca->SendMessage(rcontrol_message_pong_);
  } else if (ProtobufEqual(rcontrol_message_get_lock_, msg)) {
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

bool GepTest::WaitForTrue(std::function<bool()> fun) {
  int64_t max_time = GetUnixTimeUsec() + kWaitTimeoutUsecs;
  bool result = fun();
  while (!result && GetUnixTimeUsec() < max_time) {
    usleep(1000);
    result = fun();
  }
  return result;
}

bool GepTest::WaitForSync(int number) {
  return WaitForTrue([=]() {return GetSynced() >= number;});
}

void GepTest::SetUpTestCase() {
}

void GepTest::TearDownTestCase() {
  // clean up protobuf library
  google::protobuf::ShutdownProtobufLibrary();
}

void GepTest::InitData() {
  // create some messages
  command1_.set_a(0xaaaaaaaaaaaaaaaa);
  command1_.set_b(0xbbbbbbbb);
  command3_.set_id(123456789);
  command4_.set_id(123456789);
  control_message_ping_.set_command(ControlMessage::COMMAND_PING);
  control_message_pong_.set_command(ControlMessage::COMMAND_PONG);
  control_message_get_lock_.set_command(ControlMessage::COMMAND_GET_LOCK);

  // copy them
  rcommand1_ = command1_;
  rcommand2_ = command2_;
  rcommand3_ = command3_;
  rcommand4_ = command4_;
  rcontrol_message_ping_ = control_message_ping_;
  rcontrol_message_pong_ = control_message_pong_;
  rcontrol_message_get_lock_ = control_message_get_lock_;

  // add text version
  TestProtocol proto(0);
  EXPECT_TRUE(proto.Serialize(command1_, &command1_str_));
  EXPECT_TRUE(proto.Serialize(command3_, &command3_str_));
  EXPECT_TRUE(proto.Serialize(command4_, &command4_str_));
  EXPECT_TRUE(proto.Serialize(control_message_ping_,
                              &control_message_ping_str_));
  EXPECT_TRUE(proto.Serialize(control_message_pong_,
                              &control_message_pong_str_));
  EXPECT_TRUE(proto.Serialize(control_message_get_lock_,
                              &control_message_get_lock_str_));
}

void GepTest::SetUp() {
  InitData();

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

  // start server in a random port
  server_->GetProto()->SetPort(0);
  EXPECT_EQ(0, server_->Start());
  int port = server_->GetProto()->GetPort();
  EXPECT_GT(port, 0);

  // start client
  client_->GetProto()->SetPort(port);
  EXPECT_EQ(0, client_->Start());

  // ensure the server has seen the client
  ASSERT_TRUE(WaitForTrue([=]() {return server_->GetNumClients() != 0;}));
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
