#include "sticky_section.hh"

namespace Skyline {
  Napi::FunctionReference *StickySectionShadowNode::GetClazz(Napi::Env env) {
    Napi::Function func = DefineClass(
        env, "StickySectionShadowNode",
        {InstanceMethod("setStyleScope", &StickySectionShadowNode::setStyleScope),
         InstanceMethod("addClass", &StickySectionShadowNode::addClass),
         InstanceMethod("setStyle", &StickySectionShadowNode::setStyle),
         InstanceMethod("setEventDefaultPrevented",
                        &StickySectionShadowNode::setEventDefaultPrevented),
         InstanceMethod("appendChild", &StickySectionShadowNode::appendChild),
         InstanceMethod("spliceAppend", &StickySectionShadowNode::spliceAppend),

         InstanceMethod("setId", &StickySectionShadowNode::setId),
         InstanceMethod("forceDetached", &StickySectionShadowNode::forceDetached),
         InstanceMethod("spliceRemove", &StickySectionShadowNode::spliceRemove),
         InstanceMethod("release", &StickySectionShadowNode::release),

         InstanceMethod("setAttributes", &StickySectionShadowNode::setAttributes),
         InstanceMethod("getBoundingClientRect",
                        &StickySectionShadowNode::getBoundingClientRect),
                        
          InstanceMethod("setClass", &StickySectionShadowNode::setClass),
          // getter isConnected
          InstanceAccessor("isConnected", &StickySectionShadowNode::isConnected, nullptr,
                            static_cast<napi_property_attributes>(
                                napi_writable | napi_configurable)),

         // getter instanceId
         InstanceAccessor("instanceId", &StickySectionShadowNode::getInstanceId, nullptr,
                          static_cast<napi_property_attributes>(
                              napi_writable | napi_configurable)),
        });
    Napi::FunctionReference *constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);

    return constructor;
  }
  StickySectionShadowNode::StickySectionShadowNode(const Napi::CallbackInfo &info)
      : Napi::ObjectWrap<StickySectionShadowNode>(info), ShadowNode(info) {
    if (info.Length() < 1) {
      throw Napi::TypeError::New(info.Env(), "Constructor: Wrong number of arguments");
    }
    if (!info[0].IsString()) {
      throw Napi::TypeError::New(info.Env(), "Constructor: First argument must be a string");
    }
    m_instanceId = info[0].As<Napi::String>().Utf8Value();
  }
} // namespace Skyline