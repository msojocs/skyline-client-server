#include "text.hh"
#include "napi.h"

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
} // namespace Skyline