#ifndef __GRID_VIEW_SHADOW_NODE__HH__
#define __GRID_VIEW_SHADOW_NODE__HH__
#include "node.hh"
#include <napi.h>

namespace Skyline {
class GridViewShadowNode : public Napi::ObjectWrap<GridViewShadowNode>,
                       public ShadowNode {
public:
  GridViewShadowNode(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);

private:
};
} // namespace Skyline
#endif // __VIEW_SHADOW_NODE__HH__