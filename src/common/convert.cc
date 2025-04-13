#include "../include/convert.hh"
#include "../client/include/async_style_sheets.hh"
#include "../client/include/fragment_binding.hh"
#include "../client/include/mutable_value.hh"
#include "../client/skyline_global/shadow_node/list_view.hh"
#include "../client/skyline_global/shadow_node/scroll_view.hh"
#include "../client/skyline_global/shadow_node/sticky_header.hh"
#include "../client/skyline_global/shadow_node/sticky_section.hh"
#include "../client/skyline_global/shadow_node/text.hh"
#include "../client/skyline_global/shadow_node/view.hh"
#include "../client/websocket.hh"
#include "../include/snowflake.hh"
#include "napi.h"
#include <nlohmann/json_fwd.hpp>

namespace Convert {

static std::map<std::string, CallbackData> callback;
using snowflake_t = snowflake<1534832906275L>;
snowflake_t callbackUuid;

static std::map<std::string, Napi::FunctionReference *> funcMap;

#ifdef _SKYLINE_CLIENT_

CallbackData * find_callback(const std::string &callbackId)
{
  auto it = callback.find(callbackId);
  if (it != callback.end()) {
    return &it->second;
  }
  return nullptr;
}
void RegisteInstanceType(Napi::Env &env) {
  callbackUuid.init(2, 1);
  // 注册实例类型和对应的构造函数
  funcMap["AsyncStylesheets"] = Skyline::AsyncStyleSheets::GetClazz(env);
  funcMap["TextShadowNode"] = Skyline::TextShadowNode::GetClazz(env);
  funcMap["ViewShadowNode"] = Skyline::ViewShadowNode::GetClazz(env);
  funcMap["ScrollViewShadowNode"] = Skyline::ScrollViewShadowNode::GetClazz(env);
  funcMap["ListViewShadowNode"] = Skyline::ListViewShadowNode::GetClazz(env);
  funcMap["StickySectionShadowNode"] = Skyline::StickySectionShadowNode::GetClazz(env);
  funcMap["StickyHeaderShadowNode"] = Skyline::StickyHeaderShadowNode::GetClazz(env);
  funcMap["FragmentBinding"] = Skyline::FragmentBinding::GetClazz(env);
  funcMap["MutableValue"] = Skyline::MutableValue::GetClazz(env);
}
#endif

nlohmann::json convertObject2Json(Napi::Env &env, const Napi::Value &value, bool isSyncCallback) {
  Napi::Object obj = value.As<Napi::Object>();
  if (obj.Get("instanceId").IsString()) {
    nlohmann::json jsonObj;
    jsonObj["instanceId"] =
        obj.Get("instanceId").As<Napi::String>().Utf8Value();
    return jsonObj;
  }
  nlohmann::json jsonObj;
  Napi::Array propertyNames = obj.GetPropertyNames();
  for (uint32_t i = 0; i < propertyNames.Length(); i++) {
    Napi::String key = propertyNames.Get(i).As<Napi::String>();
    Napi::Value val = obj.Get(key);
    jsonObj[key.Utf8Value()] = convertValue2Json(env, val, isSyncCallback);
  }
  return jsonObj;
}
nlohmann::json convertValue2Json(Napi::Env &env, const Napi::Value &value) {
  return convertValue2Json(env, value, false);
}
nlohmann::json convertValue2Json(Napi::Env &env, const Napi::Value &value, bool isSyncCallback) {
  if (value.IsString()) {
    return value.As<Napi::String>().Utf8Value();
  } else if (value.IsNumber()) {
    return value.As<Napi::Number>().DoubleValue();
  } else if (value.IsBoolean()) {
    return value.As<Napi::Boolean>().Value();
  } else if (value.IsFunction()) {
    Napi::Function func = value.As<Napi::Function>();
    nlohmann::json jsonObj;
    // 生成callbackId，把Function和callbackId绑定在一起
    std::string callbackId = std::to_string(callbackUuid.nextid());
    jsonObj["callbackId"] = callbackId;
    jsonObj["syncCallback"] = isSyncCallback;

    callback[callbackId] = {
        std::make_shared<Napi::FunctionReference>(Napi::Persistent(func)),
        Napi::ThreadSafeFunction::New(env, func, "Callback", 0, 1)};
    return jsonObj;
  } else if (value.IsBuffer()) {
    Napi::Buffer<uint8_t> buffer = value.As<Napi::Buffer<uint8_t>>();
    size_t byteLength = buffer.Length();
    nlohmann::json jsonArr;
    for (uint32_t i = 0; i < byteLength; i++) {
      jsonArr[i] = buffer.Data()[i];
    }
    return jsonArr;
  } else if (value.IsArrayBuffer()) {
    Napi::ArrayBuffer arrayBuffer = value.As<Napi::ArrayBuffer>();
    size_t byteLength = arrayBuffer.ByteLength();
    nlohmann::json jsonArr;
    for (uint32_t i = 0; i < byteLength; i++) {
      jsonArr[i] = static_cast<uint8_t *>(arrayBuffer.Data())[i];
    }
    return jsonArr;
  } else if (value.IsArray()) {
    Napi::Array arr = value.As<Napi::Array>();
    nlohmann::json jsonArr = nlohmann::json::array();
    for (uint32_t i = 0; i < arr.Length(); i++) {
      jsonArr[i] = convertValue2Json(env, arr.Get(i), isSyncCallback);
    }
    return jsonArr;
  } else if (value.IsObject()) {
    return convertObject2Json(env, value, isSyncCallback);
  }
  return nlohmann::json();
}

Napi::Value convertJson2Value(Napi::Env &env, const nlohmann::json &data) {
  if (data.is_null()) {
    return env.Undefined();
  }
  if (data.is_string()) {
    return Napi::String::New(env, data.get<std::string>());
  } else if (data.is_number()) {
    return Napi::Number::New(env, data.get<double>());
  } else if (data.is_boolean()) {
    return Napi::Boolean::New(env, data.get<bool>());
  } else if (data.is_array()) {
    Napi::Array arr = Napi::Array::New(env, data.size());
    for (size_t i = 0; i < data.size(); i++) {
      arr[i] = convertJson2Value(env, data[i]);
    }
    return arr;
  } else if (data.is_object()) {

#ifdef _SKYLINE_CLIENT_
    if (data.contains("instanceId") && data.contains("instanceType") &&
        data["instanceType"].get<std::string>() == "function") {
      // 返回值是个函数，如makeShareable
      return Napi::Function::New(env, [data](const Napi::CallbackInfo &info) {
        auto env = info.Env();
        nlohmann::json args;
        for (int i = 0; i < info.Length(); i++) {
          args[i] = convertValue2Json(env, info[i]);
        }
        auto result = WebSocket::callStaticSync(
            "functionData", data["instanceId"].get<std::string>(), args);
        auto returnValue = result["returnValue"];

        return Convert::convertJson2Value(env, returnValue);
      });
    }
#endif

    if (data.contains("instanceId") && data.contains("instanceType")) {
      auto it = funcMap.find(data["instanceType"].get<std::string>());
      if (it != funcMap.end()) {
        try {
          Napi::FunctionReference *func = it->second;
          auto result = func->New(
              {Napi::String::New(env, data["instanceId"].get<std::string>())});
          return result;
        } catch (const std::exception &e) {
          Napi::Error::New(env,
                           std::string("Error creating instance: ") + e.what())
              .ThrowAsJavaScriptException();
          return env.Undefined();
        }
      }
      return env.Undefined();
    }
    Napi::Object obj = Napi::Object::New(env);
    for (auto it = data.begin(); it != data.end(); ++it) {
      obj.Set(it.key(), convertJson2Value(env, it.value()));
    }
    return obj;
  }
  // undefined
  return env.Undefined();
}
} // namespace Convert