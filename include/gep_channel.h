// GEP Protocol: GEPChannel.

#ifndef _GEP_CHANNEL_H_
#define _GEP_CHANNEL_H_

#include <mutex>
#include <stdint.h>  // for uint32_t, uint8_t
#include <string>  // for string

#include "gep_common.h"  // for GepProtobufMessage
#include "gep_protocol.h"  // for GepProtocol (ptr only), etc

class SocketInterface;


// Class used to manage a communication channel where protobuf messages can
// be sent back and forth.
class GepChannel {
 public:
  GepChannel(int id, const std::string &name, GepProtocol *proto,
             const GepVFT *ops, void *context, int socket = -1);
  virtual ~GepChannel();

  // Process received event data: execute any commands and remove completed
  // commands from the recv buffer.
  // Any leftover data is moved to beginning of the buffer. If the host data
  // contains several commands, all of them are processed.
  // Returns 0 for success, -1 on a fatal error, and -2 if the connection was
  // closed.
  int RecvData();

  // Send a specific protobuf message to a GEP client.
  // Returns status value (0 if ok, -1 for error)
  virtual int SendMessage(const GepProtobufMessage &msg);

  // socket opening/closing
  int OpenClientSocket();
  int Close();

  // accessors
  int GetId() const { return id_; }
  int GetSocket();
  void SetSocket(int socket);
  bool IsOpenSocket();
  void *GetContext() const { return context_; }
  int GetLen() const { return len_; }
  void SetLen(int len) { len_ = len; }

  // socket interface
  SocketInterface *GetSocketInterface() { return socket_interface_; }
  void SetSocketInterface(SocketInterface *socket_interface) {
    socket_interface_ = socket_interface;
  }

  // some constants
  // maximum time to wait for a send
  static const int64_t kGepSendTimeoutMs = 5;

 protected:
  // return values used by GepChannel::RecvData()
  enum Result {
    CMD_ERROR      = -1,  // unsupported command or invalid channel pointer
    CMD_OK         =  0,  // successfully processed one or more commands
    CMD_FRAGMENTED =  1,  // command is fragmented, needs more data
    CMD_DROPPED    =  2   // unsupported command, data dropped
  };
  bool IsRecoverable(Result ret) { return ret >= 0; }

 private:
  // sends generic data to the GEP channel socket
  int SendData(const char *buf, int bytes);
  // receives generic data in the GEP channel socket
  Result RecvString();
  // receives a TLV tuple in the GEP channel socket
  Result RecvTLV(uint32_t tag, int value_len, const uint8_t *value);
  // sends a TLV tuple to the GEP channel socket
  int SendTLV(uint32_t tag, int value_len, const char *value);
  // sends a generic string to the GEP channel socket
  int SendString(uint32_t tag, const std::string &s);

  std::string name_;
  GepProtocol *proto_;      // not owned
  const GepVFT *ops_;       // VFT for the receiving side (not owned)
  void *context_;           // link to context (not owned)
  int id_;
  SocketInterface *socket_interface_;  // socket interface
  int socket_;              // command socket used to talk to the other side
  int len_;                 // amount of data currently in buf
  uint8_t buf_[GepProtocol::kMaxMsgLen];  // receive buffer for command data
                                         // from clients
  std::mutex socket_lock_;  // guards access to channel socket between senders
                            // and the socket controller (open/close) (which
                            // is also the recv)

  // do not copy this object
  GepChannel(const GepChannel&) = delete;  // suppress copy
  GepChannel& operator=(const GepChannel&) = delete;  // suppress assignment
};

#endif  // _GEP_CHANNEL_H_
