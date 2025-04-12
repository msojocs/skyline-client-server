#include "scroll_view.hh"
#include "../../../include/convert.hh"
#include "../../websocket.hh"

namespace Skyline {
Napi::FunctionReference *ScrollViewShadowNode::GetClazz(Napi::Env env) {
  Napi::Function func = DefineClass(
      env, "ScrollViewShadowNode",
      {InstanceMethod("setStyleScope", &ScrollViewShadowNode::setStyleScope),
       InstanceMethod("addClass", &ScrollViewShadowNode::addClass),
       InstanceMethod("setStyle", &ScrollViewShadowNode::setStyle),
       InstanceMethod("setEventDefaultPrevented",
                      &ScrollViewShadowNode::setEventDefaultPrevented),
       InstanceMethod("appendChild", &ScrollViewShadowNode::appendChild),
       InstanceMethod("spliceAppend", &ScrollViewShadowNode::spliceAppend),

       InstanceMethod("setId", &ScrollViewShadowNode::setId),
       InstanceMethod("forceDetached", &ScrollViewShadowNode::forceDetached),
       InstanceMethod("spliceRemove", &ScrollViewShadowNode::spliceRemove),
       InstanceMethod("release", &ScrollViewShadowNode::release),

       InstanceMethod("setAttributes", &ScrollViewShadowNode::setAttributes),
       InstanceMethod("getBoundingClientRect",
                      &ScrollViewShadowNode::getBoundingClientRect),
        InstanceMethod("setRefresherHeader", &ScrollViewShadowNode::setRefresherHeader),
       // getter instanceId
       InstanceAccessor("instanceId", &ScrollViewShadowNode::getInstanceId, nullptr,
                        static_cast<napi_property_attributes>(
                            napi_writable | napi_configurable)),
      });
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
ScrollViewShadowNode::ScrollViewShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<ScrollViewShadowNode>(info), ShadowNode(info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "Constructor: Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(), "Constructor: First argument must be a string");
  }
  m_instanceId = info[0].As<Napi::String>().Utf8Value();
}
Napi::Value ScrollViewShadowNode::setRefresherHeader(const Napi::CallbackInfo &info) {
  
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  auto env = info.Env();
  return Convert::convertJson2Value(env, returnValue);
}
}