#include "view.hh"
#include "napi.h"
#include "node.hh"

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

  m_instanceId = info[0].As<Napi::Number>().Int64Value();
}
} // namespace Skyline