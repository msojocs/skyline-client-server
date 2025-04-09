#include "../include/convert.hh"
#include <nlohmann/json_fwd.hpp>

namespace Convert {
nlohmann::json convertValue2Json(const Napi::Value &value) {
  if (value.IsString()) {
    return value.As<Napi::String>().Utf8Value();
  } else if (value.IsNumber()) {
    return value.As<Napi::Number>().DoubleValue();
  } else if (value.IsBoolean()) {
    return value.As<Napi::Boolean>().Value();
  } else if (value.IsObject()) {
    return convertObject2Json(value);
  } else if (value.IsArray()) {
    Napi::Array arr = value.As<Napi::Array>();
    nlohmann::json jsonArr;
    for (uint32_t i = 0; i < arr.Length(); i++) {
      jsonArr.push_back(convertValue2Json(arr.Get(i)));
    }
    return jsonArr;
  }
  return nullptr;
}
nlohmann::json convertObject2Json(const Napi::Value &value) {
  Napi::Object obj = value.As<Napi::Object>();
  nlohmann::json jsonObj;
  Napi::Array propertyNames = obj.GetPropertyNames();
  for (uint32_t i = 0; i < propertyNames.Length(); i++) {
    Napi::String key = propertyNames.Get(i).As<Napi::String>();
    Napi::Value val = obj.Get(key);
    jsonObj[key.Utf8Value()] = convertValue2Json(val);
  }
  return jsonObj;
}
} // namespace Convert