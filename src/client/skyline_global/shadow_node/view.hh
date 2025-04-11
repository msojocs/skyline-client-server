#ifndef __VIEW_SHADOW_NODE__HH__
#define __VIEW_SHADOW_NODE__HH__
#include "node.hh"
#include <napi.h>

namespace Skyline {
class ViewShadowNode : public Napi::ObjectWrap<ViewShadowNode>,
                       public ShadowNode {
public:
  ViewShadowNode(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);

private:
};
} // namespace Skyline
#endif // __VIEW_SHADOW_NODE__HH__