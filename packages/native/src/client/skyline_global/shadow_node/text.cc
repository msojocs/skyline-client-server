#include "text.hh"
#include "napi.h"
#include "../../socket_client.hh"
#include "../../../common/convert.hh"

namespace Skyline {
Napi::FunctionReference *TextShadowNode::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<TextShadowNode>();
  methods.push_back(
      Napi::InstanceWrap<TextShadowNode>::InstanceMethod("setText", &TextShadowNode::setText));
  Napi::Function func = DefineClass(env, "TextShadowNode", methods);
  
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
TextShadowNode::TextShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<TextShadowNode>(info), ShadowNode(info) {
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
Napi::Value TextShadowNode::setText(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
} // namespace Skyline