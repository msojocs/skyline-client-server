#include "scroll_view.hh"
#include "../../../include/convert.hh"
#include "../../websocket.hh"

namespace Skyline {
Napi::FunctionReference *ScrollViewShadowNode::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<ScrollViewShadowNode>();
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("setRefresherHeader", &ScrollViewShadowNode::setRefresherHeader));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("requestRefresherRefresh", &ScrollViewShadowNode::requestRefresherRefresh));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("requestRefresherTwoLevel", &ScrollViewShadowNode::requestRefresherTwoLevel));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("scrollIntoView", &ScrollViewShadowNode::scrollIntoView));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("scrollTo", &ScrollViewShadowNode::scrollTo));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("getScrollPosition", &ScrollViewShadowNode::getScrollPosition));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("holdPosition", &ScrollViewShadowNode::holdPosition));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("onRefresherAbortEvent", &ScrollViewShadowNode::onRefresherAbortEvent));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("onRefresherPullingEvent", &ScrollViewShadowNode::onRefresherPullingEvent));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("onRefresherRefreshEvent", &ScrollViewShadowNode::onRefresherRefreshEvent));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("onRefresherRestoreEvent", &ScrollViewShadowNode::onRefresherRestoreEvent));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("onRefresherStatusChangeEvent", &ScrollViewShadowNode::onRefresherStatusChangeEvent));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("onRefresherWillRefreshEvent", &ScrollViewShadowNode::onRefresherWillRefreshEvent));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("onScrollEndEvent", &ScrollViewShadowNode::onScrollEndEvent));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("onScrollEvent", &ScrollViewShadowNode::onScrollEvent));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("onScrollStartEvent", &ScrollViewShadowNode::onScrollStartEvent));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("onScrollToLowerEvent", &ScrollViewShadowNode::onScrollToLowerEvent));
  methods.push_back(Napi::InstanceWrap<ScrollViewShadowNode>::InstanceMethod("onScrollToUpperEvent", &ScrollViewShadowNode::onScrollToUpperEvent));
  
  Napi::Function func = DefineClass(env, "ScrollViewShadowNode", methods);

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
ScrollViewShadowNode::ScrollViewShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<ScrollViewShadowNode>(info), ShadowNode(info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "Constructor: Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(), "Constructor: First argument must be a string");
  }
  m_instanceId = info[0].As<Napi::String>().Utf8Value();
}
// getScrollPosition
Napi::Value ScrollViewShadowNode::getScrollPosition(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
// holdPosition
Napi::Value ScrollViewShadowNode::holdPosition(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
// onRefresherAbortEvent
Napi::Value ScrollViewShadowNode::onRefresherAbortEvent(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
// onRefresherPullingEvent
Napi::Value ScrollViewShadowNode::onRefresherPullingEvent(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
// onRefresherRefreshEvent
Napi::Value ScrollViewShadowNode::onRefresherRefreshEvent(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
// onRefresherRestoreEvent
Napi::Value ScrollViewShadowNode::onRefresherRestoreEvent(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
// onRefresherStatusChangeEvent
Napi::Value ScrollViewShadowNode::onRefresherStatusChangeEvent(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
// onRefresherWillRefreshEvent
Napi::Value ScrollViewShadowNode::onRefresherWillRefreshEvent(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ScrollViewShadowNode::onScrollEndEvent(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ScrollViewShadowNode::onScrollEvent(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ScrollViewShadowNode::onScrollStartEvent(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ScrollViewShadowNode::onScrollToLowerEvent(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ScrollViewShadowNode::onScrollToUpperEvent(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ScrollViewShadowNode::requestRefresherRefresh(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ScrollViewShadowNode::requestRefresherTwoLevel(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ScrollViewShadowNode::scrollIntoView(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ScrollViewShadowNode::scrollTo(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ScrollViewShadowNode::setRefresherHeader(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
}