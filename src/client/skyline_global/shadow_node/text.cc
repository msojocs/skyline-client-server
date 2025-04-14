#include "text.hh"
#include "napi.h"
#include "../../websocket.hh"
#include "../../../include/convert.hh"

namespace Skyline {
Napi::FunctionReference *TextShadowNode::GetClazz(Napi::Env env) {

  Napi::Function func = DefineClass(
      env, "TextShadowNode",
      {
          InstanceMethod("setStyleScope", &TextShadowNode::setStyleScope),
          InstanceMethod("addClass", &TextShadowNode::addClass),
          InstanceMethod("setStyle", &TextShadowNode::setStyle),
          InstanceMethod("setEventDefaultPrevented",
                         &TextShadowNode::setEventDefaultPrevented),
          InstanceMethod("appendChild", &TextShadowNode::appendChild),
          InstanceMethod("spliceAppend", &TextShadowNode::spliceAppend),
          
          InstanceMethod("setId", &TextShadowNode::setId),
          InstanceMethod("forceDetached", &TextShadowNode::forceDetached),
          InstanceMethod("spliceRemove", &TextShadowNode::spliceRemove),
          InstanceMethod("release", &TextShadowNode::release),
          
          InstanceMethod("setAttributes", &TextShadowNode::setAttributes),
          InstanceMethod("getBoundingClientRect", &TextShadowNode::getBoundingClientRect),
          
          InstanceMethod("setClass", &TextShadowNode::setClass),
          InstanceMethod("matches", &TextShadowNode::matches),
          // removeChild
          InstanceMethod("removeChild", &TextShadowNode::removeChild),
          // setLayoutCallback
          InstanceMethod("setLayoutCallback", &TextShadowNode::setLayoutCallback),
          //setTouchEventNeedsLocalCoords
          InstanceMethod("setTouchEventNeedsLocalCoords", &TextShadowNode::setTouchEventNeedsLocalCoords),
          // setAttribute
          InstanceMethod("setAttribute", &TextShadowNode::setAttribute),
          // setText
          InstanceMethod("setText", &TextShadowNode::setText),
          // getter isConnected
          InstanceAccessor("isConnected", &TextShadowNode::isConnected, nullptr,
                           static_cast<napi_property_attributes>(
                               napi_writable | napi_configurable)),
          // getter instanceId
          InstanceAccessor("instanceId", &TextShadowNode::getInstanceId, nullptr,
                         static_cast<napi_property_attributes>(
                             napi_writable | napi_configurable)),
      });
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
TextShadowNode::TextShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<TextShadowNode>(info), ShadowNode(info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: First argument must be a string");
  }
  m_instanceId = info[0].As<Napi::String>().Utf8Value();
}
Napi::Value TextShadowNode::setText(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  auto env = info.Env();
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, "setText", args);
  return Convert::convertJson2Value(env, result["returnValue"]);
}
} // namespace Skyline