// GEP Protocol: GEPChannel implementation.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS
#endif


#include "gep_channel.h"

#include <errno.h>  // for errno, ECONNRESET
#include <inttypes.h>
#include <map>  // for _Rb_tree_const_iterator
#include <mutex>
#include <netinet/in.h>  // for sockaddr_in, htonl, htons, etc
#include <string.h>  // for memmove
#include <sys/socket.h>  // for AF_INET, connect, recv, etc
#include <unistd.h>  // for close, usleep
#include <utility>  // for pair

#include "gep_common.h"  // for GepProtobufMessage
#include "socket_interface.h"  // for SocketInterface
#include "utils.h"  // for snprintf_printable

using namespace libgep_utils;

GepChannel::GepChannel(int id, const std::string &name,
                       GepProtocol *proto, const GepVFT *ops,
                       void *context, int socket)
    : name_(name),
      proto_(proto),
      ops_(ops),
      context_(context),
      id_(id),
      socket_(socket),
      len_(0) {
  socket_interface_ = new SocketInterface();
}

GepChannel::~GepChannel() {
  Close();
  delete socket_interface_;
}


int GepChannel::OpenClientSocket() {
  int port = proto_->GetPort();

  // Open socket.
  int new_socket;
  if ((new_socket = socket_interface_->Socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    gep_log(LOG_ERROR,
            "%s(%i):Error-cannot open client socket",
            name_.c_str(), id_);
    return -1;
  }

  // Connect socket.
  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
  // Use INADDR_LOOPBACK as server address to connect to the localhost.
  //  You have to bind the server to the same INADDR_LOOPBACK address for
  //  the latter to only accept calls from there.
  saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (connect(new_socket, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
    gep_log(LOG_ERROR,
            "%s(%i):Error-cannot connect client socket %i",
            name_.c_str(), id_, new_socket);
    close(new_socket);
    return -1;
  }
  std::lock_guard<std::mutex> lock_guard(socket_lock_);
  socket_ = new_socket;
  gep_log(LOG_DEBUG,
          "%s(%i):open client socket %d",
          name_.c_str(), id_, socket_);
  return 0;
}

int GepChannel::Close() {
  if (socket_ != -1) {
    std::lock_guard<std::mutex> lock_guard(socket_lock_);
    gep_log(LOG_DEBUG,
            "%s(%i):closed socket %d",
            name_.c_str(), id_, socket_);
    close(socket_);
    socket_ = -1;
    len_ = 0;
    return 0;
  }
  return -1;
}

int GepChannel::GetSocket() {
  std::lock_guard<std::mutex> lock_guard(socket_lock_);
  return socket_;
}

void GepChannel::SetSocket(int socket) {
  std::lock_guard<std::mutex> lock_guard(socket_lock_);
  socket_ = socket;
}

bool GepChannel::IsOpenSocket() {
  std::lock_guard<std::mutex> lock_guard(socket_lock_);
  return socket_ != -1;
}

int GepChannel::RecvData() {
  if (socket_ < 0) {
    gep_log(LOG_ERROR,
            "%s:recv(%i):Error-invalid socket %d",
            name_.c_str(), id_, socket_);
    return -1;
  }

  // check there is some space in the buffer
  if (len_ >= sizeof(buf_)) {
    gep_log(LOG_ERROR,
            "%s:recv(%i):Error-buf_ full (%i/%zu)",
            name_.c_str(), id_, len_, sizeof(buf_));
    return -1;
  }

  // read new data from command socket and append to any leftover one
  socket_lock_.lock();
  int bytes = socket_interface_->Recv(socket_, buf_ + len_,
                                      sizeof(buf_) - len_, 0);
  socket_lock_.unlock();

  // TODO(chema): support EAGAIN and EWOULDBLOCK
  if (bytes > 0) {
    len_ += bytes;
    if (RecvString() == CMD_ERROR) {
      gep_log(LOG_ERROR,
              "%s:recv(%i):Error-Incorrect data received on socket %d",
              name_.c_str(), id_, socket_);
      return -1;
    }
  } else if (bytes == 0) {
    gep_log(LOG_DEBUG,
            "%s:recv(%i):socket %d was closed by peer",
            name_.c_str(), id_, socket_);
    return -2;
  } else {
    gep_perror(errno, "%s:recv(%i):Error-recv() failed on socket %d:",
                 name_.c_str(), id_, socket_);
    return -1;
  }
  return 0;
}

int GepChannel::SendData(const char *buf, int bytes) {
  int sent = socket_interface_->FullSend(socket_, (const uint8_t *)buf, bytes,
                                         kGepSendTimeoutMs);
  if (sent == 0) {
    gep_log(LOG_DEBUG,
            "%s:send(%i):socket %d was closed by peer",
            name_.c_str(), id_, socket_);
  } else if (sent == -2) {
    gep_log(LOG_DEBUG,
            "%s:send(%i):socket %d was closed by peer",
            name_.c_str(), id_, socket_);
  } else if (sent == -1) {
    gep_perror(errno, "%s:send(%d):Error-failed sending %d bytes on "
               "socket %d", name_.c_str(), id_, bytes, socket_);
  }

  // return the number of bytes sent
  return sent;
}

GepChannel::Result GepChannel::RecvString() {
  while (len_ >= proto_->GetHdrLen()) {
    uint32_t tag;
    uint32_t value_len;
    if (!proto_->ScanHeader(buf_, &tag, &value_len)) {
      char tmp[4 * 4 + 1];
      snprintf_printable(tmp, sizeof(tmp), buf_, 4);
      gep_log(LOG_ERROR,
              "%s:recv(*):Error-Wrong magic number (%s)",
              name_.c_str(), tmp);
      len_ = 0;
      return CMD_ERROR;
    }

    // ensure the value length is ok for GEP
    if (value_len >= (GepProtocol::kMaxMsgLen - proto_->GetHdrLen())) {
      gep_log(LOG_ERROR,
              "%s:recv(%i):Error-Value length too large (%" PRIu32
              " >= %" PRIu32 ")",
              name_.c_str(), id_, value_len, GepProtocol::kMaxMsgLen);
      len_ = 0;
      return CMD_ERROR;
    }
    uint32_t msg_len = proto_->GetHdrLen() + value_len;

    // process fragmented packets
    if (len_ < msg_len) {
      // value is not complete in command buffer, wait for more
      gep_log(LOG_DEBUG,
              "%s:recv(%i):Command is fragmented (recv %i bytes)",
              name_.c_str(), id_, len_);
      return CMD_FRAGMENTED;
    }

    // receive the packet
    uint8_t *value = buf_ + proto_->GetOffsetValue();
    if (gep_log_get_level() >= LOG_DEBUG) {
      char tmp[value_len * 4 + 1];
      snprintf_printable(tmp, sizeof(tmp), value, value_len);
      char tag_string[kMaxTagString];
      proto_->TagString(tag, tag_string, kMaxTagString);
      gep_log(LOG_DEBUG,
              "%s:recv(%i):Received command (%i bytes) {%s, %u, %s}",
              name_.c_str(), id_, msg_len,
              tag_string, value_len, tmp);
    }

    // unpack and recv the message
    Result ret = RecvTLV(tag, value_len, value);
    if (!IsRecoverable(ret))
      return ret;

    // process remaining data
    int remain = len_ - msg_len;
    if (remain) {
      // copy left-over to the beginning of the buffer
      memmove(buf_, buf_ + msg_len, remain);
      gep_log(LOG_DEBUG,
              "%s:recv(%i):Fragmented command (left %d bytes)",
              name_.c_str(), id_, len_);
    }
    len_ = remain;
  }
  return len_ ? CMD_FRAGMENTED : CMD_OK;
}

int GepChannel::SendString(uint32_t tag, const std::string &s) {
  // send the TLV
  const char *value = s.c_str();
  int value_len = s.length();
  // return the total number of bytes sent
  return SendTLV(tag, value_len, value);
}

GepChannel::Result GepChannel::RecvTLV(uint32_t tag, int value_len,
                                       const uint8_t *value) {
  int ret = 0;
  char tag_string[kMaxTagString];
  proto_->TagString(tag, tag_string, kMaxTagString);
  auto iter = ops_->find(tag);
  if (iter != ops_->end()) {
    GepProtobufMessage *msg = proto_->GetMessage(tag);
    gep_log(LOG_DEBUG,
            "%s:recv(%i):Received message with tag [%s] (%d value bytes)",
            name_.c_str(), id_, tag_string, value_len);
    std::string value_str((const char *)value, (size_t)value_len);
    if (!proto_->Unserialize(value_str, msg)) {
      char tmp[value_len * 4];
      snprintf_printable(tmp, value_len * 4, value, value_len);
      gep_log(LOG_WARNING,
              "%s:recv(%i):Error-Unpackable message with tag [%s] (%d bytes) "
              "[%s]",
              name_.c_str(), id_, tag_string, len_, tmp);
      delete msg;
      return CMD_ERROR;
    }
    ret = iter->second(*msg, this);
    if (ret < 0) {
      gep_log(LOG_WARNING,
              "%s:recv(%i):callback error [%s]: %i",
              name_.c_str(), id_, tag_string, ret);
    }
    delete msg;
  } else {
    gep_log(LOG_WARNING,
            "%s:recv(%i):Error-Unsupported tag [%s] (%d bytes)",
            name_.c_str(), id_, tag_string, len_);
    return CMD_DROPPED;
  }

  return CMD_OK;
}

// Send a message with the given message tag and optional value[value_len]
//  to the GEP client.
// Returns number of bytes sent, -1 for error.
int GepChannel::SendTLV(uint32_t tag, int value_len, const char *value) {
  // mutex is held for all this function to ensure header and data are
  // sent consecutively
  std::lock_guard<std::mutex> lock_guard(socket_lock_);

  if (!value) value_len = 0;
  char tag_string[kMaxTagString];
  proto_->TagString(tag, tag_string, kMaxTagString);

  // send protocol header
  int hdr_len = proto_->GetHdrLen();
  char buf[hdr_len];
  proto_->PrintHeader(tag, value_len, reinterpret_cast<uint8_t *>(buf));
  int ret1 = SendData(buf, hdr_len);
  if (ret1 != hdr_len) {
    gep_log(LOG_ERROR,
            "%s:send(%i):Error-Only sent %d/%d hdr bytes to host",
            name_.c_str(), id_, ret1, hdr_len);
    return -1;
  }
  gep_log(LOG_DEBUG,
          "%s:send(%i):sent header:%s, %d/%d bytes",
          name_.c_str(), id_, tag_string, ret1, hdr_len + value_len);

  // check if there is a value to send
  if (value_len <= 0)
    return 0;

  // send value
  int ret2 = SendData(value, value_len);
  if (ret2 != value_len) {
    gep_log(LOG_ERROR,
            "%s:send(%i):Error-Only sent %d/%d data bytes to host",
            name_.c_str(), id_, ret2, value_len);
    return -1;
  }
  gep_log(LOG_DEBUG,
          "%s:send(%i):sent message:%s, %d/%d bytes",
          name_.c_str(), id_, tag_string, ret2,
          hdr_len + value_len);
  // return error code
  return 0;
}

int GepChannel::SendMessage(const GepProtobufMessage &msg) {
  // serialize the message
  std::string s;
  if (!proto_->Serialize(msg, &s)) {
    gep_log(LOG_ERROR,
            "%s:send(%i):Error-%s:serializing message",
            name_.c_str(), id_, __func__);
    return -1;
  }
  // send the string
  return SendString(proto_->GetTag(&msg), s);
}
