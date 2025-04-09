#include "../include/page_context.hh"
#include "napi.h"
#include <spdlog/spdlog.h>
#include "../websocket.hh"

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
       InstanceMethod("updateRouteConfig", &PageContext::updateRouteConfig)});
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);
  exports.Set("PageContext", func);
}
PageContext::PageContext(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<PageContext>(info) {
  if (info.Length() < 3) {
    throw Napi::TypeError::New(info.Env(), "Wrong number of arguments");
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
  m_instanceId = result["result"]["instanceId"].get<std::string>();
}

void PageContext::appendCompiledStyleSheets(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::appendStyleSheet(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::appendStyleSheetIndex(const Napi::CallbackInfo &info) {
  if (info.Length() < 2) {
    throw Napi::TypeError::New(info.Env(), "Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be a string");
  }
  if (!info[1].IsNumber()) {
    throw Napi::TypeError::New(info.Env(), "Second argument must be a number");
  }
  auto path = info[0].As<Napi::String>().Utf8Value();
  auto id = info[1].As<Napi::Number>().Int32Value();
  nlohmann::json data = {
    path,
    id,
  };
  WebSocket::callDynamicSync(m_instanceId, __FUNCTION__, data);
}

void PageContext::appendStyleSheets(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::attach(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::attachCustomRoute(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::clearStylesheets(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::createElement(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::createFragment(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

Napi::Value PageContext::createStyleSheetIndexGroup(const Napi::CallbackInfo &info) {
  nlohmann::json data;
  auto result = WebSocket::callDynamicSync(m_instanceId, __FUNCTION__, data);
  auto returnValue = result["result"]["returnValue"];
  return Napi::Number::New(info.Env(), returnValue.get<int>());
}

void PageContext::createTextNode(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::finishStyleSheetsCompilation(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::getComputedStyle(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::getHostNode(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::getNodeFromPoint(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::getRootNode(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::getWindowId(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::isTab(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::loadFontFace(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}
/**
 * @param {Array} sheets
 * @return {Array} AsyncStyleSheets(object)
 */
void PageContext::preCompileStyleSheets(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "Wrong number of arguments");
  }
  if (!info[0].IsArray()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be an array");
  }
  Napi::Array sheets = info[0].As<Napi::Array>();
  /**
   * item 结构：
   * - {groupId} number
   * - {path} string
   * - {prefix} string
   * - {styleSheet} object
   */
  /**
   * styleSheet结构：
   * - {binary} boolean
   * - {enabled} boolean
   * - {index} number
   * - {injected} boolean
   * - {path} string
   * - {priority} number
   * - {styleScope} number
   * - {styleScopeSetter} null ???
   */
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::recalcStyle(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::release(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::setAsTab(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::setNavigateBackInterception(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::startRender(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::updateRouteConfig(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

} // namespace Skyline