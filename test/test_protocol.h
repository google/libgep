// GEP/Test protocol
//
// Test is a GEP-based protocol.
//

#ifndef _TEST_PROTOCOL_H_
#define _TEST_PROTOCOL_H_

#include <stdint.h>  // for uint32_t

#include "gep_common.h"  // for GepProtobufMessage
#include "gep_protocol.h"  // for MakeTag, GepProtocol

// Sagesrv internals protocol
class TestProtocol : public GepProtocol {
 public:
  explicit TestProtocol(int port = kPort);
  virtual ~TestProtocol() {}

  // basic protocol constants
  static const int kPort = 6999;

  // supported messages
  static constexpr uint32_t MSG_TAG_COMMAND_1 =
      MakeTag('c', 'm', 'd', '1');
  static constexpr uint32_t MSG_TAG_COMMAND_2 =
      MakeTag('c', 'm', 'd', '2');
  static constexpr uint32_t MSG_TAG_COMMAND_3 =
      MakeTag('c', 'm', 'd', '3');
  static constexpr uint32_t MSG_TAG_COMMAND_4 =
      MakeTag('c', 'm', 'd', '4');
  static constexpr uint32_t MSG_TAG_CONTROL =
      MakeTag('c', 't', 'r', 'l');

  // returns the tag associated to a message.
  virtual uint32_t GetTag(const GepProtobufMessage *msg);
  // constructs an object of a given type.
  virtual GepProtobufMessage *GetMessage(uint32_t tag);
};

#endif  // _TEST_PROTOCOL_H_
