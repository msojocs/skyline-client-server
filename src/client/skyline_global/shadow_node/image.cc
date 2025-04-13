#include "image.hh"
#include "napi.h"

namespace Skyline {
Napi::FunctionReference *ImageShadowNode::GetClazz(Napi::Env env) {

  Napi::Function func = DefineClass(
      env, "ImageShadowNode",
      {
          InstanceMethod("setStyleScope", &ImageShadowNode::setStyleScope),
          InstanceMethod("addClass", &ImageShadowNode::addClass),
          InstanceMethod("setStyle", &ImageShadowNode::setStyle),
          InstanceMethod("setEventDefaultPrevented",
                         &ImageShadowNode::setEventDefaultPrevented),
          InstanceMethod("appendChild", &ImageShadowNode::appendChild),
          InstanceMethod("spliceAppend", &ImageShadowNode::spliceAppend),
          
          InstanceMethod("setId", &ImageShadowNode::setId),
          InstanceMethod("forceDetached", &ImageShadowNode::forceDetached),
          InstanceMethod("spliceRemove", &ImageShadowNode::spliceRemove),
          InstanceMethod("release", &ImageShadowNode::release),
          
          InstanceMethod("setAttributes", &ImageShadowNode::setAttributes),
          InstanceMethod("getBoundingClientRect", &ImageShadowNode::getBoundingClientRect),
          
          InstanceMethod("setClass", &ImageShadowNode::setClass),
          InstanceMethod("matches", &ImageShadowNode::matches),
          // removeChild
          InstanceMethod("removeChild", &ImageShadowNode::removeChild),
          // setLayoutCallback
          InstanceMethod("setLayoutCallback", &ImageShadowNode::setLayoutCallback),
          //setTouchEventNeedsLocalCoords
          InstanceMethod("setTouchEventNeedsLocalCoords", &ImageShadowNode::setTouchEventNeedsLocalCoords),
          // setAttribute
          InstanceMethod("setAttribute", &ImageShadowNode::setAttribute),
          // getter isConnected
          InstanceAccessor("isConnected", &ImageShadowNode::isConnected, nullptr,
                           static_cast<napi_property_attributes>(
                               napi_writable | napi_configurable)),
          // getter instanceId
          InstanceAccessor("instanceId", &ImageShadowNode::getInstanceId, nullptr,
                         static_cast<napi_property_attributes>(
                             napi_writable | napi_configurable)),
      });
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
ImageShadowNode::ImageShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<ImageShadowNode>(info), ShadowNode(info) {
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