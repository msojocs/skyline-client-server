#ifndef __TEXT_SHADOW_NODE__HH__
#define __TEXT_SHADOW_NODE__HH__
#include "node.hh"
#include <napi.h>


namespace Skyline {
class TextShadowNode : public Napi::ObjectWrap<TextShadowNode>,
                       public ShadowNode {
public:
  TextShadowNode(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);

private:
  /**
   * 相比view多出来的
   * setText: ƒ setText()
   */
  Napi::Value setText(const Napi::CallbackInfo &info);
};
} // namespace Skyline
#endif // __TEXT_SHADOW_NODE__HH__