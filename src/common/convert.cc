#include "../include/convert.hh"
#include "../client/include/async_style_sheets.hh"
#include "../client/include/view_shadow_node.hh"
#include "../client/include/fragment_binding.hh"
#include "napi.h"
#include <nlohmann/json_fwd.hpp>

namespace Convert {
static std::map<std::string, Napi::FunctionReference *> funcMap;

#ifdef _SKYLINE_CLIENT_
void RegisteInstanceType(Napi::Env &env) {
  // 注册实例类型和对应的构造函数
  funcMap["AsyncStylesheets"] = Skyline::AsyncStyleSheets::GetClazz(env);
  funcMap["ViewShadowNode"] = Skyline::ViewShadowNode::GetClazz(env);
  funcMap["FragmentBinding"] = Skyline::FragmentBinding::GetClazz(env);
}
#endif

nlohmann::json convertValue2Json(const Napi::Value &value) {
  if (value.IsString()) {
    return value.As<Napi::String>().Utf8Value();
  } else if (value.IsNumber()) {
    return value.As<Napi::Number>().DoubleValue();
  } else if (value.IsBoolean()) {
    return value.As<Napi::Boolean>().Value();
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
    nlohmann::json jsonArr;
    for (uint32_t i = 0; i < arr.Length(); i++) {
      jsonArr[i] = convertValue2Json(arr.Get(i));
    }
    return jsonArr;
  } else if (value.IsObject()) {
    return convertObject2Json(value);
  }
  return nlohmann::json();
}
nlohmann::json convertObject2Json(const Napi::Value &value) {
  Napi::Object obj = value.As<Napi::Object>();
  if (obj.Get("instanceId").IsString()) {
    nlohmann::json jsonObj;
    jsonObj["instanceId"] = obj.Get("instanceId").As<Napi::String>().Utf8Value();
    return jsonObj;
  }
  nlohmann::json jsonObj;
  Napi::Array propertyNames = obj.GetPropertyNames();
  for (uint32_t i = 0; i < propertyNames.Length(); i++) {
    Napi::String key = propertyNames.Get(i).As<Napi::String>();
    Napi::Value val = obj.Get(key);
    jsonObj[key.Utf8Value()] = convertValue2Json(val);
  }
  return jsonObj;
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
    if (data.contains("instanceId") && data.contains("instanceType")) {
      auto it = funcMap.find(data["instanceType"].get<std::string>());
      if (it != funcMap.end()) {
        try {
          Napi::FunctionReference *func = it->second;
          auto result = func->New({Napi::String::New(env, data["instanceId"].get<std::string>())});
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