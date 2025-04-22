#ifndef __SWIPER_ITEM_SHADOW_NODE__HH__
#define __SWIPER_ITEM_SHADOW_NODE__HH__
#include "node.hh"
#include <napi.h>

namespace Skyline {
class SwiperItemShadowNode : public Napi::ObjectWrap<SwiperItemShadowNode>,
                       public ShadowNode {
public:
  SwiperItemShadowNode(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);

private:
};
} // namespace Skyline
#endif // __VIEW_SHADOW_NODE__HH__