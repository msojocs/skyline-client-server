#ifndef __IMAGE_SHADOW_NODE__HH__
#define __IMAGE_SHADOW_NODE__HH__
#include <napi.h>
#include "node.hh"

namespace Skyline {
  class ImageShadowNode : public Napi::ObjectWrap<ImageShadowNode>,
  public ShadowNode {
public:
ImageShadowNode(const Napi::CallbackInfo &info);
static Napi::FunctionReference *GetClazz(Napi::Env env);
private:
/**
 * 相比view多出来的
 * getImageLoadInfo: ƒ getImageLoadInfo()
 */
  };
}
#endif // __IMAGE_SHADOW_NODE__HH__