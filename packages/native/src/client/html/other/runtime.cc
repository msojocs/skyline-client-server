#include "runtime.hh"
#include "../../common/convert.hh"
#include "../client_action.hh"
#include "napi.h"
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_map>

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
  Napi::Value sendToServerSync(const Napi::CallbackInfo &info, const std::string &methodName) {
    auto env = info.Env();
    nlohmann::json args;
    for (int i = 0; i < info.Length(); i++) {
      args[i] = Convert::convertValue2Json(env, info[i]);
    }
    try {
      auto result = ClientAction::callStaticSync("SkylineRuntime", methodName, args);
      auto returnValue = result["returnValue"];
      return Convert::convertJson2Value(env, returnValue);
    } catch (const std::exception &e) {
      throw Napi::Error::New(env, e.what());
    }
    catch (...) {
      throw Napi::Error::New(env, "Unknown error occurred");
    }
  }
void registerTouchCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void registerMouseCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void registerTransitionCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void registerAnimationCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void registerTriggerEventCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void registerPerformanceCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void registerNavigateBackInterceptCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
static std::unordered_map<int64_t, std::shared_ptr<Napi::Reference<Napi::Value>>> jsValueCache;
Napi::Value registerJsValue(const Napi::CallbackInfo &info) {
  auto jsId = sendToServerSync(info, __func__);
  jsValueCache.emplace(jsId.As<Napi::Number>().Int64Value(), std::make_shared<Napi::Reference<Napi::Value>>(Napi::Persistent(info[0])));
  return jsId;
}
Napi::Value unRegisterJsValue(const Napi::CallbackInfo &info) {
  jsValueCache.erase(info[0].As<Napi::Number>().Int64Value());
  return sendToServerSync(info, __func__);
}
Napi::Value getJsValueById(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "getJsValueById: Wrong number of arguments");
  }
  if (!info[0].IsNumber()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be a number");
  }
  /**
   * Client                     Server
   * registerJsValue
   * worklet函数  ->   worklet函数
   * getJsValue
   * 普通函数 (问题点)  <----  worklet函数
   * 
   * 解决：本地存储删除取用；服务器存储删除，不取用。
   */
  auto jsId = info[0].As<Napi::Number>().Int64Value();
  auto it = jsValueCache.find(jsId);
  if (it != jsValueCache.end()) {
    auto v = it->second;
    return v->Value();
  } else {
    throw Napi::Error::New(info.Env(), "JsValue not found");
  }
}

void registerFontFaceCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void setFeatureFlags(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void updateMapCustomCallout(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}

/**
 * TODO: 参数，返回值未确定
 */
Napi::Value preloadAssets(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
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