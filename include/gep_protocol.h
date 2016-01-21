// GEP (Generic Event Protobuf aka Generic Eh... Protobuf) protocol

#ifndef _GEP_PROTOCOL_H_
#define _GEP_PROTOCOL_H_

#include <functional>  // for function
#include <google/protobuf/message.h>  // for Message
#include <map>  // for map
#include <stdint.h>  // for uint32_t, uint8_t
#include <string>  // for string


// Callback function that will be called when a protobuf message is
// received.
// \param msg: the received message
// \param context: (pointer to) a generic object
// \return 0 if the callback went OK, >1 if not implemented, <0 otherwise
typedef std::function<int(const ::google::protobuf::Message &msg,
                          void *context)> GepCallback;

typedef std::map<uint32_t, GepCallback> GepVFT;

// macro used to define tags
constexpr int MakeTag(char a, char b, char c, char d) {
  return (((unsigned)(a) << 24) | ((b) << 16) | ((c) << 8) | (d));
}

// Characters may need up to 4 characters to be printable ("\x00" for '\0').
// Add one for the final end of string.
const int kMaxTagString = 4 * 4 + 1;

class GepProtocol {
 public:
  explicit GepProtocol(int port);
  virtual ~GepProtocol();

  // scans a buffer looking for a valid GEP header. Returns true if valid
  // header, false otherwise
  bool ScanHeader(uint8_t *buf, uint32_t *tag, uint32_t *value_len);
  // prints a a valid GEP header into a buffer
  void PrintHeader(uint32_t tag, uint32_t value_len, uint8_t *buf);

  // returns the tag associated to a message.
  virtual uint32_t GetTag(const ::google::protobuf::Message *msg) = 0;
  // constructs an object of a given type.
  virtual ::google::protobuf::Message *GetMessage(uint32_t tag) = 0;

  // converts a tag into a printable string
  int TagString(uint32_t tag, char *buf, int max_buf);

  // serializer code
  // TODO(chema): provide a flag to support binary-protobuf GEP
  // Return an error code (true if ok, false if problems)
  bool Serialize(const ::google::protobuf::Message &msg, std::string *s);
  bool Unserialize(const std::string &s, ::google::protobuf::Message *msg);

  // accessors
  int GetPort() const { return port_; }
  void SetPort(int port) { port_ = port; }
  static uint32_t GetHdrLen() { return kHdrLen; }
  static uint32_t GetOffsetValue() { return kOffsetValue; }
  int64_t GetSelectTimeoutUsec() { return select_timeout_usec_; }
  void SetSelectTimeoutUsec(int64_t select_timeout_usec);
  uint32_t GetMagic() const { return magic_; }
  void SetMagic(uint32_t magic) { magic_ = magic; }

  // GEP protocol constants
  // maximum length of a single message
  static const uint32_t kMaxMsgLen = 1 << 20;

 protected:
  int port_;

  // GEP protocol header
  static const uint32_t kOffsetMagic = 0;
  static const uint32_t kOffsetTag = 4;
  static const uint32_t kOffsetLen = 8;
  static const uint32_t kOffsetValue = 12;
  static const uint32_t kHdrLen = kOffsetValue;

  // protocol header
  static constexpr uint32_t kMagic = MakeTag('g', 'e', 'p', 'p');
  uint32_t magic_;

  // select() timeout, in usec
  int64_t select_timeout_usec_;
};

#endif  // _GEP_PROTOCOL_H_
