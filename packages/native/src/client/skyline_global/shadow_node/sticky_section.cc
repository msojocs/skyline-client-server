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

    m_instanceId = info[0].As<Napi::Number>().Int64Value();
  }
} // namespace Skyline