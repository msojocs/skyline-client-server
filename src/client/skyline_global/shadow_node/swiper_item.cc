#include "swiper_item.hh"
#include "napi.h"
#include "node.hh"

namespace Skyline {
Napi::FunctionReference *SwiperItemShadowNode::GetClazz(Napi::Env env) {

  Napi::Function func = DefineClass(
      env, "SwiperItemShadowNode",
      {
          InstanceMethod("setStyleScope", &SwiperItemShadowNode::setStyleScope),
          InstanceMethod("addClass", &SwiperItemShadowNode::addClass),
          InstanceMethod("setStyle", &SwiperItemShadowNode::setStyle),
          InstanceMethod("setEventDefaultPrevented",
                         &SwiperItemShadowNode::setEventDefaultPrevented),
          InstanceMethod("appendChild", &SwiperItemShadowNode::appendChild),
          InstanceMethod("spliceAppend", &SwiperItemShadowNode::spliceAppend),
          InstanceMethod("setId", &SwiperItemShadowNode::setId),
          InstanceMethod("forceDetached", &SwiperItemShadowNode::forceDetached),
          InstanceMethod("spliceRemove", &SwiperItemShadowNode::spliceRemove),
          InstanceMethod("release", &SwiperItemShadowNode::release),
          
          InstanceMethod("setAttributes", &SwiperItemShadowNode::setAttributes),
          InstanceMethod("getBoundingClientRect", &SwiperItemShadowNode::getBoundingClientRect),
          
          InstanceMethod("setClass", &SwiperItemShadowNode::setClass),
          InstanceMethod("matches", &SwiperItemShadowNode::matches),
          // removeChild
          InstanceMethod("removeChild", &SwiperItemShadowNode::removeChild),
          // setLayoutCallback
          InstanceMethod("setLayoutCallback", &SwiperItemShadowNode::setLayoutCallback),
          // setTouchEventNeedsLocalCoords
          InstanceMethod("setTouchEventNeedsLocalCoords", &SwiperItemShadowNode::setTouchEventNeedsLocalCoords),
          // setAttribute
          InstanceMethod("setAttribute", &SwiperItemShadowNode::setAttribute),
          // getter isConnected
          InstanceAccessor("isConnected", &SwiperItemShadowNode::isConnected, nullptr,
                           static_cast<napi_property_attributes>(
                               napi_writable | napi_configurable)),
          // getter instanceId
          InstanceAccessor("instanceId", &SwiperItemShadowNode::getInstanceId, nullptr,
                         static_cast<napi_property_attributes>(
                             napi_writable | napi_configurable)),
      });
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
SwiperItemShadowNode::SwiperItemShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<SwiperItemShadowNode>(info), ShadowNode(info) {
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