// SGP protocol: server side.

#ifndef _SGP_SERVER_H_
#define _SGP_SERVER_H_

#include <gep_protocol.h>  // for GepVFT
#include <gep_server.h>  // for GepServer
#include <gep_utils.h>  // for RecvMessage

#include "sgp_protocol.h"  // for SGPProtocol, etc
#include "sgp.pb.h"  // for Command1, etc

// Class implementing a SGPServer: This is basically a GEP server
// that hardcodes the use of a SGPProtocol. It is used as the
// context in callbacks.
class SGPServer: public GepServer {
 public:
  explicit SGPServer(int max_channel_num);
  SGPServer(int max_channel_num, int port);
  virtual ~SGPServer();

  // protocol object callbacks: These are object (non-static) callback
  // methods, which is handy for the callers.
  virtual bool Recv(const Command1 &msg) = 0;
  virtual bool Recv(const Command2 &msg) = 0;
  virtual bool Recv(const Command3 &msg) = 0;
  virtual bool Recv(const Command4 &msg) = 0;
};

const GepVFT kSGPServerOps = {
  {SGPProtocol::MSG_TAG_COMMAND_1, &RecvMessage<SGPServer, Command1>},
  {SGPProtocol::MSG_TAG_COMMAND_2, &RecvMessage<SGPServer, Command2>},
  {SGPProtocol::MSG_TAG_COMMAND_3, &RecvMessage<SGPServer, Command3>},
  {SGPProtocol::MSG_TAG_COMMAND_4, &RecvMessage<SGPServer, Command4>},
};

#endif  // _SGP_SERVER_H_
