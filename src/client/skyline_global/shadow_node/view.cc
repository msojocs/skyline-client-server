#include "view.hh"
#include "napi.h"
#include "node.hh"

namespace Skyline {
Napi::FunctionReference *ViewShadowNode::GetClazz(Napi::Env env) {

  Napi::Function func = DefineClass(
      env, "ViewShadowNode",
      {
          InstanceMethod("setStyleScope", &ViewShadowNode::setStyleScope),
          InstanceMethod("addClass", &ViewShadowNode::addClass),
          InstanceMethod("setStyle", &ViewShadowNode::setStyle),
          InstanceMethod("setEventDefaultPrevented",
                         &ViewShadowNode::setEventDefaultPrevented),
          InstanceMethod("appendChild", &ViewShadowNode::appendChild),
          InstanceMethod("spliceAppend", &ViewShadowNode::spliceAppend),
          InstanceMethod("setId", &ViewShadowNode::setId),
          InstanceMethod("forceDetached", &ViewShadowNode::forceDetached),
          InstanceMethod("spliceRemove", &ViewShadowNode::spliceRemove),
          InstanceMethod("release", &ViewShadowNode::release),
          
          InstanceMethod("setAttributes", &ViewShadowNode::setAttributes),
          InstanceMethod("getBoundingClientRect", &ViewShadowNode::getBoundingClientRect),
          
          InstanceMethod("setClass", &ViewShadowNode::setClass),
          InstanceMethod("matches", &ViewShadowNode::matches),
          // removeChild
          InstanceMethod("removeChild", &ViewShadowNode::removeChild),
          // setLayoutCallback
          InstanceMethod("setLayoutCallback", &ViewShadowNode::setLayoutCallback),
          // setTouchEventNeedsLocalCoords
          InstanceMethod("setTouchEventNeedsLocalCoords", &ViewShadowNode::setTouchEventNeedsLocalCoords),
          // getter isConnected
          InstanceAccessor("isConnected", &ViewShadowNode::isConnected, nullptr,
                           static_cast<napi_property_attributes>(
                               napi_writable | napi_configurable)),
          // getter instanceId
          InstanceAccessor("instanceId", &ViewShadowNode::getInstanceId, nullptr,
                         static_cast<napi_property_attributes>(
                             napi_writable | napi_configurable)),
      });
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
ViewShadowNode::ViewShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<ViewShadowNode>(info), ShadowNode(info) {
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