#include "../include/mutable_value.hh"
#include "../../include/convert.hh"
#include "../websocket.hh"
#include "napi.h"


namespace Skyline {
Napi::FunctionReference *MutableValue::GetClazz(Napi::Env env) {
  auto func = DefineClass(
      env, "Mutable",
      {InstanceAccessor("value", &MutableValue::getValue, &MutableValue::setValue,
                        static_cast<napi_property_attributes>(
                            napi_writable | napi_configurable)),
      //  InstanceMethod("installGetter", &MutableValue::installGetter),
       InstanceAccessor("instanceId", &MutableValue::getInstanceId, nullptr,
                        static_cast<napi_property_attributes>(
                            napi_writable | napi_configurable)),
      });

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  return constructor;
}
MutableValue::MutableValue(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<MutableValue>(info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: First argument must be a string");
  }
  m_instanceId = info[0].As<Napi::String>().Utf8Value();
}

Napi::Value MutableValue::installGetter(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  // 发送消息到 WebSocket
  nlohmann::json data;
  for (int i = 0; i < info.Length(); i++) {
    data[i] = Convert::convertValue2Json(env, info[i]);
  }
  WebSocket::callStaticSync("Mutable", __func__, data);

  return env.Undefined();
}
Napi::Value MutableValue::getValue(const Napi::CallbackInfo &info) {
  return getProperty(info, "value");
}
void MutableValue::setValue(const Napi::CallbackInfo &info, const Napi::Value &value) {
  setProperty(info, "value");
}
Napi::Value MutableValue::getAnimation(const Napi::CallbackInfo &info) {
  return getProperty(info, "_animation");
}
void MutableValue::setAnimation(const Napi::CallbackInfo &info) {
  setProperty(info, "_animation");
}
Napi::Value MutableValue::getWindowId(const Napi::CallbackInfo &info) {
  return getProperty(info, "_windowId");
}
void MutableValue::setWindowId(const Napi::CallbackInfo &info) {
  setProperty(info, "_windowId");
}
} // namespace Skyline