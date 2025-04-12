#ifndef __STICKY_HEADER_SHADOW_NODE__HH__
#define __STICKY_HEADER_SHADOW_NODE__HH__
#include <napi.h>
#include "node.hh"

namespace Skyline {
class StickyHeaderShadowNode : public Napi::ObjectWrap<StickyHeaderShadowNode>,
                                public ShadowNode {
public:
  static Napi::FunctionReference *GetClazz(Napi::Env env);
  StickyHeaderShadowNode(const Napi::CallbackInfo &info);
private:
  /**
   * 相比view多出来的
   * onStickOnTopChangeEvent: ƒ onStickOnTopChangeEvent()
   */
};
}
#endif