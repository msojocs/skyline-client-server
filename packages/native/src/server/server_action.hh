#ifndef __SERVER_ACTION_HH__
#define __SERVER_ACTION_HH__

#include <napi.h>
#include <nlohmann/json.hpp>

namespace ServerAction {
    void setMessageCallback(const Napi::CallbackInfo &info);
    Napi::Number start(const Napi::CallbackInfo &info);
    void stop(const Napi::CallbackInfo &info);
    Napi::Value sendMessageSync(const Napi::CallbackInfo &info);

}

#endif // __SOCKET_SERVER_HH__
