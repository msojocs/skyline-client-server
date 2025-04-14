#include "../include/worklet_module.hh"
#include "../websocket.hh"
#include "../../include/convert.hh"
#include "napi.h"

namespace Skyline {
namespace WorkletModule {
  Napi::Value installCoreFunctions(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    // 发送消息到 WebSocket
    auto func = info[0].As<Napi::Function>();
    // 从function里面提取数据
    nlohmann::json args {
      {
        {"asString", func.Get("asString").As<Napi::String>().Utf8Value()},
        {"workletHash", func.Get("__workletHash").As<Napi::Number>().Int64Value()},
        {"location", func.Get("__location").As<Napi::String>().Utf8Value()},
        {"isWorklet", func.Get("__worklet").As<Napi::Boolean>().Value()},
      }
    };
    auto result = WebSocket::callCustomHandleSync(__func__, args);
    auto returnValue = result["returnValue"];
    return Convert::convertJson2Value(env, returnValue);
  }
  Napi::Value makeShareable(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    // 发送消息到 WebSocket
    auto func = info[0].As<Napi::Function>();
    // 从function里面提取数据
    nlohmann::json args {
      {
        {"asString", func.Get("asString").As<Napi::String>().Utf8Value()},
        {"workletHash", func.Get("__workletHash").As<Napi::Number>().Int64Value()},
        {"location", func.Get("__location").As<Napi::String>().Utf8Value()},
        {"isWorklet", func.Get("__worklet").As<Napi::Boolean>().Value()},
      }
    };
    auto result = WebSocket::callCustomHandleSync(__func__, args);
    auto returnValue = result["returnValue"];
    return Convert::convertJson2Value(env, returnValue);
  }
  Napi::Value makeMutable(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    // 发送消息到 WebSocket
    nlohmann::json args;
    for (int i = 0; i < info.Length(); i++) {
      args[i] = Convert::convertValue2Json(env, info[i]);
    }
    auto result = WebSocket::callStaticSync("SkylineWorkletModule", __func__, args);
    auto returnValue = result["returnValue"];
    return Convert::convertJson2Value(env, returnValue);
  }
  Napi::Value registerEventHandler(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    nlohmann::json args;
    for (int i = 0; i < info.Length(); i++) {
      args[i] = Convert::convertValue2Json(env, info[i]);
    }
    auto result = WebSocket::callStaticSync("SkylineWorkletModule", __func__, args);
    auto returnValue = result["returnValue"];
    return Convert::convertJson2Value(env, returnValue);
  }
  Napi::Value startMapper(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    nlohmann::json args;
    for (int i = 0; i < info.Length(); i++) {
      args[i] = Convert::convertValue2Json(env, info[i]);
    }
    auto result = WebSocket::callStaticSync("SkylineWorkletModule", __func__, args);
    auto returnValue = result["returnValue"];
    return Convert::convertJson2Value(env, returnValue);
  }
  void Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "installCoreFunctions"), Napi::Function::New(env, installCoreFunctions));
    exports.Set(Napi::String::New(env, "makeShareable"), Napi::Function::New(env, makeShareable));
    exports.Set(Napi::String::New(env, "makeMutable"), Napi::Function::New(env, makeMutable));
    exports.Set(Napi::String::New(env, "registerEventHandler"), Napi::Function::New(env, registerEventHandler));
    exports.Set(Napi::String::New(env, "startMapper"), Napi::Function::New(env, startMapper));
  }
}
}