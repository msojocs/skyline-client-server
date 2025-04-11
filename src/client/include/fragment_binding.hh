#ifndef __FRAGMENT_BINDING_HH__
#define __FRAGMENT_BINDING_HH__
#include "base_client.hh"
#include "napi.h"


namespace Skyline {
  class FragmentBinding : public Napi::ObjectWrap<FragmentBinding>, public BaseClient {
  public:
    static Napi::FunctionReference *GetClazz(Napi::Env env);
    Napi::Value getInstanceId(const Napi::CallbackInfo &info);
    FragmentBinding(const Napi::CallbackInfo &info);
  private:
    Napi::Value appendChild(const Napi::CallbackInfo &info);
    Napi::Value associateComponent(const Napi::CallbackInfo &info);
    Napi::Value equal(const Napi::CallbackInfo &info);
    Napi::Value findChildPosition(const Napi::CallbackInfo &info);
    Napi::Value getChildNode(const Napi::CallbackInfo &info);
    Napi::Value getParentNode(const Napi::CallbackInfo &info);
    Napi::Value insertChild(const Napi::CallbackInfo &info);
    Napi::Value release(const Napi::CallbackInfo &info);
    Napi::Value removeChild(const Napi::CallbackInfo &info);
    Napi::Value replaceChild(const Napi::CallbackInfo &info);
    Napi::Value splice(const Napi::CallbackInfo &info);
  };
} // namespace Skyline

#endif // __FRAGMENT_BINDING_HH__