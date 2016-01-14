// GEP protocol: client side.

#ifndef _GEP_CLIENT_H_
#define _GEP_CLIENT_H_

#include <atomic>  // for atomic
#include <string>  // for string
#include <thread>  // for thread

#include "gep_channel.h"  // for GepChannel
#include "gep_protocol.h"  // for GepProtocol


class GepClient {
 public:
  // Creates a GEP client instance. The instance supports connecting to
  // 1 GEP server and (when started via Start()) kicks off a
  // thread that connects on the provided TCP port to the GEP
  // server (at localhost).
  GepClient(const std::string &name, void *context,
            GepProtocol *proto, const GepVFT* ops);
  virtual ~GepClient();

  // Starts the given GEP client by creating the service thread and
  // connecting to the GEP server.
  // Returns an error code (0 if ok, <0 if problems)
  int Start();
  // Stop the given GEP client by terminating the thread
  // and closing the GEP channel connections.
  // Returns an error code (0 if ok, <0 if problems)
  void Stop();

  // default function run by the server thread
  virtual void RunThread();

  // accessors
  GepProtocol *GetProto() { return proto_; }
  GepChannel *GetGepChannel() { return gep_channel_; }
  std::atomic<bool> &GetThreadCtrl() { return thread_ctrl_; }

  // send API
  // Returns status value (0 if all ok, -1 for any error)
  virtual int Send(const ::google::protobuf::Message &msg);

  // Returns how many times the client reconnected to the server socket.
  int GetReconnectCount() { return reconnect_count_; }

 private:
  // Attempts to reconnect the socket when disconnected.
  void Reconnect();

  std::string name_;
  void *context_;  // not owned
  GepProtocol *proto_;  // owned and responsible for destruction
  const GepVFT* ops_;  // not owned
  GepChannel *gep_channel_;  // owned
  std::thread thread_;
  std::atomic<bool> thread_ctrl_;
  std::atomic<int> reconnect_count_;
};

#endif  // _GEP_CLIENT_H_
