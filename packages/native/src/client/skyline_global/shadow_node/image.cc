#include "image.hh"
#include "napi.h"
#include "../../../common/convert.hh"

namespace Skyline {
Napi::FunctionReference *ImageShadowNode::GetClazz(Napi::Env env) {
  auto methods = GetCommonMethods<ImageShadowNode>();
  methods.push_back(InstanceMethod("getImageLoadInfo", &ImageShadowNode::getImageLoadInfo));
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

  m_instanceId = info[0].As<Napi::Number>().Int64Value();
}

Napi::Value ImageShadowNode::getImageLoadInfo(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

} // namespace Skyline