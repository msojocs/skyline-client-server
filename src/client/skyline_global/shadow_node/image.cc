#include "image.hh"
#include "napi.h"

namespace Skyline {
Napi::FunctionReference *ImageShadowNode::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<ImageShadowNode>();

  Napi::Function func = DefineClass(env, "ImageShadowNode", methods);

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
ImageShadowNode::ImageShadowNode(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<ImageShadowNode>(info), ShadowNode(info) {
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