#include "../include/page_context.hh"
#include "napi.h"
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>
#include "../websocket.hh"
#include "../../include/convert.hh"

namespace Skyline {
void PageContext::Init(Napi::Env env, Napi::Object exports) {

  Napi::Function func = DefineClass(
      env, "PageContext",
      {InstanceMethod("appendCompiledStyleSheets",
                      &PageContext::appendCompiledStyleSheets),
       InstanceMethod("appendStyleSheet", &PageContext::appendStyleSheet),
       InstanceMethod("appendStyleSheetIndex",
                      &PageContext::appendStyleSheetIndex),
       InstanceMethod("appendStyleSheets", &PageContext::appendStyleSheets),
       InstanceMethod("attach", &PageContext::attach),
       InstanceMethod("attachCustomRoute", &PageContext::attachCustomRoute),
       InstanceMethod("clearStylesheets", &PageContext::clearStylesheets),
       InstanceMethod("createElement", &PageContext::createElement),
       InstanceMethod("createFragment", &PageContext::createFragment),
       InstanceMethod("createStyleSheetIndexGroup",
                      &PageContext::createStyleSheetIndexGroup),
       InstanceMethod("createTextNode", &PageContext::createTextNode),
       InstanceMethod("finishStyleSheetsCompilation",
                      &PageContext::finishStyleSheetsCompilation),
       InstanceMethod("getComputedStyle", &PageContext::getComputedStyle),
       InstanceMethod("getHostNode", &PageContext::getHostNode),
       InstanceMethod("getNodeFromPoint", &PageContext::getNodeFromPoint),
       InstanceMethod("getRootNode", &PageContext::getRootNode),
       InstanceMethod("getWindowId", &PageContext::getWindowId),
       InstanceMethod("isTab", &PageContext::isTab),
       InstanceMethod("loadFontFace", &PageContext::loadFontFace),
       InstanceMethod("preCompileStyleSheets",
                      &PageContext::preCompileStyleSheets),
       InstanceMethod("recalcStyle", &PageContext::recalcStyle),
       InstanceMethod("release", &PageContext::release),
       InstanceMethod("setAsTab", &PageContext::setAsTab),
       InstanceMethod("setNavigateBackInterception",
                      &PageContext::setNavigateBackInterception),
       InstanceMethod("startRender", &PageContext::startRender),
       InstanceMethod("updateRouteConfig", &PageContext::updateRouteConfig),
       InstanceAccessor("instanceId", &PageContext::getInstanceId, nullptr,
                        static_cast<napi_property_attributes>(napi_writable |
                                                             napi_configurable)),
       InstanceAccessor("frameworkType", &PageContext::getFrameworkType, &PageContext::setFrameworkType,
              static_cast<napi_property_attributes>(napi_writable |
                                                   napi_configurable)),
      });
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);
  exports.Set("PageContext", func);
}
PageContext::PageContext(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<PageContext>(info) {
  if (info.Length() < 3) {
    throw Napi::TypeError::New(info.Env(), "PageContext: Wrong number of arguments");
  }
  if (!info[0].IsNumber()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be a number");
  }
  if (!info[1].IsNumber()) {
    throw Napi::TypeError::New(info.Env(), "Second argument must be a number");
  }
  if (!info[2].IsObject()) {
    throw Napi::TypeError::New(info.Env(), "Third argument must be an object");
  }
  spdlog::info("PageContext constructor");
  auto pageId = info[0].As<Napi::Number>().Int32Value();
  auto windowId = info[1].As<Napi::Number>().Int32Value();
  auto options = info[2].As<Napi::Object>();
  // 解析 options 对象
  /**
   * {defaultBlockLayout: true
      defaultContentBox: false
      enableImagePreload: false
      enableScrollViewAutoSize: false
      tagNameStyleIsolation: 0}
   */
  nlohmann::json optionsJson;
  if (options.Has("defaultBlockLayout")) {
    optionsJson["defaultBlockLayout"] = options.Get("defaultBlockLayout").As<Napi::Boolean>().Value();
  }
  if (options.Has("defaultContentBox")) {
    optionsJson["defaultContentBox"] = options.Get("defaultContentBox").As<Napi::Boolean>().Value();
  }
  if (options.Has("enableImagePreload")) {
    optionsJson["enableImagePreload"] = options.Get("enableImagePreload").As<Napi::Boolean>().Value();
  }
  if (options.Has("enableScrollViewAutoSize")) {
    optionsJson["enableScrollViewAutoSize"] = options.Get("enableScrollViewAutoSize").As<Napi::Boolean>().Value();
  }
  if (options.Has("tagNameStyleIsolation")) {
    optionsJson["tagNameStyleIsolation"] = options.Get("tagNameStyleIsolation").As<Napi::Number>().Int32Value();
  }
  nlohmann::json data {
    pageId,
    windowId,
    optionsJson,
  };
  auto result = WebSocket::callConstructorSync("PageContext", data);
  m_instanceId = result["instanceId"].get<std::string>();
}

Napi::Value PageContext::getFrameworkType(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  auto result = WebSocket::callDynamicPropertyGetSync(m_instanceId, "frameworkType", args);
  auto returnValue = result["returnValue"];
  return Napi::Number::New(info.Env(), returnValue.get<int>());
}
void PageContext::setFrameworkType(const Napi::CallbackInfo &info, const Napi::Value &value) {
  nlohmann::json args;
  auto env = info.Env();
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  // 发送消息到 WebSocket
  WebSocket::callDynamicPropertySetSync(m_instanceId, "frameworkType", args);
}
/**
 * 1个Array参数
 */
Napi::Value PageContext::appendCompiledStyleSheets(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::appendStyleSheet(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::appendStyleSheetIndex(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
/**
 * 参数数量：1个
 * 参数1：Array
 */
Napi::Value PageContext::appendStyleSheets(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

/**
 * 2个参数
 */
Napi::Value PageContext::attach(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::attachCustomRoute(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::clearStylesheets(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::createElement(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::createFragment(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::createStyleSheetIndexGroup(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::createTextNode(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::finishStyleSheetsCompilation(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::getComputedStyle(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::getHostNode(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::getNodeFromPoint(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::getRootNode(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::getWindowId(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::isTab(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::loadFontFace(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
/**
 * @param {Array} sheets
 * @return {Array} AsyncStyleSheets(object)
 */
Napi::Value PageContext::preCompileStyleSheets(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::recalcStyle(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::release(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::setAsTab(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::setNavigateBackInterception(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

/**
 * 1个Callback参数
 */
Napi::Value PageContext::startRender(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value PageContext::updateRouteConfig(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

} // namespace Skyline