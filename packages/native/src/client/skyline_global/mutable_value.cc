#include "mutable_value.hh"
#include "../../common/convert.hh"
#include "napi.h"


namespace Skyline {
Napi::FunctionReference *MutableValue::GetClazz(Napi::Env env) {
  auto func = DefineClass(
      env, "MutableValue",
      {
        InstanceMethod("installGetter", &MutableValue::installGetter, static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
        InstanceAccessor("value", &MutableValue::getValue, &MutableValue::setValue, static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
        InstanceAccessor("_value", &MutableValue::getValue, &MutableValue::setValue, static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
        InstanceAccessor("_animation", &MutableValue::getAnimation, &MutableValue::setAnimation, static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
        InstanceAccessor("_windowId", &MutableValue::getWindowId, &MutableValue::setWindowId, static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
        
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

  m_instanceId = info[0].As<Napi::Number>().Int64Value();
}

Napi::Value MutableValue::installGetter(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  return sendToServerSync(info, __func__);
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
void MutableValue::setAnimation(const Napi::CallbackInfo &info, const Napi::Value &value) {
  setProperty(info, "_animation");
}
Napi::Value MutableValue::getWindowId(const Napi::CallbackInfo &info) {
  return getProperty(info, "_windowId");
}
void MutableValue::setWindowId(const Napi::CallbackInfo &info, const Napi::Value &value) {
  setProperty(info, "_windowId");
}
} // namespace Skyline