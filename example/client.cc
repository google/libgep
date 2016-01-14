// simple client

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS
#endif


#include "sgp_client.h"

#include <limits.h>
#include <getopt.h>
#include <unistd.h>

#include "sgp_protocol.h"  // for SGPProtocol, etc
#include "sgp.pb.h"  // for Command1, etc

class MyClient: public SGPClient {
 public:
  MyClient(int port)
    : SGPClient(port) {
    cnt1_ = 0;
    cnt2_ = 0;
    cnt3_ = 0;
    cnt4_ = 0;
  }
  virtual ~MyClient() {};

  // protocol callbacks
  bool Recv(const Command1 &msg) override;
  bool Recv(const Command2 &msg) override;
  bool Recv(const Command3 &msg) override;
  bool Recv(const Command4 &msg) override;

  int cnt1_;
  int cnt2_;
  int cnt3_;
  int cnt4_;
};

bool MyClient::Recv(const Command1 &msg) {
  cnt1_++;
  return true;
}

bool MyClient::Recv(const Command2 &msg) {
  cnt2_++;
  return true;
}

bool MyClient::Recv(const Command3 &msg) {
  cnt3_++;
  return true;
}

bool MyClient::Recv(const Command4 &msg) {
  cnt4_++;
  return true;
}


typedef struct arg_values
{
  int port;
  int cnt1;
  int cnt2;
  int cnt3;
  int cnt4;
  int nrem;
  char** rem;
} arg_values;


// default values
#define DEFAULT_CNT1 1
#define DEFAULT_CNT2 2
#define DEFAULT_CNT3 3
#define DEFAULT_CNT4 4


void usage(char *name)
{
  fprintf(stderr, "usage: %s [options]\n", name);
  fprintf(stderr, "where options are:\n");
  fprintf(stderr, "\t--cnt1 <cnt>:\tSend <cnt> Command1 messages [%i]\n",
      DEFAULT_CNT1);
  fprintf(stderr, "\t--cnt2 <cnt>:\tSend <cnt> Command2 messages [%i]\n",
      DEFAULT_CNT2);
  fprintf(stderr, "\t--cnt3 <cnt>:\tSend <cnt> Command3 messages [%i]\n",
      DEFAULT_CNT3);
  fprintf(stderr, "\t--cnt4 <cnt>:\tSend <cnt> Command4 messages [%i]\n",
      DEFAULT_CNT4);
  fprintf(stderr, "\t-h:\t\tHelp\n");
}

// For long options that have no equivalent short option, use a
// non-character as a pseudo short option, starting with CHAR_MAX + 1.
enum {
  CNT1OPTION = CHAR_MAX + 1,
  CNT2OPTION,
  CNT3OPTION,
  CNT4OPTION,
};


arg_values *parse_args(int argc, char** argv) {
  int c;
  // getopt_long stores the option index here
  int optindex = 0;

  static arg_values values;
  // set default values
  values.cnt1 = DEFAULT_CNT1;
  values.cnt2 = DEFAULT_CNT2;
  values.cnt3 = DEFAULT_CNT3;
  values.cnt4 = DEFAULT_CNT4;

  // long options
  static struct option longopts[] = {
    // matching options to short options
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    // options without a short option match
    {"cnt1", required_argument, NULL, CNT1OPTION},
    {"cnt2", required_argument, NULL, CNT2OPTION},
    {"cnt3", required_argument, NULL, CNT3OPTION},
    {"cnt4", required_argument, NULL, CNT4OPTION},
    {NULL, 0, NULL, 0}
  };

  char *endptr;
  while ((c = getopt_long(argc, argv, "", longopts, &optindex)) != -1) {
    switch (c) {
      case 'p':
        values.port = strtol(optarg, &endptr, 0);
        if (*endptr != '\0')
          {
          usage(argv[0]);
          exit(-1);
          }
        break;

      case CNT1OPTION:
        values.cnt1 = strtol(optarg, &endptr, 0);
        if (*endptr != '\0')
          {
          usage(argv[0]);
          exit(-1);
          }
        break;

      case CNT2OPTION:
        values.cnt2 = strtol(optarg, &endptr, 0);
        if (*endptr != '\0')
          {
          usage(argv[0]);
          exit(-1);
          }
        break;

      case CNT3OPTION:
        values.cnt3 = strtol(optarg, &endptr, 0);
        if (*endptr != '\0')
          {
          usage(argv[0]);
          exit(-1);
          }
        break;

      case CNT4OPTION:
        values.cnt4 = strtol(optarg, &endptr, 0);
        if (*endptr != '\0')
          {
          usage(argv[0]);
          exit(-1);
          }
        break;

      case '?':
      case 'h':
        // getopt_long already printed an error message
        usage(argv[0]);
        exit(0);
        break;

      default:
        printf("Unsupported option: %c\n", c);
        usage(argv[0]);
    }
  }

  // store remaining arguments
  values.nrem = argc - optind;
  values.rem = argv + optind;

  return &values;
}


int main(int argc, char **argv) {
  arg_values *values;

  // parse args
  values = parse_args(argc, argv);

  // create the client
  std::unique_ptr<MyClient> my_client;
  my_client.reset(new MyClient(values->port));

  // start the client
  int tries = 0;
  while (my_client->Start() < 0) {
    if (++tries > 3) {
      fprintf(stderr, "error: cannot start client (tried %d times)\n", tries);
      return -1;
    }
    sleep(1);
  }

  // send some messages
  Command1 cmd1;
  for (int i = 0; i < values->cnt1; ++i)
    my_client->Send(cmd1);
  Command2 cmd2;
  for (int i = 0; i < values->cnt2; ++i)
    my_client->Send(cmd2);
  Command3 cmd3;
  for (int i = 0; i < values->cnt3; ++i)
    my_client->Send(cmd3);
  Command4 cmd4;
  for (int i = 0; i < values->cnt4; ++i)
    my_client->Send(cmd4);

  // give some time for all both sides to finish
  usleep(3000000);

  printf("results: %i %i %i %i\n", my_client->cnt1_, my_client->cnt2_,
      my_client->cnt3_, my_client->cnt4_);

  my_client->Stop();

  return 0;
}
