#ifndef __LIST_VIEW_SHADOW_NODE__HH__
#define __LIST_VIEW_SHADOW_NODE__HH__
#include "node.hh"
#include <napi.h>

namespace Skyline {
class ListViewShadowNode : public Napi::ObjectWrap<ListViewShadowNode>,
                       public ShadowNode {
public:
  ListViewShadowNode(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);

private:
};
} // namespace Skyline
#endif // __LISTVIEW_SHADOW_NODE__HH__
