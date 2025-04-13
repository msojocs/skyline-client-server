#ifndef __SHADOW_NODE__HH__
#define __SHADOW_NODE__HH__
#include "../../include/base_client.hh"
#include <napi.h>

namespace Skyline {
class ShadowNode : public BaseClient {
public:
  ShadowNode(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);
protected:
/**
 * addAnimatedStyle: ƒ addAnimatedStyle()
addClass: ƒ addClass()
appendChild: ƒ appendChild()
appendFragment: ƒ appendFragment()
appendStyle: ƒ appendStyle()
associateComponent: ƒ associateComponent()
clearAnimatedStyle: ƒ clearAnimatedStyle()
clearClasses: ƒ clearClasses()
cloneNode: ƒ cloneNode()
createIntersectionObserver: ƒ createIntersectionObserver()
equal: ƒ equal()
findChildPosition: ƒ findChildPosition()
forceDetached: ƒ forceDetached()
getAfterPseudoNode: ƒ getAfterPseudoNode()
getBeforePseudoNode: ƒ getBeforePseudoNode()
getBoundingClientRect: ƒ getBoundingClientRect()
getChildNode: ƒ getChildNode()
getOffsetPosition: ƒ getOffsetPosition()
getParentNode: ƒ getParentNode()
insertBefore: ƒ insertBefore()
insertChild: ƒ insertChild()
insertFragmentBefore: ƒ insertFragmentBefore()
length: ƒ length()
markAsListItem: ƒ markAsListItem()
markAsRepaintBoundary: ƒ markAsRepaintBoundary()
matches: ƒ matches()
preventSnapshot: ƒ preventSnapshot()
release: ƒ release()
removeAttribute: ƒ removeAttribute()
removeChild: ƒ removeChild()
removeClass: ƒ removeClass()
replaceChild: ƒ replaceChild()
setAttribute: ƒ setAttribute()
setAttributes: ƒ setAttributes()
setClass: ƒ setClass()
setEventDefaultPrevented: ƒ setEventDefaultPrevented()
setId: ƒ setId()
setIntersectionListener: ƒ setIntersectionListener()
setLayoutCallback: ƒ setLayoutCallback()
setListenerOption: ƒ setListenerOption()
setStyle: ƒ setStyle()
setStyleScope: ƒ setStyleScope()
setTouchEventNeedsLocalCoords: ƒ setTouchEventNeedsLocalCoords()
splice: ƒ splice()
spliceAppend: ƒ spliceAppend()
spliceBefore: ƒ spliceBefore()
spliceRemove: ƒ spliceRemove()
updateStyle: ƒ updateStyle()
 */
  Napi::Value addClass(const Napi::CallbackInfo &info);
  Napi::Value appendChild(const Napi::CallbackInfo &info);
  Napi::Value getInstanceId(const Napi::CallbackInfo &info);
  Napi::Value setEventDefaultPrevented(const Napi::CallbackInfo &info);
  Napi::Value setStyleScope(const Napi::CallbackInfo &info);
  Napi::Value setStyle(const Napi::CallbackInfo &info);
  Napi::Value spliceAppend(const Napi::CallbackInfo &info);
  Napi::Value setId(const Napi::CallbackInfo &info);
  Napi::Value forceDetached(const Napi::CallbackInfo &info);
  Napi::Value spliceRemove(const Napi::CallbackInfo &info);
  Napi::Value release(const Napi::CallbackInfo &info);
  Napi::Value setAttributes(const Napi::CallbackInfo &info);
  Napi::Value getBoundingClientRect(const Napi::CallbackInfo &info);
  Napi::Value isConnected(const Napi::CallbackInfo &info);
  Napi::Value setClass(const Napi::CallbackInfo &info);
  Napi::Value setListenerOption(const Napi::CallbackInfo &info);
  Napi::Value matches(const Napi::CallbackInfo &info);
  Napi::Value removeChild(const Napi::CallbackInfo &info);
  Napi::Value setLayoutCallback(const Napi::CallbackInfo &info);
  Napi::Value setTouchEventNeedsLocalCoords(const Napi::CallbackInfo &info);
};
} // namespace Skyline
#endif // __SHADOW_NODE__HH__