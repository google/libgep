// GEP protocol: common.

#ifndef _GEP_COMMON_H_
#define _GEP_COMMON_H_

#ifndef GEP_LITE
# include <google/protobuf/message.h>  // for Message
# define GepProtobufMessage ::google::protobuf::Message
#else
# include <google/protobuf/message_lite.h>  // for MessageLite
# define GepProtobufMessage ::google::protobuf::MessageLite
#endif

#endif  // _GEP_COMMON_H_
