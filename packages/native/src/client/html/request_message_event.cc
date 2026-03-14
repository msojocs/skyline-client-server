#include "request_message_event.hh"
#include "napi.h"
#include "node.hh"

namespace HTML {
Napi::FunctionReference *RequestMessageEvent::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<RequestMessageEvent>();
  methods.push_back(InstanceMethod("addListener", &RequestMessageEvent::addListener));
  methods.push_back(InstanceMethod("dispatch", &RequestMessageEvent::dispatch));
  methods.push_back(InstanceMethod("dispatchNW", &RequestMessageEvent::dispatchNW));
  methods.push_back(InstanceMethod("getListeners", &RequestMessageEvent::getListeners));
  methods.push_back(InstanceMethod("hasListener", &RequestMessageEvent::hasListener));
  methods.push_back(InstanceMethod("hasListeners", &RequestMessageEvent::hasListeners));
  methods.push_back(InstanceMethod("removeListener", &RequestMessageEvent::removeListener));

  Napi::Function func = DefineClass(env, "RequestMessageEvent", methods);

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
RequestMessageEvent::RequestMessageEvent(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<RequestMessageEvent>(info), ShadowNode(info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: Wrong number of arguments");
  }

  m_instanceId = info[0].As<Napi::Number>().Int64Value();
}
Napi::Value RequestMessageEvent::addListener(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value RequestMessageEvent::removeListener(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value RequestMessageEvent::dispatch(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value RequestMessageEvent::dispatchNW(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value RequestMessageEvent::getListeners(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value RequestMessageEvent::hasListener(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value RequestMessageEvent::hasListeners(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
} // namespace HTML