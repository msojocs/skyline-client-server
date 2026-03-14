#ifndef __STICKY_SECTION_SHADOW_NODE__HH__
#define __STICKY_SECTION_SHADOW_NODE__HH__
#include <napi.h>
#include "node.hh"

namespace Skyline {
class StickySectionShadowNode : public Napi::ObjectWrap<StickySectionShadowNode>,
                                public ShadowNode {
public:
  static Napi::FunctionReference *GetClazz(Napi::Env env);
  StickySectionShadowNode(const Napi::CallbackInfo &info);
};
}
#endif