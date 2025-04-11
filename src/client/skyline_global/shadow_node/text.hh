#ifndef __TEXT_SHADOW_NODE__HH__
#define __TEXT_SHADOW_NODE__HH__
#include <napi.h>
#include "../../include/base_client.hh"
#include "node.hh"

namespace Skyline {
  class TextShadowNode : public Napi::ObjectWrap<TextShadowNode>,
  public ShadowNode {
public:
TextShadowNode(const Napi::CallbackInfo &info);
static Napi::FunctionReference *GetClazz(Napi::Env env);
private:
/**
 * setText: ƒ setText()
 */
  };
}
#endif // __TEXT_SHADOW_NODE__HH__