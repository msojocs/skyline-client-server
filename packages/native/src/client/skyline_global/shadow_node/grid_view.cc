#include "grid_view.hh"
#include "napi.h"
#include "node.hh"

namespace Skyline {
Napi::FunctionReference *GridViewShadowNode::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<GridViewShadowNode>();

  Napi::Function func = DefineClass(env, "GridViewShadowNode", methods);

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