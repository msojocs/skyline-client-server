#include "node.hh"
#include "napi.h"
#include <sstream>
#include <vector>

namespace Skyline {
ShadowNode::ShadowNode(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: Wrong number of arguments");
  }

  m_instanceId = info[0].As<Napi::Number>().Int64Value();
}
Napi::Value ShadowNode::__secondaryAnimationMapperId(const Napi::CallbackInfo &info) {
  return getProperty(info, __func__);
}
Napi::Value ShadowNode::setStyleScope(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "setStyleScope: Wrong number of arguments");
  }
  if (!info[0].IsNumber()) {
    throw Napi::TypeError::New(info.Env(), "setStyleScope: First argument must be a number");
  }
  this->styleScope = info[0].As<Napi::Number>().Int32Value();
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::addClass(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "addClass: Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(), "addClass: First argument must be a string");
  }
  std::string className = info[0].As<Napi::String>().Utf8Value();
  classList.push_back(className);
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::setStyle(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::setEventDefaultPrevented(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::appendChild(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::spliceAppend(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value ShadowNode::setId(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "setId: Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(), "setId: First argument must be a string");
  }
  this->id = info[0].As<Napi::String>().Utf8Value();
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

/**
 * TODO: Local处理
 */
Napi::Value ShadowNode::matches(const Napi::CallbackInfo &info) {
  
  if (info.Length() < 2) {
    throw Napi::TypeError::New(info.Env(), "matches: Wrong number of arguments");
  }
  if (!info[0].IsNumber()) {
    throw Napi::TypeError::New(info.Env(), "matches: First argument must be a number");
  }
  if (!info[1].IsString()) {
    throw Napi::TypeError::New(info.Env(), "matches: Second argument must be a string");
  }
  int styleScope = info[0].As<Napi::Number>().Int32Value();
  if (styleScope != this->styleScope) {
    return Napi::Boolean::New(info.Env(), false);
  }
  std::string className = info[1].As<Napi::String>().Utf8Value();
  std::istringstream f(className);
  std::string s;
  std::vector<std::string> tempList;
  while (getline(f, s, ' ')) {
    tempList.push_back(s);
  }
  std::string last = tempList[tempList.size() - 1];
  if (last[0] == '#') {
    // element id
    auto id = last.substr(1, last.length() - 1);
    if (id == this->id) {
      return Napi::Boolean::New(info.Env(), true);
    }
  }
  // else if (last[0] == '.') {
  //   // class name
  //   auto className = last.substr(1, last.length() - 1);
  //   if (std::find(classList.begin(), classList.end(), className) != classList.end()) {
  //     return Napi::Boolean::New(info.Env(), true);
  //   }
  // }
  // return Napi::Boolean::New(info.Env(),false);
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