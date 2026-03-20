#ifndef __WEBVIEW_ELEMENT__HH__
#define __WEBVIEW_ELEMENT__HH__
#include "node.hh"
#include <napi.h>

namespace HTML {
class WebviewElement : public Napi::ObjectWrap<WebviewElement>,
                       public ShadowNode {
public:
  WebviewElement(const Napi::CallbackInfo &info);
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
Napi::Value addEventListener(const Napi::CallbackInfo &info);
Napi::Value executeScript(const Napi::CallbackInfo &info);
Napi::Value getAttribute(const Napi::CallbackInfo &info);
Napi::Value getUserAgent(const Napi::CallbackInfo &info);
Napi::Value removeEventListener(const Napi::CallbackInfo &info);
void setOndialog(const Napi::CallbackInfo &info, const Napi::Value &value);
Napi::Value setUserAgentOverride(const Napi::CallbackInfo &info);
Napi::Value showDevTools(const Napi::CallbackInfo &info);

Napi::Value getRequest(const Napi::CallbackInfo &info);
Napi::Value getSrc(const Napi::CallbackInfo &info);
void setSrc(const Napi::CallbackInfo &info, const Napi::Value &value);
Napi::Value getStyle(const Napi::CallbackInfo &info);
void setStyle(const Napi::CallbackInfo &info, const Napi::Value &value);
};
} // namespace HTML
#endif // __WEBVIEW_ELEMENT__HH__
