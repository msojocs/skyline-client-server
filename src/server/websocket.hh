#ifndef __WEBSOCKET_HH__
#define __WEBSOCKET_HH__
#include "napi.h"

namespace WebSocketServer {
  Napi::Number start(Napi::CallbackInfo &info);
  void stop(Napi::CallbackInfo &info);
  void setMessageCallback(Napi::CallbackInfo &info);
  Napi::Value sendMessageSync(Napi::CallbackInfo &info);
}
#endif