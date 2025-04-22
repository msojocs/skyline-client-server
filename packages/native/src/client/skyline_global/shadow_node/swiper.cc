#include "swiper.hh"
#include "napi.h"
#include "../../websocket.hh"
#include "../../../common/convert.hh"

namespace Skyline {
Napi::FunctionReference *SwiperShadowNode::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<SwiperShadowNode>();
  methods.push_back(
      InstanceMethod("onChangeEvent", &SwiperShadowNode::onChangeEvent));
  methods.push_back(
      InstanceMethod("onScrollEndEvent", &SwiperShadowNode::onScrollEndEvent));
  methods.push_back(
      InstanceMethod("onScrollEvent", &SwiperShadowNode::onScrollEvent));
  Napi::Function func = DefineClass(env, "SwiperShadowNode", methods);
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
SwiperShadowNode::SwiperShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<SwiperShadowNode>(info), ShadowNode(info) {
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
Napi::Value SwiperShadowNode::onChangeEvent(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value SwiperShadowNode::onScrollEndEvent(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value SwiperShadowNode::onScrollEvent(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
} // namespace Skyline