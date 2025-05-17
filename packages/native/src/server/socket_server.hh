#ifndef __SOCKET_SERVER_HH__
#define __SOCKET_SERVER_HH__

#include <napi.h>
#include <memory>
#include <map>
#include <future>
#include <queue>
#include "../common/snowflake.hh"
#include <nlohmann/json.hpp>

namespace SocketServer {
    void setMessageCallback(const Napi::CallbackInfo &info);
    Napi::Number start(const Napi::CallbackInfo &info);
    void stop(const Napi::CallbackInfo &info);
    Napi::Value sendMessageSync(const Napi::CallbackInfo &info);

}

#endif // __SOCKET_SERVER_HH__
