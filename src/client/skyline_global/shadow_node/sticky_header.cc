#include "sticky_header.hh"

namespace Skyline {
Napi::FunctionReference *StickyHeaderShadowNode::GetClazz(Napi::Env env) {
  Napi::Function func = DefineClass(
      env, "StickyHeaderShadowNode",
      {
          InstanceMethod("setStyleScope",
                         &StickyHeaderShadowNode::setStyleScope),
          InstanceMethod("addClass", &StickyHeaderShadowNode::addClass),
          InstanceMethod("setStyle", &StickyHeaderShadowNode::setStyle),
          InstanceMethod("setEventDefaultPrevented",
                         &StickyHeaderShadowNode::setEventDefaultPrevented),
          InstanceMethod("appendChild", &StickyHeaderShadowNode::appendChild),
          InstanceMethod("spliceAppend", &StickyHeaderShadowNode::spliceAppend),

          InstanceMethod("setId", &StickyHeaderShadowNode::setId),
          InstanceMethod("forceDetached",
                         &StickyHeaderShadowNode::forceDetached),
          InstanceMethod("spliceRemove", &StickyHeaderShadowNode::spliceRemove),
          InstanceMethod("release", &StickyHeaderShadowNode::release),

          InstanceMethod("setAttributes",
                         &StickyHeaderShadowNode::setAttributes),
          InstanceMethod("getBoundingClientRect",
                         &StickyHeaderShadowNode::getBoundingClientRect),
          InstanceMethod("setClass", &StickyHeaderShadowNode::setClass),

          InstanceMethod("matches", &StickyHeaderShadowNode::matches),
          // removeChild
          InstanceMethod("removeChild", &StickyHeaderShadowNode::removeChild),
          // setLayoutCallback
          InstanceMethod("setLayoutCallback",
                         &StickyHeaderShadowNode::setLayoutCallback),
          InstanceMethod("setTouchEventNeedsLocalCoords",
                         &StickyHeaderShadowNode::setTouchEventNeedsLocalCoords),
          // setAttribute
          InstanceMethod("setAttribute",
                         &StickyHeaderShadowNode::setAttribute),
          // getter isConnected
          InstanceAccessor("isConnected", &StickyHeaderShadowNode::isConnected,
                           nullptr,
                           static_cast<napi_property_attributes>(
                               napi_writable | napi_configurable)),
          // getter instanceId
          InstanceAccessor("instanceId", &StickyHeaderShadowNode::getInstanceId,
                           nullptr,
                           static_cast<napi_property_attributes>(
                               napi_writable | napi_configurable)),
      });
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
StickyHeaderShadowNode::StickyHeaderShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<StickyHeaderShadowNode>(info), ShadowNode(info) {
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