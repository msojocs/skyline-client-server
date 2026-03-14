#ifndef __REQUEST_MESSAGE_EVENT_HH__
#define __REQUEST_MESSAGE_EVENT_HH__
#include "node.hh"
#include <napi.h>

namespace HTML {
class RequestMessageEvent : public Napi::ObjectWrap<RequestMessageEvent>,
                       public ShadowNode {
public:
  RequestMessageEvent(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);

private:
Napi::Value addListener(const Napi::CallbackInfo &info);
Napi::Value dispatch(const Napi::CallbackInfo &info);
Napi::Value dispatchNW(const Napi::CallbackInfo &info);
Napi::Value getListeners(const Napi::CallbackInfo &info);
Napi::Value hasListener(const Napi::CallbackInfo &info);
Napi::Value hasListeners(const Napi::CallbackInfo &info);
Napi::Value removeListener(const Napi::CallbackInfo &info);
};
} // namespace Skyline
#endif // __HERO_SHADOW_NODE__HH__