#ifndef __SWIPER_SHADOW_NODE__HH__
#define __SWIPER_SHADOW_NODE__HH__
#include <napi.h>
#include "node.hh"

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
  };
}
#endif // __IMAGE_SHADOW_NODE__HH__