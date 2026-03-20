#ifndef __CONTROLLER_HH__
#define __CONTROLLER_HH__

#include "../base_client.hh"
#include "napi.h"

namespace HTML {

class Controller : public Napi::ObjectWrap<Controller>, public BaseClient {
public:
  static Napi::FunctionReference *GetClazz(Napi::Env env);
  Controller(const Napi::CallbackInfo &info);

private:
  Napi::Value getWebview(const Napi::CallbackInfo &info);
  Napi::Value mount(const Napi::CallbackInfo &info);
  Napi::Value unmount(const Napi::CallbackInfo &info);
  static Napi::Value connect(const Napi::CallbackInfo &info);
};

} // namespace HTML

#endif