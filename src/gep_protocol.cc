// GEP protocol implementation.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS
#endif

#include "gep_protocol.h"

#include <google/protobuf/text_format.h>  // for TextFormat
#include <netinet/in.h>  // for htonl

#include "gep_common.h"  // for GepProtobufMessage
#include "utils.h"  // for SET_UINT32, UINT32, snprintf_printable

using namespace libgep_utils;

const int64_t kDefaultSelectTimeUsec = secs_to_usecs(1);

GepProtocol::GepProtocol(int port)
    : port_(port),
      mode_(kMode),
      magic_(kMagic),
      select_timeout_usec_(kDefaultSelectTimeUsec) {
}

GepProtocol::~GepProtocol() {
}

// descriptor/tag converters
int GepProtocol::TagString(uint32_t tag, char *buf, int max_buf) {
  uint32_t tag_n = htonl(tag);
  return snprintf_printable(buf, max_buf,
                            reinterpret_cast<uint8_t *>(&tag_n), 4);
}

void GepProtocol::SetSelectTimeoutUsec(int64_t select_timeout_usec) {
  select_timeout_usec_ = select_timeout_usec;
}

bool GepProtocol::ScanHeader(uint8_t *buf, uint32_t *tag, uint32_t *value_len) {
  // read TL in TLV
  uint32_t magic = UINT32(buf + kOffsetMagic);
  *tag = UINT32(buf + kOffsetTag);
  *value_len = UINT32(buf + kOffsetLen);

  // check the magic number
  return (magic == magic_);
}

void GepProtocol::PrintHeader(uint32_t tag, uint32_t value_len, uint8_t *buf) {
  SET_UINT32(buf + kOffsetMagic, magic_);
  SET_UINT32(buf + kOffsetTag, tag);
  SET_UINT32(buf + kOffsetLen, value_len);
}

bool GepProtocol::Serialize(const GepProtobufMessage &msg, std::string *s) {
#ifndef GEP_LITE
  if (mode_ == MODE_TEXT) {
    return (google::protobuf::TextFormat::PrintToString(msg, s));
  } else {  // mode_ == MODE_BINARY
#endif
    return msg.SerializeToString(s);
#ifndef GEP_LITE
  }
#endif
}

bool GepProtocol::Unserialize(const std::string &s, GepProtobufMessage *msg) {
#ifndef GEP_LITE
  if (mode_ == MODE_TEXT) {
    return (google::protobuf::TextFormat::ParseFromString(s, msg));
  } else {  // mode_ == MODE_BINARY
#endif
    msg->Clear();
    if (!s.empty())
      return msg->ParseFromString(s);
    return true;
#ifndef GEP_LITE
  }
#endif
}
