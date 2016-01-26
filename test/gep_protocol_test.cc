#include <stdio.h>  // for remove
#include <string.h>  // for memset
#include <unistd.h>  // for usleep

#include "gep_channel.h"
#include "gep_test_lib.h"
#include "test_protocol.h"

#include "gtest/gtest.h"

class GepProtocolTest : public GepTest {
 public:
  void SetUp();
  void TearDown() {};

  TestProtocol *proto_;
};

void GepProtocolTest::SetUp() {
  InitData();
  // create a Test protocol instance
  proto_ = new TestProtocol(0);
  proto_->SetSelectTimeoutUsec(msecs_to_usecs(10));
}

TEST_F(GepProtocolTest, Serialize) {
  // use the client GepChannel to test Serialize
  static Command1 empty_command1;
  struct gep_serialize_test {
    int line;
    Command1 *command1;
    std::string expected_command1_str;
  } test_arr[] = {
    {__LINE__, &command1_, command1_str_},
    {__LINE__, &empty_command1, ""},
  };

  for (const auto &test_item : test_arr) {
    std::string capture_str;
    EXPECT_TRUE(proto_->Serialize(*test_item.command1, &capture_str)) <<
        "Error on line " << test_item.line;
    EXPECT_STREQ(test_item.expected_command1_str.c_str(),
                 capture_str.c_str()) <<
        "Error on line " << test_item.line;
  }
}

TEST_F(GepProtocolTest, Unserialize) {
  // use the client GepChannel to test Unserialize
  static Command1 empty_command1;
  struct gep_unserialize_test {
    int line;
    bool success;
    Command1 *expected_command1;
    std::string command1_str;
  } test_arr[] = {
    {__LINE__, true, &command1_, command1_str_},
    {__LINE__, true, &empty_command1, ""},
    // invalid message
    {__LINE__, false, &empty_command1, "invalid text protobuf"},
  };

  for (const auto &test_item : test_arr) {
    Command1 msg;
    EXPECT_EQ(test_item.success,
              proto_->Unserialize(test_item.command1_str, &msg)) <<
        "Error on line " << test_item.line;
    if (test_item.success)
      EXPECT_TRUE(ProtobufEqual(*test_item.expected_command1, msg)) <<
          "Error on line " << test_item.line;
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
