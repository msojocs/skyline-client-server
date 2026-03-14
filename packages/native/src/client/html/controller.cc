#include "controller.hh"
#include <spdlog/spdlog.h>

namespace HTML {
Napi::FunctionReference *Controller::GetClazz(Napi::Env env) {
  std::vector<Napi::ClassPropertyDescriptor<Controller>> methods;
  methods.push_back(Napi::InstanceWrap<Controller>::InstanceMethod("mount", &Controller::mount));
  methods.push_back(Napi::InstanceWrap<Controller>::InstanceMethod("unmount", &Controller::unmount));
  methods.push_back(Napi::InstanceWrap<Controller>::InstanceAccessor("webview", &Controller::getWebview, nullptr, static_cast<napi_property_attributes>(napi_configurable | napi_writable)));

  Napi::Function func = DefineClass(env, "Controller", methods);

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
Controller::Controller(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<Controller>(info) {
  m_instanceId = sendConstructorToServerSync(info, __func__);
}
Napi::Value Controller::getWebview(const Napi::CallbackInfo &info) {
  return getProperty(info, "webview");
}
Napi::Value Controller::mount(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value Controller::unmount(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
}