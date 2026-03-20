#include "request_rule.hh"
#include "napi.h"

namespace HTML {
Napi::FunctionReference *RequestRule::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<RequestRule>();
  methods.push_back(InstanceMethod("getRules", &RequestRule::getRules));
  methods.push_back(InstanceMethod("addRules", &RequestRule::addRules));
  methods.push_back(InstanceMethod("removeRules", &RequestRule::removeRules));
  Napi::Function func = DefineClass(env, "RequestRule", methods);

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
RequestRule::RequestRule(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<RequestRule>(info), ShadowNode(info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: Wrong number of arguments");
  }

  m_instanceId = info[0].As<Napi::Number>().Int64Value();
}

Napi::Value RequestRule::addRules(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value RequestRule::getRules(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value RequestRule::removeRules(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

} // namespace HTML