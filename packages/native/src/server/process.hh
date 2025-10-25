#ifndef __PROCESS_HH__
#define __PROCESS_HH__

#include <napi.h>

namespace Process {
    // Protobuf-based methods (replacing JSON versions)
    void setMessageCallback(const Napi::CallbackInfo &info);
    Napi::Value start(const Napi::CallbackInfo &info);
    void stop(const Napi::CallbackInfo &info);
    Napi::Value sendMessageSync(const Napi::CallbackInfo &info);
    Napi::Value sendMessageSingle(const Napi::CallbackInfo &info);
}

#endif // __PROCESS_HH__
