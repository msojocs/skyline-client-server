#include "node.hh"
#include "napi.h"

namespace Skyline {
ShadowNode::ShadowNode(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: First argument must be a string");
  }
  m_instanceId = info[0].As<Napi::String>().Utf8Value();
}
Napi::Value ShadowNode::setStyleScope(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::addClass(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::setStyle(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value
ShadowNode::setEventDefaultPrevented(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::appendChild(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::spliceAppend(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::setId(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::forceDetached(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::spliceRemove(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::release(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::setAttributes(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::getBoundingClientRect(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

// isConnected
Napi::Value ShadowNode::isConnected(const Napi::CallbackInfo &info) {
  return getProperty(info, __func__);
}
Napi::Value ShadowNode::viewTag(const Napi::CallbackInfo &info) {
  return getProperty(info, __func__);
}
// setClass 
Napi::Value ShadowNode::setClass(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
// setListenerOption
Napi::Value ShadowNode::setListenerOption(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::matches(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::removeChild(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::setLayoutCallback(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::setTouchEventNeedsLocalCoords(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::setAttribute(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::getParentNode(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::addAnimatedStyle(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::appendFragment(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::appendStyle(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::associateComponent(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::clearAnimatedStyle(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::clearClasses(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::cloneNode(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::createIntersectionObserver(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::equal(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::findChildPosition(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::getAfterPseudoNode(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::getBeforePseudoNode(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::getChildNode(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::getOffsetPosition(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::insertBefore(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::insertChild(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::insertFragmentBefore(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::length(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::markAsListItem(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::markAsRepaintBoundary(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::preventSnapshot(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::removeAttribute(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::removeClass(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::replaceChild(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::setIntersectionListener(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::splice(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::spliceBefore(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::updateStyle(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

} // namespace Skyline