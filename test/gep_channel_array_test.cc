#include "gep_channel_array.h"

#include <unistd.h>  // for socklen_t, ssize_t

#include "gep_channel_array.h"  // for GepChannelArray
#include "gep_protocol.h"  // for GepProtocol
#include "gep_test_lib.h"  // for TestServer, GepTest
#include "gtest/gtest.h"  // for EXPECT_EQ, TEST_F
#include "socket_interface.h"  // for SocketInterface


class FailingSocketInterface : public SocketInterface {
 public:
  FailingSocketInterface() :
    socket_fail_(false),
    recv_fail_(false),
    bind_fail_(false),
    listen_fail_(false),
    getport_fail_(false),
    setreuseaddr_fail_(false) {
  }
  virtual ~FailingSocketInterface() {}
  virtual int Socket(int domain, int type, int protocol) {
    return socket_fail_ ? -1 :
           SocketInterface::Socket(domain, type, protocol);
  }
  virtual ssize_t Recv(int sockfd, void *buf, size_t len, int flags) {
    return recv_fail_ ? -1 :
           SocketInterface::Recv(sockfd, buf, len, flags);
  }
  virtual int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return bind_fail_ ? -1 :
           SocketInterface::Bind(sockfd, addr, addrlen);
  }
  virtual int Listen(int sockfd, int backlog) {
    return listen_fail_ ? -1 :
           SocketInterface::Listen(sockfd, backlog);
  }
  virtual int GetPort(const char *log_module, int sock, int *port) {
    return getport_fail_ ? -1 :
           SocketInterface::GetPort(log_module, sock, port);
  }
  virtual int SetReuseAddr(const char *log_module, int sock) {
    return setreuseaddr_fail_ ? -1 :
           SocketInterface::SetReuseAddr(log_module, sock);
  }
  bool socket_fail_;
  bool recv_fail_;
  bool bind_fail_;
  bool listen_fail_;
  bool getport_fail_;
  bool setreuseaddr_fail_;
};

class GepChannelArrayTest : public GepTest {
};

TEST_F(GepChannelArrayTest, FailingSendSocket) {
  // stop the GepServer so it does not interfere
  server_->Stop();

  // create an independent GepChannelArray
  GepChannelArray *gca = server_->GetGepChannelArray();
  FailingSocketInterface failing_socket_interface;
  SocketInterface *old_socket_interface = gca->GetSocketInterface();
  gca->SetSocketInterface(&failing_socket_interface);

  // check different failure modes
  gca->Stop();
  failing_socket_interface.socket_fail_ = true;
  EXPECT_EQ(-1, gca->OpenServerSocket());
  failing_socket_interface.socket_fail_ = false;

  gca->Stop();
  failing_socket_interface.setreuseaddr_fail_ = true;
  EXPECT_EQ(-1, gca->OpenServerSocket());
  failing_socket_interface.setreuseaddr_fail_ = false;

  gca->Stop();
  failing_socket_interface.bind_fail_ = true;
  EXPECT_EQ(-1, gca->OpenServerSocket());
  failing_socket_interface.bind_fail_ = false;

  gca->Stop();
  failing_socket_interface.listen_fail_ = true;
  EXPECT_EQ(-1, gca->OpenServerSocket());
  failing_socket_interface.listen_fail_ = false;

  gca->Stop();
  failing_socket_interface.getport_fail_ = true;
  server_->GetProto()->SetPort(0);
  EXPECT_EQ(-1, gca->OpenServerSocket());
  failing_socket_interface.getport_fail_ = false;

  // reinstante the socket interface
  // ensure the server is up before teardown
  server_->Start();
  gca->SetSocketInterface(old_socket_interface);
}
