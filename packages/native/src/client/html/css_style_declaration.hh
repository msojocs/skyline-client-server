#ifndef __CSS_STYLE_DECLARATION_HH__
#define __CSS_STYLE_DECLARATION_HH__
#include "node.hh"
#include <napi.h>


namespace HTML {
class CSSStyleDeclaration : public Napi::ObjectWrap<CSSStyleDeclaration>,
                       public ShadowNode {
public:
  CSSStyleDeclaration(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);

private:
  /**
   * 相比view多出来的
   * setText: ƒ setText()
   */
  Napi::Value setText(const Napi::CallbackInfo &info);
  Napi::Value getDisplay(const Napi::CallbackInfo &info);
  void setDisplay(const Napi::CallbackInfo &info, const Napi::Value &value);
  Napi::Value getPointerEvents(const Napi::CallbackInfo &info);
  void setPointerEvents(const Napi::CallbackInfo &info, const Napi::Value &value);
};
} // namespace HTML
#endif // __TEXT_SHADOW_NODE__HH__