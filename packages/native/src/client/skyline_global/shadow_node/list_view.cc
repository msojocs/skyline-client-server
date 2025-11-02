#include "list_view.hh"
#include "napi.h"
#include "node.hh"

namespace Skyline {
Napi::FunctionReference *ListViewShadowNode::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<ListViewShadowNode>();

  Napi::Function func = DefineClass(env, "ListViewShadowNode", methods);

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

  m_instanceId = info[0].As<Napi::Number>().Int64Value();
}
} // namespace Skyline
