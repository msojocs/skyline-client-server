#include "node.hh"
#include "../../../include/convert.hh"
#include "../../websocket.hh"
#include "napi.h"

namespace Skyline {
ShadowNode::ShadowNode(const Napi::CallbackInfo &info) {
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
Napi::Value ShadowNode::getInstanceId(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), m_instanceId);
}
Napi::Value ShadowNode::setStyleScope(const Napi::CallbackInfo &info) {
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
Napi::Value ShadowNode::addClass(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
  return info.Env().Undefined();
}
Napi::Value ShadowNode::setStyle(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
  return info.Env().Undefined();
}
Napi::Value
ShadowNode::setEventDefaultPrevented(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
  return info.Env().Undefined();
}
Napi::Value ShadowNode::appendChild(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  // 参数1转换为ShadowNode并取得InstanceId
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env = info.Env();
  auto t = result["returnValue"];
  return Convert::convertJson2Value(env, t);
}
Napi::Value ShadowNode::spliceAppend(const Napi::CallbackInfo &info) {
  nlohmann::json args;

  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);

  auto env = info.Env();
  return Convert::convertJson2Value(env, result["returnValue"]);
}
Napi::Value ShadowNode::setId(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env = info.Env();
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}
Napi::Value ShadowNode::forceDetached(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env = info.Env();
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}

Napi::Value ShadowNode::spliceRemove(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env = info.Env();
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}

Napi::Value ShadowNode::release(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env = info.Env();
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}
Napi::Value ShadowNode::setAttributes(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env = info.Env();
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}

Napi::Value ShadowNode::getBoundingClientRect(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env = info.Env();
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}

} // namespace Skyline