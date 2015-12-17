// GEP protocol: server's array of GepChannel.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS
#endif


#include "gep_channel_array.h"

#include <algorithm>  // for max
#include <errno.h>  // for errno
#include <ext/alloc_traits.h>
#include <google/protobuf/text_format.h>  // for TextFormat
#include <netinet/in.h>  // for sockaddr_in, htons, etc
#include <string.h>  // for memset
#include <sys/socket.h>  // for AF_INET, accept, bind, etc
#include <unistd.h>  // for close

#include "gep_channel.h"  // for GepChannel
#include "utils.h"  // for set_socket_no_delay, etc

using namespace libgep_utils;

GepChannelArray::GepChannelArray(const std::string &name, GepProtocol *proto,
                                 int max_channels, const GepVFT *ops,
                                 void *context)
    : name_(name),
     proto_(proto),
     ops_(ops),
     context_(context),
     max_channels_(max_channels),
     last_channel_id_(0),
     server_socket_(-1) {
}

void GepChannelArray::ClearGepChannelVector() {
  Stop();
}

int GepChannelArray::OpenServerSocket() {
  int sock_fd;

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    gep_perror(errno, "%s(*):Error-opening socket failed-", name_.c_str());
    return -1;
  }

  if (set_socket_reuse_addr(name_.c_str(), sock_fd) < 0) {
    close(sock_fd);
    return -1;
  }

  set_socket_non_blocking(name_.c_str(), sock_fd);
  set_socket_no_delay(name_.c_str(), sock_fd);
  set_socket_priority(name_.c_str(), sock_fd, 4);

  struct sockaddr_in serveraddr;
  serveraddr.sin_family = AF_INET;
  // restrict to local address
  serveraddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  serveraddr.sin_port = htons(proto_->GetPort());
  memset(&(serveraddr.sin_zero), '\0', 8);

  if (bind(sock_fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
    gep_perror(errno, "%s(*):Error-bind service socket-", name_.c_str());
    close(sock_fd);
    return -1;
  }

  if (listen(sock_fd, 4) == -1) {
    gep_perror(errno, "%s(*):Error-listen on service socket-",
                 name_.c_str());
    close(sock_fd);
    return -1;
  }

  if (proto_->GetPort() == 0) {
    // port was dynamically assigned, get it and save it
    int port;
    if (get_socket_port(name_.c_str(), sock_fd, &port) < 0) {
      close(sock_fd);
      return -1;
    }
    proto_->SetPort(port);
  }

  server_socket_ = sock_fd;
  gep_log(LOG_DEBUG,
          "%s(*):open control socket %d on port %d.",
          name_.c_str(), sock_fd, proto_->GetPort());

  return 0;
}

int GepChannelArray::Stop() {
  std::lock_guard<std::recursive_mutex> lock(gep_channel_vector_lock_);
  if (server_socket_ >= 0) {
    gep_log(LOG_DEBUG,
            "%s(*):GepChannelArray::Stop(), closing service socket %d",
            name_.c_str(), server_socket_);
    close(server_socket_);
    server_socket_ = -1;
  }

  // Delete all GepChannel's
  gep_channel_vector_.clear();

  return 0;
}

int GepChannelArray::AddChannel(int socket) {
  std::lock_guard<std::recursive_mutex> lock(gep_channel_vector_lock_);
  if (gep_channel_vector_.size() >= max_channels_) {
    gep_log(LOG_ERROR,
            "%s(*):Error-Too many clients", name_.c_str());
    return -1;
  }
  int id = last_channel_id_++;
  gep_channel_vector_.push_back(std::shared_ptr<GepChannel>(
      new GepChannel(id, "gep_channel", proto_, ops_, context_, socket)));
  gep_log(LOG_DEBUG,
          "%s(%d):add GEP channel using socket %d",
          name_.c_str(), id, socket);
  return 0;
}

int GepChannelArray::AcceptConnection() {
  int new_socket;
  struct sockaddr clientaddr;
  socklen_t addrlen = sizeof(struct sockaddr_in);
  if ((new_socket = accept(server_socket_, &clientaddr, &addrlen)) == -1) {
    gep_perror(errno, "%s(*):ERROR accepting new connection using "
                "socket %d", name_.c_str(), new_socket);
    return -1;
  }
  char tmp[32];
  char *peer_ip = get_peer_ip(new_socket, tmp, sizeof(tmp));
  gep_log(LOG_DEBUG,
          "%s(*):socket %d accepted connection from %s using socket %d",
          name_.c_str(), server_socket_, peer_ip, new_socket);
  set_socket_non_blocking(name_.c_str(), new_socket);
  set_socket_no_delay(name_.c_str(), new_socket);
  set_socket_priority(name_.c_str(), new_socket, 4);
  AddChannel(new_socket);
  return 0;
}

// Returns -1 if any of the channels fails, 0 otherwise
int GepChannelArray::SendMessage(const ::google::protobuf::Message &msg) {
  std::lock_guard<std::recursive_mutex> lock(gep_channel_vector_lock_);
  // send the message to all open GepChannel's
  int ret = 0;
  for (auto &gep_channel_ptr : gep_channel_vector_) {
    if (gep_channel_ptr->IsOpenSocket())
      if (gep_channel_ptr->SendMessage(msg) < 0)
        ret = -1;
  }
  return ret;
}

// Returns -1 if the message couldn't be sent, 0 otherwise
int GepChannelArray::SendMessage(const ::google::protobuf::Message &msg,
                                 int id) {
  std::lock_guard<std::recursive_mutex> lock(gep_channel_vector_lock_);
  // send the message to a specific GepChannel's
  for (auto &gep_channel_ptr : gep_channel_vector_) {
    if (gep_channel_ptr->IsOpenSocket() && id == gep_channel_ptr->GetId())
      return gep_channel_ptr->SendMessage(msg);
  }
  return -1;
}

int GepChannelArray::GetVectorSize() {
  std::lock_guard<std::recursive_mutex> lock(gep_channel_vector_lock_);
  return gep_channel_vector_.size();
}

int GepChannelArray::GetVectorSocket(int i) {
  std::lock_guard<std::recursive_mutex> lock(gep_channel_vector_lock_);
  return gep_channel_vector_[i]->GetSocket();
}

void GepChannelArray::GetVectorReadFds(int *max_fds, fd_set *read_fds) {
  std::lock_guard<std::recursive_mutex> lock(gep_channel_vector_lock_);
  for (const auto &gep_channel_ptr : gep_channel_vector_) {
    int socket = gep_channel_ptr->GetSocket();
    if (socket < 0) {
      gep_log(LOG_ERROR,
              "%s(*):Error-invalid client socket (%i)",
              name_.c_str(), gep_channel_ptr->GetId());
      continue;
    }
    FD_SET(socket, read_fds);
    *max_fds = std::max(socket, *max_fds);
  }
}

void GepChannelArray::RecvData(fd_set *read_fds) {
  std::lock_guard<std::recursive_mutex> lock(gep_channel_vector_lock_);
  // On all opened channels:
  //   * handle incoming requests from GEP clients
  //   * check for timeout
  for (auto it = gep_channel_vector_.begin();
       it != gep_channel_vector_.end(); ) {
    std::shared_ptr<GepChannel> gep_channel_ptr = *it;
    int socket = gep_channel_ptr->GetSocket();
    if (socket >= 0 && FD_ISSET(socket, read_fds)) {
      if (gep_channel_ptr->RecvData() < 0) {
        it = gep_channel_vector_.erase(it);
        continue;
      }
    }
    ++it;
  }
}
