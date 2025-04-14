#ifndef __SWIPER_SHADOW_NODE__HH__
#define __SWIPER_SHADOW_NODE__HH__
#include "node.hh"
#include <napi.h>


namespace Skyline {
class SwiperShadowNode : public Napi::ObjectWrap<SwiperShadowNode>,
                         public ShadowNode {
public:
  SwiperShadowNode(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);

private:
  /**
   * 相比view多出来的
   * onChangeEvent: ƒ onChangeEvent()
  onScrollEndEvent: ƒ onScrollEndEvent()
  onScrollEvent: ƒ onScrollEvent()
   */
  Napi::Value onChangeEvent(const Napi::CallbackInfo &info);
  Napi::Value onScrollEndEvent(const Napi::CallbackInfo &info);
  Napi::Value onScrollEvent(const Napi::CallbackInfo &info);
};
} // namespace Skyline
#endif // __IMAGE_SHADOW_NODE__HH__