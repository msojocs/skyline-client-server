#include "list_view.hh"
#include "napi.h"
#include "node.hh"

namespace Skyline {
Napi::FunctionReference *ListViewShadowNode::GetClazz(Napi::Env env) {

  Napi::Function func = DefineClass(
      env, "ListViewShadowNode",
      {
          InstanceMethod("setStyleScope", &ListViewShadowNode::setStyleScope),
          InstanceMethod("addClass", &ListViewShadowNode::addClass),
          InstanceMethod("setStyle", &ListViewShadowNode::setStyle),
          InstanceMethod("setEventDefaultPrevented",
                         &ListViewShadowNode::setEventDefaultPrevented),
          InstanceMethod("appendChild", &ListViewShadowNode::appendChild),
          InstanceMethod("spliceAppend", &ListViewShadowNode::spliceAppend),
          InstanceMethod("setId", &ListViewShadowNode::setId),
          InstanceMethod("forceDetached", &ListViewShadowNode::forceDetached),
          InstanceMethod("spliceRemove", &ListViewShadowNode::spliceRemove),
          InstanceMethod("release", &ListViewShadowNode::release),
          
          InstanceMethod("setAttributes", &ListViewShadowNode::setAttributes),
          InstanceMethod("getBoundingClientRect", &ListViewShadowNode::getBoundingClientRect),
          // setClass 
          
          InstanceMethod("setClass", &ListViewShadowNode::setClass),
          InstanceMethod("matches", &ListViewShadowNode::matches),
          // removeChild
          InstanceMethod("removeChild", &ListViewShadowNode::removeChild),
          // setLayoutCallback
          InstanceMethod("setLayoutCallback", &ListViewShadowNode::setLayoutCallback),
          // setTouchEventNeedsLocalCoords
          InstanceMethod("setTouchEventNeedsLocalCoords", &ListViewShadowNode::setTouchEventNeedsLocalCoords),
          
          // getter isConnected
          InstanceAccessor("isConnected", &ListViewShadowNode::isConnected, nullptr,
                           static_cast<napi_property_attributes>(
                               napi_writable | napi_configurable)),
          // getter instanceId
          InstanceAccessor("instanceId", &ListViewShadowNode::getInstanceId, nullptr,
                         static_cast<napi_property_attributes>(
                             napi_writable | napi_configurable)),
      });
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
ListViewShadowNode::ListViewShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<ListViewShadowNode>(info), ShadowNode(info) {
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
