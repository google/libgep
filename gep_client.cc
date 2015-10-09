// GEP protocol: client implementation.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS
#endif


#include "gep_client.h"

#include <errno.h>  // for errno, EINTR
#include <stdint.h>  // for int64_t, uint32_t
#include <stdio.h>  // for NULL
#include <sys/select.h>  // for FD_ISSET, FD_SET, select, etc
#include <sys/time.h>  // for timeval
#include <syscall.h>  // for __NR_gettid
#include <thread>  // NOLINT
#include <unistd.h>  // for syscall, pid_t

#include "gep_channel.h"  // for GepChannel
#include "gep_protocol.h"  // for GepProtocol, etc
#include "utils.h"  // for LOG_ERROR, gep_log, etc

static const uint32_t kReconnectRetryDelaySecs = 5;

GepClient::GepClient(const std::string &name, void *context,
                     GepProtocol *proto, const GepVFT* ops)
    : name_(name),
      context_(context),
      proto_(proto),
      ops_(ops),
      thread_ctrl_(false),
      reconnect_count_(0) {
  gep_channel_ = new GepChannel(0, name_, proto_, ops_, context_);
}

GepClient::~GepClient() {
  // close and free the GEP channel
  delete gep_channel_;
  delete proto_;
}

int GepClient::Start() {
  if (gep_channel_->OpenClientSocket(proto_->GetPort()) < 0) {
    gep_log(LOG_ERROR,
            "%s(*):cannot open server socket.",
            name_.c_str());
    return -1;
  }

  thread_ctrl_ = true;
  thread_ = std::thread(&GepClient::RunThread, this);
  gep_log(LOG_WARNING,
          "%s(*):thread started", name_.c_str());
  return 0;
}

void GepClient::Stop() {
  gep_log(LOG_WARNING,
          "%s(*):kill thread", name_.c_str());
  thread_ctrl_ = false;
  thread_.join();

  // closing GEP channel
  gep_channel_->Close();
  reconnect_count_ = 0;
}

void GepClient::Reconnect() {
  // try to reconnect
  gep_log(LOG_WARNING,
          "%s(*):reconnecting to server socket.",
          name_.c_str());
  if (gep_channel_->OpenClientSocket(proto_->GetPort()) < 0) {
    gep_log(LOG_ERROR,
            "%s(*):cannot open server socket.",
            name_.c_str());
    sleep(kReconnectRetryDelaySecs);
  } else {
    gep_log(LOG_WARNING,
            "%s(*):reconnected.", name_.c_str());
    reconnect_count_++;
  }
}

int GepClient::SendMessage(const ::google::protobuf::Message &msg) {
  return gep_channel_->SendMessage(msg);
}

void GepClient::RunThread() {
  int max_fds;
  fd_set read_fds;
  pid_t tid = syscall(__NR_gettid);

  gep_log(LOG_DEBUG,
          "%s(*):service thread is running (tid:%d)",
          name_.c_str(), tid);

  int socket = gep_channel_->GetSocket();
  while (GetThreadCtrl()) {
    if (socket == -1) {
      Reconnect();
      socket = gep_channel_->GetSocket();
      continue;
    }
    FD_ZERO(&read_fds);
    FD_SET(socket, &read_fds);
    max_fds = socket;

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

    // Handle incoming requests from the server and check for timeout
    if (FD_ISSET(socket, &read_fds)) {
      int res = gep_channel_->RecvData();
      if (res < 0) {
        // on any receive error, toss the existing connection and try to
        // reconnect
        gep_log(LOG_WARNING,
                "%s(*):connection reset by peer.",
                name_.c_str());
        gep_channel_->Close();
        socket = -1;
      }
    }
  }  // endof while (GetThreadCtrl())

  gep_log(LOG_WARNING,
          "%s(*):service thread is exiting (tid:%d)",
          name_.c_str(), tid);
}
