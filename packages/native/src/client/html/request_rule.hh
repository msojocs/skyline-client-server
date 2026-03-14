#ifndef __REQUEST_RULE_HH__
#define __REQUEST_RULE_HH__
#include "node.hh"
#include <napi.h>


namespace HTML {
class RequestRule : public Napi::ObjectWrap<RequestRule>,
                        public ShadowNode {
public:
  RequestRule(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);

private:
  Napi::Value addRules(const Napi::CallbackInfo &info);
  Napi::Value getRules(const Napi::CallbackInfo &info);
  Napi::Value removeRules(const Napi::CallbackInfo &info);
};
} // namespace HTML
#endif // __IMAGE_SHADOW_NODE__HH__