#include "../include/view_shadow_node.hh"
#include "../../include/convert.hh"
#include "../websocket.hh"
#include "napi.h"

namespace Skyline {
Napi::FunctionReference *ViewShadowNode::GetClazz(Napi::Env env) {

  Napi::Function func = DefineClass(
      env, "ViewShadowNode",
      {
          InstanceMethod("setStyleScope", &ViewShadowNode::setStyleScope),
          InstanceMethod("addClass", &ViewShadowNode::addClass),
          InstanceMethod("setStyle", &ViewShadowNode::setStyle),
          InstanceMethod("setEventDefaultPrevented",
                         &ViewShadowNode::setEventDefaultPrevented),
          InstanceMethod("appendChild", &ViewShadowNode::appendChild),
          InstanceMethod("spliceAppend", &ViewShadowNode::spliceAppend),
          // getter instanceId
          InstanceAccessor<&ViewShadowNode::getInstanceId>(
              "instanceId", static_cast<napi_property_attributes>(
                                napi_writable | napi_configurable)),
      });
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
ViewShadowNode::ViewShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<ViewShadowNode>(info) {
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
Napi::Value ViewShadowNode::getInstanceId(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), m_instanceId);
}
Napi::Value ViewShadowNode::setStyleScope(const Napi::CallbackInfo &info) {
  if (info.Length() < 2) {
    throw Napi::TypeError::New(info.Env(),
                               "setStyleScope: Wrong number of arguments");
  }
  nlohmann::json args;
  args[0] = Convert::convertValue2Json(info[0]);
  args[1] = Convert::convertValue2Json(info[1]);
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
  return info.Env().Undefined();
}
Napi::Value ViewShadowNode::addClass(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
  return info.Env().Undefined();
}
Napi::Value ViewShadowNode::setStyle(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
  return info.Env().Undefined();
}
Napi::Value
ViewShadowNode::setEventDefaultPrevented(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
  return info.Env().Undefined();
}
Napi::Value ViewShadowNode::appendChild(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  // 调试log
  auto log = info.Env()
                 .Global()
                 .Get("console")
                 .As<Napi::Object>()
                 .Get("log")
                 .As<Napi::Function>();
  log.Call({Napi::String::New(info.Env(), "appendChild")});
  log.Call({Napi::String::New(info.Env(), "Arguments length: " +
                                              std::to_string(info.Length()))});
  // 调试参数
  for (int i = 0; i < info.Length(); i++) {
    log.Call({info[i]});
  }
  // 参数1转换为ViewShadowNode并取得InstanceId
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env = info.Env();
  auto t = result["returnValue"];
  return Convert::convertJson2Value(env, t);
}
Napi::Value ViewShadowNode::spliceAppend(const Napi::CallbackInfo &info) {
  nlohmann::json args;

  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);

  auto env = info.Env();
  return Convert::convertJson2Value(env, result["returnValue"]);
}

} // namespace Skyline