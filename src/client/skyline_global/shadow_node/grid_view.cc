#include "grid_view.hh"
#include "napi.h"
#include "node.hh"

namespace Skyline {
Napi::FunctionReference *GridViewShadowNode::GetClazz(Napi::Env env) {

  Napi::Function func = DefineClass(
      env, "GridViewShadowNode",
      {
          InstanceMethod("setStyleScope", &GridViewShadowNode::setStyleScope),
          InstanceMethod("addClass", &GridViewShadowNode::addClass),
          InstanceMethod("setStyle", &GridViewShadowNode::setStyle),
          InstanceMethod("setEventDefaultPrevented",
                         &GridViewShadowNode::setEventDefaultPrevented),
          InstanceMethod("appendChild", &GridViewShadowNode::appendChild),
          InstanceMethod("spliceAppend", &GridViewShadowNode::spliceAppend),
          InstanceMethod("setId", &GridViewShadowNode::setId),
          InstanceMethod("forceDetached", &GridViewShadowNode::forceDetached),
          InstanceMethod("spliceRemove", &GridViewShadowNode::spliceRemove),
          InstanceMethod("release", &GridViewShadowNode::release),
          
          InstanceMethod("setAttributes", &GridViewShadowNode::setAttributes),
          InstanceMethod("getBoundingClientRect", &GridViewShadowNode::getBoundingClientRect),
          
          InstanceMethod("setClass", &GridViewShadowNode::setClass),
          InstanceMethod("matches", &GridViewShadowNode::matches),
          // removeChild
          InstanceMethod("removeChild", &GridViewShadowNode::removeChild),
          // setLayoutCallback
          InstanceMethod("setLayoutCallback", &GridViewShadowNode::setLayoutCallback),
          // setTouchEventNeedsLocalCoords
          InstanceMethod("setTouchEventNeedsLocalCoords", &GridViewShadowNode::setTouchEventNeedsLocalCoords),
          // setAttribute
          InstanceMethod("setAttribute", &GridViewShadowNode::setAttribute),
          // getter isConnected
          InstanceAccessor("isConnected", &GridViewShadowNode::isConnected, nullptr,
                           static_cast<napi_property_attributes>(
                               napi_writable | napi_configurable)),
          // getter instanceId
          InstanceAccessor("instanceId", &GridViewShadowNode::getInstanceId, nullptr,
                         static_cast<napi_property_attributes>(
                             napi_writable | napi_configurable)),
      });
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
GridViewShadowNode::GridViewShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<GridViewShadowNode>(info), ShadowNode(info) {
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