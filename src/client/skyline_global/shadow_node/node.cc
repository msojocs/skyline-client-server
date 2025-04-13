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
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
  return info.Env().Undefined();
}
Napi::Value ShadowNode::addClass(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
  return info.Env().Undefined();
}
Napi::Value ShadowNode::setStyle(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
  return info.Env().Undefined();
}
Napi::Value
ShadowNode::setEventDefaultPrevented(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
  return info.Env().Undefined();
}
Napi::Value ShadowNode::appendChild(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  // 参数1转换为ShadowNode并取得InstanceId
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto t = result["returnValue"];
  return Convert::convertJson2Value(env, t);
}
Napi::Value ShadowNode::spliceAppend(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  auto env = info.Env();

  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);

  return Convert::convertJson2Value(env, result["returnValue"]);
}
Napi::Value ShadowNode::setId(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}
Napi::Value ShadowNode::forceDetached(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}

Napi::Value ShadowNode::spliceRemove(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}

Napi::Value ShadowNode::release(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}
Napi::Value ShadowNode::setAttributes(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}

Napi::Value ShadowNode::getBoundingClientRect(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}

// isConnected
Napi::Value ShadowNode::isConnected(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicPropertyGetSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}
// setClass 
Napi::Value ShadowNode::setClass(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}
// setListenerOption
Napi::Value ShadowNode::setListenerOption(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}
Napi::Value ShadowNode::matches(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}
Napi::Value ShadowNode::removeChild(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}
Napi::Value ShadowNode::setLayoutCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}
Napi::Value ShadowNode::setTouchEventNeedsLocalCoords(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}

} // namespace Skyline