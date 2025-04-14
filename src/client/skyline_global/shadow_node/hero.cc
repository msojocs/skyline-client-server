#include "hero.hh"
#include "napi.h"
#include "node.hh"

namespace Skyline {
Napi::FunctionReference *HeroShadowNode::GetClazz(Napi::Env env) {

  Napi::Function func = DefineClass(
      env, "HeroShadowNode",
      {
          InstanceMethod("setStyleScope", &HeroShadowNode::setStyleScope),
          InstanceMethod("addClass", &HeroShadowNode::addClass),
          InstanceMethod("setStyle", &HeroShadowNode::setStyle),
          InstanceMethod("setEventDefaultPrevented",
                         &HeroShadowNode::setEventDefaultPrevented),
          InstanceMethod("appendChild", &HeroShadowNode::appendChild),
          InstanceMethod("spliceAppend", &HeroShadowNode::spliceAppend),
          InstanceMethod("setId", &HeroShadowNode::setId),
          InstanceMethod("forceDetached", &HeroShadowNode::forceDetached),
          InstanceMethod("spliceRemove", &HeroShadowNode::spliceRemove),
          InstanceMethod("release", &HeroShadowNode::release),
          
          InstanceMethod("setAttributes", &HeroShadowNode::setAttributes),
          InstanceMethod("getBoundingClientRect", &HeroShadowNode::getBoundingClientRect),
          
          InstanceMethod("setClass", &HeroShadowNode::setClass),
          InstanceMethod("matches", &HeroShadowNode::matches),
          // removeChild
          InstanceMethod("removeChild", &HeroShadowNode::removeChild),
          // setLayoutCallback
          InstanceMethod("setLayoutCallback", &HeroShadowNode::setLayoutCallback),
          // setTouchEventNeedsLocalCoords
          InstanceMethod("setTouchEventNeedsLocalCoords", &HeroShadowNode::setTouchEventNeedsLocalCoords),
          // setAttribute
          InstanceMethod("setAttribute", &HeroShadowNode::setAttribute),
          // getter isConnected
          InstanceAccessor("isConnected", &HeroShadowNode::isConnected, nullptr,
                           static_cast<napi_property_attributes>(
                               napi_writable | napi_configurable)),
          // getter instanceId
          InstanceAccessor("instanceId", &HeroShadowNode::getInstanceId, nullptr,
                         static_cast<napi_property_attributes>(
                             napi_writable | napi_configurable)),
      });
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
HeroShadowNode::HeroShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<HeroShadowNode>(info), ShadowNode(info) {
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