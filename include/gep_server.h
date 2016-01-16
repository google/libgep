// GEP protocol: server side.

#ifndef _GEP_SERVER_H_
#define _GEP_SERVER_H_

#include <atomic>
#include <google/protobuf/message.h>  // for Message
#include <thread>  // for thread
#include <string>  // for string
#include <stddef.h>  // for NULL

#include "gep_channel_array.h"
#include "gep_protocol.h"


// internals
class GepServer {
 public:
  // Creates a GEP server. The instance supports up to max_channels
  // GEP channels and (when started via Start()) kicks off a
  // service thread that listens on the provided TCP port for requests from
  // GEP clients (localhost only). Each connection creates a new GEP
  // channel and stays up for the entirety of an GEP session.
  // \param name: name used for logging
  // \param max_channels: maximum number of concurrent clients
  // \param context: a void* that will be passed to recv callbacks
  // \param proto: a GEP protocol object
  // \param ops: a GEP virtual function table
  GepServer(const std::string &name, int max_channels, void *context,
            GepProtocol *proto, const GepVFT* ops);
  virtual ~GepServer();

  // Starts the given GEP server by opening the TCP service socket
  // and creating the service thread that accepts and manages GEP channel
  // connections.
  // Returns an error code (0 if ok, <0 if error).
  virtual int Start();
  // Stops the given GEP server by terminating the server thread
  // and closing all open GEP channel connections.
  virtual void Stop();

  // default function run by the server thread
  virtual void RunThread();

  // accessors
  GepProtocol *GetProto() { return proto_; }
  GepChannelArray *GetGepChannelArray() { return gep_channel_array_; }
  int GetNumClients() { return gep_channel_array_->GetVectorSize(); }
  std::atomic<bool> &GetThreadCtrl() { return thread_ctrl_; }
  int GetPort() { return proto_->GetPort(); }

  // send API
  // Returns status value (0 if all ok, -1 for any error)
  virtual int Send(const ::google::protobuf::Message &msg);
  virtual int Send(const ::google::protobuf::Message &msg, int id);

  // client (dis)connection callbacks
  virtual void AddClient(int id) { }
  virtual void DelClient(int id) { }

 private:
  std::string name_;
  void *context_;  // not owned
  GepProtocol *proto_;  // owned and responsible for destruction
  const GepVFT* ops_;  // not owned
  GepChannelArray *gep_channel_array_;  // structure to manage channels (owned)
  std::thread thread_;
  std::atomic<bool> thread_ctrl_;
};

#endif  // _GEP_SERVER_H_
