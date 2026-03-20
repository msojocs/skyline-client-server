#include "web_request_event.hh"
#include "napi.h"
#include "node.hh"

namespace HTML {
Napi::FunctionReference *WebRequestEvent::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<WebRequestEvent>();
  methods.push_back(InstanceMethod("addListener", &WebRequestEvent::addListener, napi_enumerable));
  methods.push_back(InstanceMethod("hasListener", &WebRequestEvent::hasListener, napi_enumerable));
  methods.push_back(InstanceMethod("removeListener", &WebRequestEvent::removeListener, napi_enumerable));

  Napi::Function func = DefineClass(env, "WebRequestEvent", methods);

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
WebRequestEvent::WebRequestEvent(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<WebRequestEvent>(info), ShadowNode(info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: Wrong number of arguments");
  }
  m_instanceId = info[0].As<Napi::Number>().Int64Value();
}
Napi::Value WebRequestEvent::addListener(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value WebRequestEvent::addRules(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value WebRequestEvent::getRules(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value WebRequestEvent::hasListener(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value WebRequestEvent::hasListeners(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value WebRequestEvent::removeListener(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value WebRequestEvent::removeRules(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

} // namespace HTML