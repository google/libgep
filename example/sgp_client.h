// SGP protocol: client side.

#ifndef _SGP_CLIENT_H_
#define _SGP_CLIENT_H_

#include <gep_protocol.h>  // for GepVFT
#include <gep_client.h>  // for GepClient
#include <gep_utils.h>  // for RecvMessage

#include "sgp_protocol.h"  // for SGPProtocol, etc
#include "sgp.pb.h"  // for Command1, etc

// Class implementing a SGPClient: This is basically a GEP client
// that hardcodes the use of a SGPProtocol.
class SGPClient: public GepClient {
 public:
  SGPClient();
  explicit SGPClient(int port);
  virtual ~SGPClient();

  // protocol object callbacks: These are object (non-static) callback
  // methods, which is handy for the callers.
  virtual bool Recv(const Command1 &msg) = 0;
  virtual bool Recv(const Command2 &msg) = 0;
  virtual bool Recv(const Command3 &msg) = 0;
  virtual bool Recv(const Command4 &msg) = 0;
};

const GepVFT kSGPClientOps = {
  {SGPProtocol::MSG_TAG_COMMAND_1, &RecvMessage<SGPClient, Command1>},
  {SGPProtocol::MSG_TAG_COMMAND_2, &RecvMessage<SGPClient, Command2>},
  {SGPProtocol::MSG_TAG_COMMAND_3, &RecvMessage<SGPClient, Command3>},
  {SGPProtocol::MSG_TAG_COMMAND_4, &RecvMessage<SGPClient, Command4>},
};

#endif  // _SGP_CLIENT_H_
