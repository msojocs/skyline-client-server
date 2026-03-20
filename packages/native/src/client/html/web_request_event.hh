#ifndef __WEB_REQUEST_EVENT__HH__
#define __WEB_REQUEST_EVENT__HH__
#include "node.hh"
#include <napi.h>

namespace HTML {
class WebRequestEvent : public Napi::ObjectWrap<WebRequestEvent>,
                       public ShadowNode {
public:
  WebRequestEvent(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);

private:
  Napi::Value addListener(const Napi::CallbackInfo &info);
  Napi::Value addRules(const Napi::CallbackInfo &info);
  Napi::Value getRules(const Napi::CallbackInfo &info);
  Napi::Value hasListener(const Napi::CallbackInfo &info);
  Napi::Value hasListeners(const Napi::CallbackInfo &info);
  Napi::Value removeListener(const Napi::CallbackInfo &info);
  Napi::Value removeRules(const Napi::CallbackInfo &info);
};
} // namespace HTML
#endif // __WEB_REQUEST_EVENT__HH__