#ifndef __SHADOW_NODE__HH__
#define __SHADOW_NODE__HH__
#include "../../base_client.hh"
#include <napi.h>

namespace Skyline {
template<typename T>
std::vector<Napi::ClassPropertyDescriptor<T>> GetCommonMethods() {
  std::vector<Napi::ClassPropertyDescriptor<T>> methods;
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("addClass", &T::addClass));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("appendChild", &T::appendChild));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("forceDetached", &T::forceDetached));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("getBoundingClientRect", &T::getBoundingClientRect));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("getParentNode", &T::getParentNode));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("matches", &T::matches));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("release", &T::release));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("removeChild", &T::removeChild));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("setAttribute", &T::setAttribute));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("setAttributes", &T::setAttributes));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("setClass", &T::setClass));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("setEventDefaultPrevented", &T::setEventDefaultPrevented));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("setId", &T::setId));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("setLayoutCallback", &T::setLayoutCallback));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("setListenerOption", &T::setListenerOption));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("setStyle", &T::setStyle));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("setStyleScope", &T::setStyleScope));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("setTouchEventNeedsLocalCoords", &T::setTouchEventNeedsLocalCoords));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("spliceAppend", &T::spliceAppend));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("spliceRemove", &T::spliceRemove));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("addAnimatedStyle", &T::addAnimatedStyle));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("appendFragment", &T::appendFragment));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("appendStyle", &T::appendStyle));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("associateComponent", &T::associateComponent));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("clearAnimatedStyle", &T::clearAnimatedStyle));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("clearClasses", &T::clearClasses));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("cloneNode", &T::cloneNode));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("createIntersectionObserver", &T::createIntersectionObserver));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("equal", &T::equal));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("findChildPosition", &T::findChildPosition));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("getAfterPseudoNode", &T::getAfterPseudoNode));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("getBeforePseudoNode", &T::getBeforePseudoNode));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("getChildNode", &T::getChildNode));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("getOffsetPosition", &T::getOffsetPosition));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("insertBefore", &T::insertBefore));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("insertChild", &T::insertChild));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("insertFragmentBefore", &T::insertFragmentBefore));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("length", &T::length));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("markAsListItem", &T::markAsListItem));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("markAsRepaintBoundary", &T::markAsRepaintBoundary));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("preventSnapshot", &T::preventSnapshot));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("removeAttribute", &T::removeAttribute));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("removeClass", &T::removeClass));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("replaceChild", &T::replaceChild));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("setIntersectionListener", &T::setIntersectionListener));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("splice", &T::splice));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("spliceBefore", &T::spliceBefore));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("updateStyle", &T::updateStyle));

  // Add instance accessors
  methods.push_back(Napi::InstanceWrap<T>::InstanceAccessor(
    "isConnected", &T::isConnected, nullptr,
    static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  methods.push_back(Napi::InstanceWrap<T>::InstanceAccessor(
    "instanceId", &T::getInstanceId, nullptr,
    static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
    methods.push_back(Napi::InstanceWrap<T>::InstanceAccessor(
      "viewTag", &T::viewTag, nullptr,
      static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  methods.push_back(Napi::InstanceWrap<T>::InstanceAccessor("__secondaryAnimationMapperId", &T::__secondaryAnimationMapperId, nullptr, static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  return methods;
}

class ShadowNode : public BaseClient {
public:
  ShadowNode(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);
  Napi::Value __secondaryAnimationMapperId(const Napi::CallbackInfo &info);
/**
 * Available Shadow Node Methods:
 *
 * addAnimatedStyle()          - Add animated style to the node
 * addClass()                  - Add CSS class
 * appendChild()               - Append a child node
 * appendFragment()            - Append a document fragment
 * appendStyle()               - Append style to the node
 * associateComponent()        - Associate with component
 * clearAnimatedStyle()        - Remove all animated styles
 * clearClasses()              - Clear all CSS classes
 * cloneNode()                 - Create a copy of the node
 * createIntersectionObserver() - Create intersection observer
 * equal()                     - Compare with another node
 * findChildPosition()         - Find position of child
 * forceDetached()             - Force detach from parent
 * getAfterPseudoNode()        - Get ::after pseudo-element
 * getBeforePseudoNode()       - Get ::before pseudo-element
 * getBoundingClientRect()     - Get bounding rectangle
 * getChildNode()              - Get child at position
 * getOffsetPosition()         - Get offset position
 * getParentNode()             - Get parent node
 * insertBefore()              - Insert node before reference
 * insertChild()               - Insert a child at position
 * insertFragmentBefore()      - Insert fragment before reference
 * isConnected                 - Check if connected to document
 * length()                    - Get number of children
 * markAsListItem()            - Mark as list item
 * markAsRepaintBoundary()     - Mark node as repaint boundary
 * matches()                   - Test if matches selector
 * preventSnapshot()           - Prevent node snapshot
 * release()                   - Release node resources
 * removeAttribute()           - Remove an attribute
 * removeChild()               - Remove child node
 * removeClass()               - Remove specific CSS class
 * replaceChild()              - Replace a child node
 * setAttribute()              - Set a single attribute
 * setAttributes()             - Set multiple attributes
 * setClass()                  - Set CSS class(es)
 * setEventDefaultPrevented()  - Set if event default is prevented
 * setId()                     - Set element ID
 * setIntersectionListener()   - Set intersection listener
 * setLayoutCallback()         - Set layout callback
 * setListenerOption()         - Set event listener option
 * setStyle()                  - Set inline style
 * setStyleScope()             - Set style scope for the node
 * setTouchEventNeedsLocalCoords() - Configure touch event coordinates
 * splice()                    - Modify children array
 * spliceAppend()              - Append via splice
 * spliceBefore()              - Insert before via splice
 * spliceRemove()              - Remove via splice
 * updateStyle()               - Update existing style
 */
  Napi::Value addClass(const Napi::CallbackInfo &info);
  Napi::Value appendChild(const Napi::CallbackInfo &info);
  Napi::Value forceDetached(const Napi::CallbackInfo &info);
  Napi::Value getBoundingClientRect(const Napi::CallbackInfo &info);
  Napi::Value getParentNode(const Napi::CallbackInfo &info);
  Napi::Value isConnected(const Napi::CallbackInfo &info);
  Napi::Value viewTag(const Napi::CallbackInfo &info);
  Napi::Value matches(const Napi::CallbackInfo &info);
  Napi::Value release(const Napi::CallbackInfo &info);
  Napi::Value removeChild(const Napi::CallbackInfo &info);
  Napi::Value setAttribute(const Napi::CallbackInfo &info);
  Napi::Value setAttributes(const Napi::CallbackInfo &info);
  Napi::Value setClass(const Napi::CallbackInfo &info);
  Napi::Value setEventDefaultPrevented(const Napi::CallbackInfo &info);
  Napi::Value setId(const Napi::CallbackInfo &info);
  Napi::Value setLayoutCallback(const Napi::CallbackInfo &info);
  Napi::Value setListenerOption(const Napi::CallbackInfo &info);
  Napi::Value setStyle(const Napi::CallbackInfo &info);
  Napi::Value setStyleScope(const Napi::CallbackInfo &info);
  Napi::Value setTouchEventNeedsLocalCoords(const Napi::CallbackInfo &info);
  Napi::Value spliceAppend(const Napi::CallbackInfo &info);
  Napi::Value spliceRemove(const Napi::CallbackInfo &info);

  // 添加缺失的方法声明
  Napi::Value addAnimatedStyle(const Napi::CallbackInfo &info);
  Napi::Value appendFragment(const Napi::CallbackInfo &info);
  Napi::Value appendStyle(const Napi::CallbackInfo &info);
  Napi::Value associateComponent(const Napi::CallbackInfo &info);
  Napi::Value clearAnimatedStyle(const Napi::CallbackInfo &info);
  Napi::Value clearClasses(const Napi::CallbackInfo &info);
  Napi::Value cloneNode(const Napi::CallbackInfo &info);
  Napi::Value createIntersectionObserver(const Napi::CallbackInfo &info);
  Napi::Value equal(const Napi::CallbackInfo &info);
  Napi::Value findChildPosition(const Napi::CallbackInfo &info);
  Napi::Value getAfterPseudoNode(const Napi::CallbackInfo &info);
  Napi::Value getBeforePseudoNode(const Napi::CallbackInfo &info);
  Napi::Value getChildNode(const Napi::CallbackInfo &info);
  Napi::Value getOffsetPosition(const Napi::CallbackInfo &info);
  Napi::Value insertBefore(const Napi::CallbackInfo &info);
  Napi::Value insertChild(const Napi::CallbackInfo &info);
  Napi::Value insertFragmentBefore(const Napi::CallbackInfo &info);
  Napi::Value length(const Napi::CallbackInfo &info);
  Napi::Value markAsListItem(const Napi::CallbackInfo &info);
  Napi::Value markAsRepaintBoundary(const Napi::CallbackInfo &info);
  Napi::Value preventSnapshot(const Napi::CallbackInfo &info);
  Napi::Value removeAttribute(const Napi::CallbackInfo &info);
  Napi::Value removeClass(const Napi::CallbackInfo &info);
  Napi::Value replaceChild(const Napi::CallbackInfo &info);
  Napi::Value setIntersectionListener(const Napi::CallbackInfo &info);
  Napi::Value splice(const Napi::CallbackInfo &info);
  Napi::Value spliceBefore(const Napi::CallbackInfo &info);
  Napi::Value updateStyle(const Napi::CallbackInfo &info);
};
} // namespace Skyline
#endif // __SHADOW_NODE__HH__