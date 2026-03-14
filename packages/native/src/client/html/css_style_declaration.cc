#include "css_style_declaration.hh"
#include "napi.h"

namespace HTML {
Napi::FunctionReference *CSSStyleDeclaration::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<CSSStyleDeclaration>();
  methods.push_back(Napi::InstanceWrap<CSSStyleDeclaration>::InstanceMethod("setText", &CSSStyleDeclaration::setText));
  methods.push_back(Napi::InstanceWrap<CSSStyleDeclaration>::InstanceAccessor("display", &CSSStyleDeclaration::getDisplay, &CSSStyleDeclaration::setDisplay, static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  methods.push_back(Napi::InstanceWrap<CSSStyleDeclaration>::InstanceAccessor("pointerEvents", &CSSStyleDeclaration::getPointerEvents, &CSSStyleDeclaration::setPointerEvents, static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  Napi::Function func = DefineClass(env, "CSSStyleDeclaration", methods);
  
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
CSSStyleDeclaration::CSSStyleDeclaration(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<CSSStyleDeclaration>(info), ShadowNode(info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: Wrong number of arguments");
  }

  m_instanceId = info[0].As<Napi::Number>().Int64Value();
}
Napi::Value CSSStyleDeclaration::setText(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value CSSStyleDeclaration::getDisplay(const Napi::CallbackInfo &info) {
  return getProperty(info, "display");
}
void CSSStyleDeclaration::setDisplay(const Napi::CallbackInfo &info, const Napi::Value &value) {
  setProperty(info, "display");
}
Napi::Value CSSStyleDeclaration::getPointerEvents(const Napi::CallbackInfo &info) {
  return getProperty(info, "pointerEvents");
}
void CSSStyleDeclaration::setPointerEvents(const Napi::CallbackInfo &info, const Napi::Value &value) {
  setProperty(info, "pointerEvents");
}
} // namespace HTML