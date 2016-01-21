// GEP/SGP protocol: client implementation.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS
#endif


#include "sgp_client.h"

#include <gep_client.h>  // for GepClient

#include "sgp_protocol.h"  // for SGPProtocol, etc
#ifndef GEP_LITE
#include "sgp.pb.h"  // for Command1, etc
#else
#include "sgp_lite.pb.h"  // for Command1, etc
#endif

SGPClient::SGPClient()
  : GepClient("sgp_client",
              reinterpret_cast<void *>(this),
              new SGPProtocol(),
              &kSGPClientOps) {
}

SGPClient::SGPClient(int port)
  : GepClient("sgp_client",
              reinterpret_cast<void *>(this),
              new SGPProtocol(port),
              &kSGPClientOps) {
}

SGPClient::~SGPClient() {
}
