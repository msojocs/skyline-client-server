#ifndef __INPUT_SHADOW_NODE__HH__
#define __INPUT_SHADOW_NODE__HH__
#include "node.hh"
#include <napi.h>

namespace Skyline {
class InputShadowNode : public Napi::ObjectWrap<InputShadowNode>,
                       public ShadowNode {
public:
  InputShadowNode(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);

private:
/**
 * 比View多出来的：
 * 
 * blur: ƒ blur()
 * focus: ƒ focus()
 * onKeyboardComplete: ƒ onKeyboardComplete()
    onKeyboardCompositionEnd: ƒ onKeyboardCompositionEnd()
    onKeyboardCompositionStart: ƒ onKeyboardCompositionStart()
    onKeyboardCompositionUpdate: ƒ onKeyboardCompositionUpdate()
    onKeyboardConfirm: ƒ onKeyboardConfirm()
    onKeyboardHeightChange: ƒ onKeyboardHeightChange()
    onKeyboardLineChange: ƒ onKeyboardLineChange()
    onKeyboardSelectionChange: ƒ onKeyboardSelectionChange()
    onKeyboardShow: ƒ onKeyboardShow()
    onKeyboardValueChange: ƒ onKeyboardValueChange()
    onSetValueCompleted: ƒ onSetValueCompleted()
 */
Napi::Value blur(const Napi::CallbackInfo &info);
Napi::Value focus(const Napi::CallbackInfo &info);
Napi::Value onKeyboardComplete(const Napi::CallbackInfo &info);
Napi::Value onKeyboardCompositionEnd(const Napi::CallbackInfo &info);
Napi::Value onKeyboardCompositionStart(const Napi::CallbackInfo &info);
Napi::Value onKeyboardCompositionUpdate(const Napi::CallbackInfo &info);
Napi::Value onKeyboardConfirm(const Napi::CallbackInfo &info);
Napi::Value onKeyboardHeightChange(const Napi::CallbackInfo &info);
Napi::Value onKeyboardLineChange(const Napi::CallbackInfo &info);
Napi::Value onKeyboardSelectionChange(const Napi::CallbackInfo &info);
Napi::Value onKeyboardShow(const Napi::CallbackInfo &info);
Napi::Value onKeyboardValueChange(const Napi::CallbackInfo &info);
Napi::Value onSetValueCompleted(const Napi::CallbackInfo &info);
};
} // namespace Skyline
#endif // __INPUT_SHADOW_NODE__HH__
