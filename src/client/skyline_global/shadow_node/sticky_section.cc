#include "sticky_section.hh"

namespace Skyline {
  Napi::FunctionReference *StickySectionShadowNode::GetClazz(Napi::Env env) {
    auto methods = GetCommonMethods<StickySectionShadowNode>();
    
    Napi::Function func = DefineClass(env, "StickySectionShadowNode", methods);
    
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