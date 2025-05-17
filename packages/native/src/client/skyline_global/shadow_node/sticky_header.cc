#include "sticky_header.hh"
#include "../../socket_client.hh"
#include "../../../common/convert.hh"

namespace Skyline {
Napi::FunctionReference *StickyHeaderShadowNode::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<StickyHeaderShadowNode>();
  methods.push_back(
      InstanceMethod("onStickOnTopChangeEvent",
                     &StickyHeaderShadowNode::onStickOnTopChangeEvent));
  Napi::Function func = DefineClass(env, "StickyHeaderShadowNode", methods);

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
StickyHeaderShadowNode::StickyHeaderShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<StickyHeaderShadowNode>(info), ShadowNode(info) {
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
Napi::Value StickyHeaderShadowNode::onStickOnTopChangeEvent(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

} // namespace Skyline