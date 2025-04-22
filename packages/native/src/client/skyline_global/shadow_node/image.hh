#ifndef __IMAGE_SHADOW_NODE__HH__
#define __IMAGE_SHADOW_NODE__HH__
#include "node.hh"
#include <napi.h>


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
  Napi::Value getImageLoadInfo(const Napi::CallbackInfo &info);
};
} // namespace Skyline
#endif // __IMAGE_SHADOW_NODE__HH__