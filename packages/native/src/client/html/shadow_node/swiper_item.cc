#include "swiper_item.hh"
#include "napi.h"
#include "node.hh"

namespace Skyline {
Napi::FunctionReference *SwiperItemShadowNode::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<SwiperItemShadowNode>();
  
  Napi::Function func = DefineClass(env, "SwiperItemShadowNode", methods);
  
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
SwiperItemShadowNode::SwiperItemShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<SwiperItemShadowNode>(info), ShadowNode(info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: Wrong number of arguments");
  }

  m_instanceId = info[0].As<Napi::Number>().Int64Value();
}
} // namespace Skyline