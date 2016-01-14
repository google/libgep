// GEP/SGP protocol
//
// SGP is a GEP-based protocol.
//

#ifndef _SGP_PROTOCOL_H_
#define _SGP_PROTOCOL_H_

#include <gep_protocol.h>  // for MakeTag, GepProtocol
#include <google/protobuf/message.h>  // for Message
#include <stdint.h>  // for uint32_t


// SGP protocol
class SGPProtocol : public GepProtocol {
 public:
  explicit SGPProtocol(int port = kPort);
  virtual ~SGPProtocol() {}

  // basic protocol constants
  static const int kPort = 3456;

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
  virtual uint32_t GetTag(const ::google::protobuf::Message *msg);
  // constructs an object of a given type.
  virtual ::google::protobuf::Message *GetMessage(uint32_t tag);
};

#endif  // _SGP_PROTOCOL_H_
