// GEP protocol: server implementation.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS
#endif


#include "gep_server.h"

#include <errno.h>  // for errno, EINTR
#include <google/protobuf/message.h>  // for Message
#include <stdint.h>  // for int64_t
#include <stdio.h>  // for NULL
#include <sys/select.h>  // for FD_ISSET, FD_SET, select, etc
#include <sys/time.h>  // for timeval
#include <syscall.h>  // for __NR_gettid
#include <thread>  // NOLINT
#include <unistd.h>  // for syscall, pid_t

#include "gep_channel_array.h"  // for GepChannelArray
#include "utils.h"  // for MAX

using namespace libgep_utils;

GepServer::GepServer(const std::string &name, int max_channels,
    void *context, GepProtocol *proto, const GepVFT* ops)
    : name_(name),
      context_(context),
      proto_(proto),
      ops_(ops),
      thread_ctrl_(false) {
  // init GEP channel array
  gep_channel_array_ = new GepChannelArray("gep_channel_array", proto_,
                                           max_channels, ops_, context_);
}

GepServer::~GepServer() {
  // close and free all the GEP channels
  delete gep_channel_array_;
  delete proto_;
}

int GepServer::Start() {
  if (gep_channel_array_->OpenServerSocket() < 0)
    return -1;

  thread_ctrl_ = true;
  thread_ = std::thread(&GepServer::RunThread, this);
  gep_log(LOG_WARNING,
          "%s(*):thread started", name_.c_str());
  return 0;
}

void GepServer::Stop() {
  gep_log(LOG_WARNING,
          "%s(*):kill thread", name_.c_str());
  thread_ctrl_ = false;
  thread_.join();

  // closing all channels and sockets
  gep_channel_array_->ClearGepChannelVector();
}

void GepServer::RunThread() {
  int max_fds;
  fd_set read_fds;
  pid_t tid = syscall(__NR_gettid);

  gep_log(LOG_DEBUG,
          "%s(*):service thread is running (tid:%d)",
          name_.c_str(), tid);

  int server_socket = gep_channel_array_->GetServerSocket();
  if (server_socket < 0) {
    gep_log(LOG_ERROR,
            "%s(*):Error-invalid server socket",
            name_.c_str());
    return;
  }

  while (GetThreadCtrl()) {
    FD_ZERO(&read_fds);
    FD_SET(server_socket, &read_fds);
    max_fds = server_socket;
    gep_channel_array_->GetVectorReadFds(&max_fds, &read_fds);

    // Calculate the select timeout.
    int64_t select_timeout_usec = proto_->GetSelectTimeoutUsec();
    struct timeval select_timeout = usecs_to_timeval(select_timeout_usec);

    int status = select(max_fds + 1, &read_fds, NULL, NULL, &select_timeout);
    if (status < 0 && errno != EINTR) {
      gep_perror(errno, "%s(*):Error-service socket select-",
                   name_.c_str());
      break;
    }

    if (!GetThreadCtrl()) break;

    // process all inputs
    gep_channel_array_->RecvData(&read_fds);

    if (!GetThreadCtrl()) break;

    // accept new GEP channel connections (from GEP clients)
    if (FD_ISSET(server_socket, &read_fds))
      if (gep_channel_array_->AcceptConnection() < 0)
        break;
  }  // while (GetThreadCtrl())

  gep_log(LOG_WARNING,
          "%s(*):service thread is exiting (tid:%d)",
          name_.c_str(), tid);
}

int GepServer::Send(const ::google::protobuf::Message &msg) {
  return gep_channel_array_->SendMessage(msg);
}

int GepServer::Send(const ::google::protobuf::Message &msg, int id) {
  return gep_channel_array_->SendMessage(msg, id);
}
