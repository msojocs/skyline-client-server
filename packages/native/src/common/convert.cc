#include "convert.hh"
#include "../client/skyline_global/async_style_sheets.hh"
#include "../client/skyline_global/fragment_binding.hh"
#include "../client/skyline_global/mutable_value.hh"
#include "../client/skyline_global/shadow_node/list_view.hh"
#include "../client/skyline_global/shadow_node/grid_view.hh"
#include "../client/skyline_global/shadow_node/scroll_view.hh"
#include "../client/skyline_global/shadow_node/sticky_header.hh"
#include "../client/skyline_global/shadow_node/sticky_section.hh"
#include "../client/skyline_global/shadow_node/text.hh"
#include "../client/skyline_global/shadow_node/image.hh"
#include "../client/skyline_global/shadow_node/swiper.hh"
#include "../client/skyline_global/shadow_node/swiper_item.hh"
#include "../client/skyline_global/shadow_node/view.hh"
#include "../client/skyline_global/shadow_node/hero.hh"
#include "../client/skyline_global/shadow_node/input.hh"
#include "../client/client_action.hh"
#include "napi.h"
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <unordered_map>

namespace Convert {
static int64_t callbackId = 1;
static std::unordered_map<int64_t, CallbackData> callback;
static std::unordered_map<int64_t, std::shared_ptr<Napi::ObjectReference>> instanceCache;

static std::unordered_map<std::string, Napi::FunctionReference *> clazzMap;

#ifdef _SKYLINE_CLIENT_

CallbackData * find_callback(int64_t callbackId)
{
  auto it = callback.find(callbackId);
  if (it != callback.end()) {
    return &it->second;
  }
  return nullptr;
}
void RegisteInstanceType(Napi::Env &env) {
  // 注册实例类型和对应的构造函数
  clazzMap["AsyncStylesheets"] = Skyline::AsyncStyleSheets::GetClazz(env);
  clazzMap["TextShadowNode"] = Skyline::TextShadowNode::GetClazz(env);
  clazzMap["InputShadowNode"] = Skyline::InputShadowNode::GetClazz(env);
  clazzMap["ImageShadowNode"] = Skyline::ImageShadowNode::GetClazz(env);
  clazzMap["SwiperShadowNode"] = Skyline::SwiperShadowNode::GetClazz(env);
  // SwiperItemShadoNode就是少了一个w
  clazzMap["SwiperItemShadoNode"] = Skyline::SwiperItemShadowNode::GetClazz(env);
  clazzMap["ViewShadowNode"] = Skyline::ViewShadowNode::GetClazz(env);
  clazzMap["GridViewShadowNode"] = Skyline::GridViewShadowNode::GetClazz(env);
  clazzMap["ScrollViewShadowNode"] = Skyline::ScrollViewShadowNode::GetClazz(env);
  clazzMap["ListViewShadowNode"] = Skyline::ListViewShadowNode::GetClazz(env);
  clazzMap["StickySectionShadowNode"] = Skyline::StickySectionShadowNode::GetClazz(env);
  clazzMap["StickyHeaderShadowNode"] = Skyline::StickyHeaderShadowNode::GetClazz(env);
  clazzMap["FragmentBinding"] = Skyline::FragmentBinding::GetClazz(env);
  clazzMap["MutableValue"] = Skyline::MutableValue::GetClazz(env);
  clazzMap["HeroShadowNode"] = Skyline::HeroShadowNode::GetClazz(env);
}
#endif

nlohmann::json convertObject2Json(Napi::Env &env, const Napi::Value &value) {
  Napi::Object obj = value.As<Napi::Object>();
  if (obj.Get("instanceId").IsNumber()) {
    nlohmann::json jsonObj;
    jsonObj["instanceId"] =
        obj.Get("instanceId").As<Napi::Number>().Int64Value();
    return jsonObj;
  }
  nlohmann::json jsonObj = nlohmann::json::object();
  Napi::Array propertyNames = obj.GetPropertyNames();
  for (uint32_t i = 0; i < propertyNames.Length(); i++) {
    Napi::String key = propertyNames.Get(i).As<Napi::String>();
    Napi::Value val = obj.Get(key);
    auto k = key.Utf8Value();
    if (k.length() == 0) {
      continue;
    }
    jsonObj[k] = convertValue2Json(env, val);
  }
  return jsonObj;
}
nlohmann::json convertValue2Json(Napi::Env &env, const Napi::Value &value) {
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
    if (callbackId >= INT64_MAX) {
      callbackId = 1;
    }
    auto cId = callbackId++;
    bool isSync = func.Get(Napi::String::New(env, "__syncCallback")).IsBoolean();
    if (isSync) {
      isSync = func.Get(Napi::String::New(env, "__syncCallback")).As<Napi::Boolean>().Value();
    }
    isSync = true;
    jsonObj["callbackId"] = cId;
    jsonObj["syncCallback"] = isSync;

    callback[cId] = {
        std::make_shared<Napi::FunctionReference>(Napi::Persistent(func)),
        Napi::ThreadSafeFunction::New(env, func, "Callback", 0, 1)};
    if (func.Get("__worklet").IsBoolean()) {
      jsonObj["asString"] = func.Get("asString").As<Napi::String>().Utf8Value();
      jsonObj["__workletHash"] = func.Get("__workletHash").As<Napi::Number>().Int64Value();
      jsonObj["__location"] = func.Get("__location").As<Napi::String>().Utf8Value();
      jsonObj["__worklet"] = func.Get("__worklet").As<Napi::Boolean>().Value();
      jsonObj["_closure"] = convertValue2Json(env, func.Get("_closure"));
    }
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
      jsonArr[i] = convertValue2Json(env, arr.Get(i));
    }
    return jsonArr;
  } else if (value.IsObject()) {
    return convertObject2Json(env, value);
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
        nlohmann::json args = nlohmann::json::array();
        for (int i = 0; i < info.Length(); i++) {
          args[i] = convertValue2Json(env, info[i]);
        }
        auto result = ClientAction::callStaticSync("functionData", std::to_string(data["instanceId"].get<int64_t>()), args);
        auto returnValue = result["returnValue"];

        return Convert::convertJson2Value(env, returnValue);
      });
    }
#endif

    if (data.contains("instanceId") && data.contains("instanceType")) {
      auto it = clazzMap.find(data["instanceType"].get<std::string>());
      if (it != clazzMap.end()) {
        try {
          Napi::FunctionReference *func = it->second;
          // 创建实例
          // 先到cache找
          auto instanceId = data["instanceId"].get<int64_t>();
          if (auto target = instanceCache.find(instanceId); target != instanceCache.end()) {
            return target->second->Value();
          }
          // cache找不到
          auto result = func->New({Napi::Number::New(env, data["instanceId"].get<int64_t>())});
          auto ref = Napi::Persistent(result);
          instanceCache.emplace(instanceId, std::make_shared<Napi::ObjectReference>(std::move(ref)) );
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