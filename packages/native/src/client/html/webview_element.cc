#include "webview_element.hh"
#include "napi.h"
#include "node.hh"
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace HTML {
Napi::FunctionReference *WebviewElement::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<WebviewElement>();
  methods.push_back(InstanceAccessor("request", &WebviewElement::getRequest, nullptr, static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  methods.push_back(InstanceAccessor("src", &WebviewElement::getSrc, &WebviewElement::setSrc, static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  methods.push_back(InstanceAccessor("style", &WebviewElement::getStyle, &WebviewElement::setStyle, static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  methods.push_back(InstanceAccessor("ondialog", nullptr, &WebviewElement::setOndialog, static_cast<napi_property_attributes>(napi_writable | napi_configurable)));

  methods.push_back(InstanceMethod("addEventListener", &WebviewElement::addEventListener, napi_enumerable));
  methods.push_back(InstanceMethod("executeScript", &WebviewElement::executeScript, napi_enumerable));
  methods.push_back(InstanceMethod("getAttribute", &WebviewElement::getAttribute, napi_enumerable));
  methods.push_back(InstanceMethod("getUserAgent", &WebviewElement::getUserAgent, napi_enumerable));
  methods.push_back(InstanceMethod("removeEventListener", &WebviewElement::removeEventListener, napi_enumerable));
  methods.push_back(InstanceMethod("setUserAgentOverride", &WebviewElement::setUserAgentOverride, napi_enumerable));
  methods.push_back(InstanceMethod("showDevTools", &WebviewElement::showDevTools, static_cast<napi_property_attributes>(napi_writable | napi_configurable | napi_enumerable)));
  Napi::Function func = DefineClass(env, "WebviewElement", methods);
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}

WebviewElement::WebviewElement(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<WebviewElement>(info), ShadowNode(info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: Wrong number of arguments");
  }

  m_instanceId = info[0].As<Napi::Number>().Int64Value();
}

Napi::Value WebviewElement::addEventListener(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value WebviewElement::executeScript(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value WebviewElement::getAttribute(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value WebviewElement::getUserAgent(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value WebviewElement::removeEventListener(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
void WebviewElement::setOndialog(const Napi::CallbackInfo &info, const Napi::Value &value) {
  setProperty(info, "ondialog");
}
Napi::Value WebviewElement::setUserAgentOverride(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value WebviewElement::showDevTools(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

Napi::Value WebviewElement::getRequest(const Napi::CallbackInfo &info) {
  return getProperty(info, "request");
}

Napi::Value WebviewElement::getSrc(const Napi::CallbackInfo &info) {
  return getProperty(info, "src");
}
void WebviewElement::setSrc(const Napi::CallbackInfo &info, const Napi::Value &value) {
  setProperty(info, "src");
}

Napi::Value WebviewElement::getStyle(const Napi::CallbackInfo &info) {
  return getProperty(info, "style");
}
void WebviewElement::setStyle(const Napi::CallbackInfo &info, const Napi::Value &value) {
  setProperty(info, "style");
}

} // namespace HTML
