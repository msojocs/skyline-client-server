#include "../include/runtime.hh"
#include "../../include/convert.hh"
#include "../websocket.hh"
#include "napi.h"
#include <nlohmann/json.hpp>

namespace Skyline {
namespace Runtime {

/**
 * "registerTouchCallback", "registerMouseCallback",
 * "registerTransitionCallback", "registerAnimationCallback",
 * "registerTriggerEventCallback", "registerPerformanceCallback",
 * "registerNavigateBackInterceptCallback", "registerJsValue",
 * "unRegisterJsValue", "getJsValueById", "registerFontFaceCallback",
 * "setFeatureFlags", "updateMapCustomCallout", "preloadAssets"
 */

void registerTouchCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  nlohmann::json args;
  args[0] = Convert::convertValue2Json(env, info[0]);
  // 发送消息到 WebSocket
  WebSocket::callStaticSync("SkylineRuntime", __func__, args);
}
void registerMouseCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  nlohmann::json args;
  args[0] = Convert::convertValue2Json(env, info[0]);
  // 发送消息到 WebSocket
  WebSocket::callStaticSync("SkylineRuntime", __func__, args);
}
void registerTransitionCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  nlohmann::json args;
  args[0] = Convert::convertValue2Json(env, info[0]);
  // 发送消息到 WebSocket
  WebSocket::callStaticSync("SkylineRuntime", __func__, args);
}
void registerAnimationCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  nlohmann::json args;
  args[0] = Convert::convertValue2Json(env, info[0]);
  // 发送消息到 WebSocket
  WebSocket::callStaticSync("SkylineRuntime", __func__, args);
}
void registerTriggerEventCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  nlohmann::json args;
  args[0] = Convert::convertValue2Json(env, info[0]);
  // 发送消息到 WebSocket
  WebSocket::callStaticSync("SkylineRuntime", __func__, args);
}
void registerPerformanceCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  nlohmann::json args;
  args[0] = Convert::convertValue2Json(env, info[0]);
  // 发送消息到 WebSocket
  WebSocket::callStaticSync("SkylineRuntime", __func__, args);
}
void registerNavigateBackInterceptCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  nlohmann::json args;
  args[0] = Convert::convertValue2Json(env, info[0]);
  // 发送消息到 WebSocket
  WebSocket::callStaticSync("SkylineRuntime", __func__, args);
}
Napi::Value registerJsValue(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  // 发送消息到 WebSocket
  auto result = WebSocket::callStaticSync("SkylineRuntime", __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}
Napi::Value unRegisterJsValue(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  // TODO：参数是任意值
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  
  // 发送消息到 WebSocket
  auto result = WebSocket::callStaticSync("SkylineRuntime", __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}
Napi::Value getJsValueById(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsNumber()) {
    throw Napi::Error::New(env, "参数0 id必须为Number类型");
  }
  int id = info[0].As<Napi::Number>().Int32Value();
  nlohmann::json data;
  data[0] = id;
  // 发送消息到 WebSocket
  auto result = WebSocket::callStaticSync("SkylineRuntime", __func__, data);
  auto returnValue = result["returnValue"];
  // TODO: 处理返回值
  return Convert::convertJson2Value(env, returnValue);
}

void registerFontFaceCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  nlohmann::json args;
  args[0] = Convert::convertValue2Json(env, info[0]);
  // 发送消息到 WebSocket
  WebSocket::callStaticSync("SkylineRuntime", __func__, args);
}
void setFeatureFlags(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsObject()) {
    throw Napi::Error::New(env, "参数0 flags必须为Object类型");
  }
  nlohmann::json data;
  data[0] = Convert::convertValue2Json(env, info[0]);
  // 发送消息到 WebSocket
  WebSocket::callStaticSync("SkylineRuntime", __func__, data);
}
void updateMapCustomCallout(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsObject()) {
    throw Napi::Error::New(env, "参数0 flags必须为Object类型");
  }
  nlohmann::json data = Convert::convertValue2Json(env, info[0]);
  // 发送消息到 WebSocket
  WebSocket::callStaticSync("SkylineRuntime", __func__, data);
}

/**
 * TODO: 参数，返回值未确定
 */
Napi::Value preloadAssets(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsString()) {
    throw Napi::Error::New(env, "参数0 url必须为String类型");
  }
  std::string url = info[0].As<Napi::String>().Utf8Value();
  nlohmann::json data;
  data[0] = url;
  // 发送消息到 WebSocket
  auto result = WebSocket::callDynamicSync("SkylineRuntime", __func__, data);
  return Napi::String::New(env, result.dump());
}

void Init(Napi::Env env, Napi::Object exports) {
  auto runtime = Napi::Object::New(env);
  runtime.Set(Napi::String::New(env, "registerTouchCallback"),
              Napi::Function::New(env, &registerTouchCallback));
  runtime.Set(Napi::String::New(env, "registerMouseCallback"),
              Napi::Function::New(env, &registerMouseCallback));
  runtime.Set(Napi::String::New(env, "registerTransitionCallback"),
              Napi::Function::New(env, &registerTransitionCallback));
  runtime.Set(Napi::String::New(env, "registerAnimationCallback"),
              Napi::Function::New(env, &registerAnimationCallback));
  runtime.Set(Napi::String::New(env, "registerTriggerEventCallback"),
              Napi::Function::New(env, &registerTriggerEventCallback));
  runtime.Set(Napi::String::New(env, "registerPerformanceCallback"),
              Napi::Function::New(env, &registerPerformanceCallback));
  runtime.Set(Napi::String::New(env, "registerNavigateBackInterceptCallback"),
              Napi::Function::New(env, &registerNavigateBackInterceptCallback));
  runtime.Set(Napi::String::New(env, "registerJsValue"),
              Napi::Function::New(env, &registerJsValue));
  runtime.Set(Napi::String::New(env, "unRegisterJsValue"),
              Napi::Function::New(env, &unRegisterJsValue));
  runtime.Set(Napi::String::New(env, "getJsValueById"),
              Napi::Function::New(env, &getJsValueById));
  runtime.Set(Napi::String::New(env, "registerFontFaceCallback"),
              Napi::Function::New(env, &registerFontFaceCallback));
  runtime.Set(Napi::String::New(env, "setFeatureFlags"),
              Napi::Function::New(env, &setFeatureFlags));
  runtime.Set(Napi::String::New(env, "updateMapCustomCallout"),
              Napi::Function::New(env, &updateMapCustomCallout));
  runtime.Set(Napi::String::New(env, "preloadAssets"),
              Napi::Function::New(env, &preloadAssets));
  exports.Set(Napi::String::New(env, "runtime"), runtime);
}
} // namespace Runtime
} // namespace Skyline