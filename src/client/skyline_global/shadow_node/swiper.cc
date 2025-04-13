#include "swiper.hh"
#include "napi.h"

namespace Skyline {
Napi::FunctionReference *SwiperShadowNode::GetClazz(Napi::Env env) {

  Napi::Function func = DefineClass(
      env, "SwiperShadowNode",
      {
          InstanceMethod("setStyleScope", &SwiperShadowNode::setStyleScope),
          InstanceMethod("addClass", &SwiperShadowNode::addClass),
          InstanceMethod("setStyle", &SwiperShadowNode::setStyle),
          InstanceMethod("setEventDefaultPrevented",
                         &SwiperShadowNode::setEventDefaultPrevented),
          InstanceMethod("appendChild", &SwiperShadowNode::appendChild),
          InstanceMethod("spliceAppend", &SwiperShadowNode::spliceAppend),
          
          InstanceMethod("setId", &SwiperShadowNode::setId),
          InstanceMethod("forceDetached", &SwiperShadowNode::forceDetached),
          InstanceMethod("spliceRemove", &SwiperShadowNode::spliceRemove),
          InstanceMethod("release", &SwiperShadowNode::release),
          
          InstanceMethod("setAttributes", &SwiperShadowNode::setAttributes),
          InstanceMethod("getBoundingClientRect", &SwiperShadowNode::getBoundingClientRect),
          
          InstanceMethod("setClass", &SwiperShadowNode::setClass),
          InstanceMethod("matches", &SwiperShadowNode::matches),
          // removeChild
          InstanceMethod("removeChild", &SwiperShadowNode::removeChild),
          // setLayoutCallback
          InstanceMethod("setLayoutCallback", &SwiperShadowNode::setLayoutCallback),
          //setTouchEventNeedsLocalCoords
          InstanceMethod("setTouchEventNeedsLocalCoords", &SwiperShadowNode::setTouchEventNeedsLocalCoords),
          // setAttribute
          InstanceMethod("setAttribute", &SwiperShadowNode::setAttribute),
          // getter isConnected
          InstanceAccessor("isConnected", &SwiperShadowNode::isConnected, nullptr,
                           static_cast<napi_property_attributes>(
                               napi_writable | napi_configurable)),
          // getter instanceId
          InstanceAccessor("instanceId", &SwiperShadowNode::getInstanceId, nullptr,
                         static_cast<napi_property_attributes>(
                             napi_writable | napi_configurable)),
      });
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
SwiperShadowNode::SwiperShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<SwiperShadowNode>(info), ShadowNode(info) {
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