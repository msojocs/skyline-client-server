#include "event.hh"
#include "napi.h"
#include "node.hh"

namespace HTML {
Napi::FunctionReference *Event::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<Event>();
  methods.push_back(InstanceAccessor("dialog", &Event::getDialog, nullptr, static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  methods.push_back(InstanceAccessor("messageText", &Event::getMessageText, nullptr, static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  methods.push_back(InstanceAccessor("messageType", &Event::getMessageType, nullptr, static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  methods.push_back(InstanceMethod("preventDefault", &Event::preventDefault, napi_enumerable));
  methods.push_back(InstanceAccessor("returnValue", nullptr, &Event::setReturnValue, static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  methods.push_back(InstanceAccessor("type", &Event::getType, nullptr, static_cast<napi_property_attributes>(napi_writable | napi_configurable)));

  Napi::Function func = DefineClass(env, "Event", methods);

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
Event::Event(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<Event>(info), ShadowNode(info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: Wrong number of arguments");
  }

  m_instanceId = info[0].As<Napi::Number>().Int64Value();
}

Napi::Value Event::getDialog(const Napi::CallbackInfo &info) {
  return getProperty(info, "dialog");
}
Napi::Value Event::getMessageText(const Napi::CallbackInfo &info) {
  return getProperty(info, "messageText");
}
Napi::Value Event::getMessageType(const Napi::CallbackInfo &info) {
  return getProperty(info, "messageType");
}
Napi::Value Event::getType(const Napi::CallbackInfo &info) {
  return getProperty(info, "type");
}
Napi::Value Event::preventDefault(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
void Event::setReturnValue(const Napi::CallbackInfo &info, const Napi::Value &value) {
  setProperty(info, "returnValue");
}
} // namespace HTML
