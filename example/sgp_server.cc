// Copyright Google Inc. Apache 2.0.

// GEP/SGP protocol: server side.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS
#endif


#include "sgp_server.h"

#include <gep_server.h>

#include "sgp_protocol.h"  // for SGPProtocol, etc
#ifndef GEP_LITE
#include "sgp.pb.h"  // for Command1, etc
#else
#include "sgp_lite.pb.h"  // for Command1, etc
#endif


SGPServer::SGPServer(int max_channel_num)
    : GepServer("sgp_server",
                max_channel_num,
                reinterpret_cast<void *>(this),
                new SGPProtocol(),
                &kSGPServerOps) {
}

SGPServer::SGPServer(int max_channel_num, int port)
    : GepServer("sgp_server",
                max_channel_num,
                reinterpret_cast<void *>(this),
                new SGPProtocol(port),
                &kSGPServerOps) {
}

SGPServer::~SGPServer() {
}
