#ifndef __SOCKET_SERVER_HH__
#define __SOCKET_SERVER_HH__

#include <napi.h>
#include <memory>
#include <map>
#include <future>
#include <queue>
#include "../common/snowflake.hh"
#include "messages.pb.h"
#include "../common/protobuf_converter.hh"

namespace SocketServer {
    // Protobuf-based methods (replacing JSON versions)
    void setMessageCallback(const Napi::CallbackInfo &info);
    Napi::Value start(const Napi::CallbackInfo &info);
    void stop(const Napi::CallbackInfo &info);
    Napi::Value sendMessageSync(const Napi::CallbackInfo &info);
    Napi::Value sendMessageSingle(const Napi::CallbackInfo &info);
}

#endif // __SOCKET_SERVER_HH__
