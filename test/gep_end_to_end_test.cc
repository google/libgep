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
#include "gep_test_lib.h"
#ifndef GEP_LITE
#include "test.pb.h"
#else
#include "test_lite.pb.h"
#endif
#include "test_protocol.h"

using namespace libgep_utils;

const int kNumWriters = 20;

class GepEndToEndTest : public GepTest {
 public:
  void SenderPusherThread();
  void SenderLockThread();
};

TEST_F(GepEndToEndTest, BasicEndToEnd) {
  // push message in the client
  client_->Send(command1_);

  // push message in the server
  server_->Send(command3_);

  WaitForSync(2);
}

TEST_F(GepEndToEndTest, ExplicitEndToEnd) {
  // push message in the server to an explicit client
  int id = server_->GetGepChannelArray()->GetClientId(0);
  server_->Send(command3_, id);

  WaitForSync(1);
}

TEST_F(GepEndToEndTest, CallbackFailure) {
  // push message in the client
  client_->Send(command2_);

  WaitForSync(1);
}

void GepEndToEndTest::SenderPusherThread() {
  // wait for the ready signal
  while (!ready_)
    std::this_thread::yield();

  // push message in the server
  server_->Send(command3_);
}

TEST_F(GepEndToEndTest, ParallelEndToEnd) {
  // push message in the client
  client_->Send(command1_);

  // start the sender pusher threads
  ready_ = false;
  std::vector<std::thread> threads;
  for (int i = 0; i < kNumWriters; ++i)
    threads.push_back(std::thread(&GepEndToEndTest::SenderPusherThread, this));
  ready_ = true;

  // wait for all the sender pushers to finish
  for (auto &th : threads)
    th.join();

  WaitForSync(1 + kNumWriters);
  EXPECT_EQ(0, client_->GetReconnectCount());
}

TEST_F(GepEndToEndTest, EndToEndDifferentMagic) {
  // use a different magic number
  uint32_t new_magic = MakeTag('r', 'f', 'l', 'a');
  cproto_->SetMagic(new_magic);
  sproto_->SetMagic(new_magic);

  // push message in the client
  client_->Send(command1_);

  // push message in the server
  server_->Send(command3_);

  // ensure we did not reconnect
  EXPECT_EQ(0, client_->GetReconnectCount());
  WaitForSync(2);
}

TEST_F(GepEndToEndTest, MultipleMessagesAreAllReceived) {
  // push 2x messages in the client
  client_->Send(command1_);
  client_->Send(command1_);

  // push message in the server
  server_->Send(command3_);

  // note that WaitForSync() will wait for 3 messages or fail
  WaitForSync(3);
}

TEST_F(GepEndToEndTest, CallbackDeadlock) {
  // push message in the client
  client_->Send(control_message_ping_);

  WaitForSync(2);
}

void GepEndToEndTest::SenderLockThread() {
  // ensure the sender locks the non-GEP mutex
  std::lock_guard<std::mutex> lock(non_gep_lock_);

  // wait for the server thread to get into the callback
  while (!stage1_)
    std::this_thread::yield();

  // let the server thread proceed
  stage2_ = true;

  // push any message in the server
  server_->Send(command3_);
}

TEST_F(GepEndToEndTest, CallbackCrossed) {
  // init sync mechanism
  stage1_ = false;
  stage2_ = false;

  // push message in the client
  client_->Send(control_message_get_lock_);

  // start the sender thread
  auto th = std::thread(&GepEndToEndTest::SenderLockThread, this);

  // wait for all the sender thread to finish
  if (WaitForSync(1))
    th.join();
}

TEST_F(GepEndToEndTest, EndToEndBinaryProtocol) {
  // use binary mode
  cproto_->SetMode(GepProtocol::MODE_BINARY);
  sproto_->SetMode(GepProtocol::MODE_BINARY);

  // push message in the client
  client_->Send(command1_);

  // push message in the server
  server_->Send(command3_);

  // ensure we did not reconnect
  EXPECT_EQ(0, client_->GetReconnectCount());
  WaitForSync(2);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
