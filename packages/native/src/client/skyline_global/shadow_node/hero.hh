#ifndef __HERO_SHADOW_NODE__HH__
#define __HERO_SHADOW_NODE__HH__
#include "node.hh"
#include <napi.h>

namespace Skyline {
class HeroShadowNode : public Napi::ObjectWrap<HeroShadowNode>,
                       public ShadowNode {
public:
  HeroShadowNode(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);

private:
};
} // namespace Skyline
#endif // __HERO_SHADOW_NODE__HH__