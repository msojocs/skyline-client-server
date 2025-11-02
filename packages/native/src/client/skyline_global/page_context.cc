#include "page_context.hh"
#include "napi.h"
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

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
  m_instanceId = sendConstructorToServerSync(info, __func__);
}

Napi::Value PageContext::getFrameworkType(const Napi::CallbackInfo &info) {
  return getProperty(info, "frameworkType");
}
void PageContext::setFrameworkType(const Napi::CallbackInfo &info, const Napi::Value &value) {
  setProperty(info, "frameworkType");
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