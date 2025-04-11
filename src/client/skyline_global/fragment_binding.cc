#include "../include/fragment_binding.hh"
#include "../../include/convert.hh"
#include "../websocket.hh"


namespace Skyline {

Napi::FunctionReference *FragmentBinding::GetClazz(Napi::Env env) {
  Napi::Function func = DefineClass(
      env, "FragmentBinding",
      {
        InstanceAccessor<&FragmentBinding::getInstanceId>("instanceId",
                         static_cast<napi_property_attributes>(napi_writable |
                                                              napi_configurable)),
          InstanceMethod("appendChild", &FragmentBinding::appendChild),
          InstanceMethod("associateComponent", &FragmentBinding::associateComponent),
          InstanceMethod("equal", &FragmentBinding::equal),
          InstanceMethod("findChildPosition", &FragmentBinding::findChildPosition),
          InstanceMethod("getChildNode", &FragmentBinding::getChildNode),
          InstanceMethod("getParentNode", &FragmentBinding::getParentNode),
          InstanceMethod("insertChild", &FragmentBinding::insertChild),
          InstanceMethod("release", &FragmentBinding::release),
          InstanceMethod("removeChild", &FragmentBinding::removeChild),
          InstanceMethod("replaceChild", &FragmentBinding::replaceChild),
          InstanceMethod("splice", &FragmentBinding::splice),
      });
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
FragmentBinding::FragmentBinding(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<FragmentBinding>(info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(),
                               "FragmentBinding: Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(
        info.Env(), "FragmentBinding: First argument must be a string");
  }
  m_instanceId = info[0].As<Napi::String>().Utf8Value();
}
Napi::Value FragmentBinding::getInstanceId(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), m_instanceId);
}
Napi::Value FragmentBinding::appendChild(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env =  info.Env();
  return Convert::convertJson2Value(env, result["returnValue"]);
}

Napi::Value FragmentBinding::associateComponent(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env =  info.Env();
  return Convert::convertJson2Value(env, result["returnValue"]);
}
Napi::Value FragmentBinding::equal(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env =  info.Env();
  return Convert::convertJson2Value(env, result["returnValue"]);
}
Napi::Value FragmentBinding::findChildPosition(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env =  info.Env();
  return Convert::convertJson2Value(env, result["returnValue"]);
}
Napi::Value FragmentBinding::getChildNode(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env =  info.Env();
  return Convert::convertJson2Value(env, result["returnValue"]);

}
Napi::Value FragmentBinding::getParentNode(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env =  info.Env();
  return Convert::convertJson2Value(env, result["returnValue"]);
}
Napi::Value FragmentBinding::insertChild(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env =  info.Env();
  return Convert::convertJson2Value(env, result["returnValue"]);
}
Napi::Value FragmentBinding::release(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env =  info.Env();
  return Convert::convertJson2Value(env, result["returnValue"]);
}
Napi::Value FragmentBinding::removeChild(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env =  info.Env();
  return Convert::convertJson2Value(env, result["returnValue"]);
}
Napi::Value FragmentBinding::replaceChild(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env =  info.Env();
  return Convert::convertJson2Value(env, result["returnValue"]);
}
Napi::Value FragmentBinding::splice(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto env =  info.Env();
  return Convert::convertJson2Value(env, result["returnValue"]);
}
} // namespace Skyline