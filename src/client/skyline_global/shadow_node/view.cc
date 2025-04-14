#include "view.hh"
#include "napi.h"
#include "node.hh"
#include <vector>

namespace Skyline {
Napi::FunctionReference *ViewShadowNode::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<ViewShadowNode>();
  
  Napi::Function func = DefineClass(env, "ViewShadowNode", methods);
  
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