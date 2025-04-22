#include "input.hh"
#include "napi.h"
#include "node.hh"

namespace Skyline {
Napi::FunctionReference *InputShadowNode::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<InputShadowNode>();
  methods.push_back(InstanceMethod("blur", &InputShadowNode::blur, napi_enumerable));
  methods.push_back(InstanceMethod("focus", &InputShadowNode::focus, napi_enumerable));
  methods.push_back(InstanceMethod("onKeyboardComplete", &InputShadowNode::onKeyboardComplete, napi_enumerable));
  methods.push_back(InstanceMethod("onKeyboardCompositionEnd", &InputShadowNode::onKeyboardCompositionEnd, napi_enumerable));
  methods.push_back(InstanceMethod("onKeyboardCompositionStart", &InputShadowNode::onKeyboardCompositionStart, napi_enumerable));
  methods.push_back(InstanceMethod("onKeyboardCompositionUpdate", &InputShadowNode::onKeyboardCompositionUpdate, napi_enumerable));
  methods.push_back(InstanceMethod("onKeyboardConfirm", &InputShadowNode::onKeyboardConfirm, napi_enumerable));
  methods.push_back(InstanceMethod("onKeyboardHeightChange", &InputShadowNode::onKeyboardHeightChange, napi_enumerable));
  methods.push_back(InstanceMethod("onKeyboardLineChange", &InputShadowNode::onKeyboardLineChange, napi_enumerable));
  methods.push_back(InstanceMethod("onKeyboardSelectionChange", &InputShadowNode::onKeyboardSelectionChange, napi_enumerable));
  methods.push_back(InstanceMethod("onKeyboardShow", &InputShadowNode::onKeyboardShow, napi_enumerable));
  methods.push_back(InstanceMethod("onKeyboardValueChange", &InputShadowNode::onKeyboardValueChange, napi_enumerable));
  methods.push_back(InstanceMethod("onSetValueCompleted", &InputShadowNode::onSetValueCompleted, napi_enumerable));
  Napi::Function func = DefineClass(env, "InputShadowNode", methods);
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}

InputShadowNode::InputShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<InputShadowNode>(info), ShadowNode(info) {
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
Napi::Value InputShadowNode::blur(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value InputShadowNode::focus(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value InputShadowNode::onKeyboardComplete(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value InputShadowNode::onKeyboardCompositionEnd(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value InputShadowNode::onKeyboardCompositionStart(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value InputShadowNode::onKeyboardCompositionUpdate(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value InputShadowNode::onKeyboardConfirm(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value InputShadowNode::onKeyboardHeightChange(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value InputShadowNode::onKeyboardLineChange(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value InputShadowNode::onKeyboardSelectionChange(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value InputShadowNode::onKeyboardShow(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value InputShadowNode::onKeyboardValueChange(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value InputShadowNode::onSetValueCompleted(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
} // namespace Skyline
