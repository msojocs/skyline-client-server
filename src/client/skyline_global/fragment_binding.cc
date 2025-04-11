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
          InstanceMethod("associateComponent", &FragmentBinding::appendChild),
          InstanceMethod("equal", &FragmentBinding::appendChild),
          InstanceMethod("findChildPosition", &FragmentBinding::appendChild),
          InstanceMethod("getChildNode", &FragmentBinding::appendChild),
          InstanceMethod("getParentNode", &FragmentBinding::appendChild),
          InstanceMethod("insertChild", &FragmentBinding::appendChild),
          InstanceMethod("release", &FragmentBinding::appendChild),
          InstanceMethod("removeChild", &FragmentBinding::appendChild),
          InstanceMethod("replaceChild", &FragmentBinding::appendChild),
          InstanceMethod("splice", &FragmentBinding::appendChild),
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
} // namespace Skyline