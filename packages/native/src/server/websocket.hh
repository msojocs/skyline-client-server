#ifndef __WEBSOCKET_HH__
#define __WEBSOCKET_HH__
#include "napi.h"

namespace WebSocketServer {
  Napi::Number start(const Napi::CallbackInfo &info);
  void stop(const Napi::CallbackInfo &info);
  void setMessageCallback(const Napi::CallbackInfo &info);
  Napi::Value sendMessageSync(const Napi::CallbackInfo &info);
}
#endif