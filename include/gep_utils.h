// Copyright Google Inc. Apache 2.0.

// GEP protocol: utils.

#ifndef _GEP_UTILS_H_
#define _GEP_UTILS_H_

#include "gep_channel.h"  // for GepChannel
#include "gep_common.h"  // for GepProtobufMessage

// These callbacks, which can be fed to the VFT, gets the actual object from
// the context variable, and then calls the corresponding protocol object
// callback.
template<typename Class, typename SpecificMessage>
bool RecvMessageId(const GepProtobufMessage &msg, void *context) {
  const SpecificMessage &smsg = dynamic_cast<const SpecificMessage &>(msg);
  // send specific msg to object callback
  GepChannel *gep_channel = static_cast<GepChannel *>(context);
  Class* that = static_cast<Class *>(gep_channel->GetContext());
  return that->Recv(smsg, gep_channel->GetId());
}

template<typename Class, typename SpecificMessage>
bool RecvMessage(const GepProtobufMessage &msg, void *context) {
  const SpecificMessage &smsg = dynamic_cast<const SpecificMessage &>(msg);
  // send specific msg to object callback
  GepChannel *gep_channel = static_cast<GepChannel *>(context);
  Class* that = static_cast<Class *>(gep_channel->GetContext());
  return that->Recv(smsg);
}

#endif  // _GEP_UTILS_H_
