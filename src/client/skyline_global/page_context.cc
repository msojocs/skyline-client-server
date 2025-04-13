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
Napi::Value PageContext::getInstanceId(const Napi::CallbackInfo& info) {
  return Napi::String::New(info.Env(), m_instanceId);
}
/**
 * 1个Array参数
 */
void PageContext::appendCompiledStyleSheets(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "appendCompiledStyleSheets: Wrong number of arguments");
  }
  if (!info[0].IsArray()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be an array");
  }
  auto env = info.Env();
  // JSON.stringfy
  auto log = info.Env()
                  .Global()
                  .Get("console")
                  .As<Napi::Object>()
                  .Get("log")
                  .As<Napi::Function>();
  log.Call({Napi::String::New(info.Env(), "appendCompiledStyleSheets")});
  auto stringify = info.Env()
                  .Global()
                  .Get("JSON")
                  .As<Napi::Object>()
                  .Get("stringify")
                  .As<Napi::Function>();
  log.Call({stringify.Call({info[0]})});
  nlohmann::json args;
  args[0] = Convert::convertValue2Json(env, info[0]);
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
}

void PageContext::appendStyleSheet(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "Not implemented");
}

void PageContext::appendStyleSheetIndex(const Napi::CallbackInfo &info) {
  if (info.Length() < 2) {
    throw Napi::TypeError::New(info.Env(), "appendStyleSheetIndex: Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be a string");
  }
  if (!info[1].IsNumber()) {
    throw Napi::TypeError::New(info.Env(), "Second argument must be a number");
  }
  auto path = info[0].As<Napi::String>().Utf8Value();
  auto id = info[1].As<Napi::Number>().Int32Value();
  nlohmann::json args;
  args[0] = path;
  args[1] = id;
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
}
/**
 * 参数数量：1个
 * 参数1：Array
 */
void PageContext::appendStyleSheets(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "appendStyleSheets: Wrong number of arguments");
  }
  if (!info[0].IsArray()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be an array");
  }
  
  auto env = info.Env();
  /**
   * [
   *  {
   *   content: '',
   *   scopeId: 0,
   *  }
   * ]
   */
  nlohmann::json args;
  Napi::Array sheets = info[0].As<Napi::Array>();
  args[0] = Convert::convertValue2Json(env, sheets);
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
  // 无返回值
}

/**
 * 2个参数
 */
Napi::Value PageContext::attach(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  auto env = info.Env();
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}

void PageContext::attachCustomRoute(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "attachCustomRoute: Not implemented");
}

void PageContext::clearStylesheets(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "clearStylesheets: Not implemented");
}

Napi::Value PageContext::createElement(const Napi::CallbackInfo &info) {
  if (info.Length() < 2) {
    throw Napi::TypeError::New(info.Env(), "createElement: Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be a string");
  }
  if (!info[1].IsString()) {
    throw Napi::TypeError::New(info.Env(), "Second argument must be a string");
  }
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}

Napi::Value PageContext::createFragment(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  auto env = info.Env();
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}

Napi::Value PageContext::createStyleSheetIndexGroup(const Napi::CallbackInfo &info) {
  nlohmann::json data;
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, data);
  auto returnValue = result["returnValue"];
  return Napi::Number::New(info.Env(), returnValue.get<int>());
}

Napi::Value PageContext::createTextNode(const Napi::CallbackInfo &info) {
  nlohmann::json args;
  auto env = info.Env();
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}

void PageContext::finishStyleSheetsCompilation(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "finishStyleSheetsCompilation: Not implemented");
}

void PageContext::getComputedStyle(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "getComputedStyle: Not implemented");
}

void PageContext::getHostNode(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "getHostNode: Not implemented");
}

void PageContext::getNodeFromPoint(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "getNodeFromPoint: Not implemented");
}

Napi::Value PageContext::getRootNode(const Napi::CallbackInfo &info) {
  
  auto env = info.Env();
  nlohmann::json args;
  for (int i = 0; i < info.Length(); i++) {
    args[i] = Convert::convertValue2Json(env, info[i]);
  }
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];
  return Convert::convertJson2Value(env, returnValue);
}

void PageContext::getWindowId(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "getWindowId: Not implemented");
}

void PageContext::isTab(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "isTab: Not implemented");
}

void PageContext::loadFontFace(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "loadFontFace: Not implemented");
}
/**
 * @param {Array} sheets
 * @return {Array} AsyncStyleSheets(object)
 */
Napi::Value PageContext::preCompileStyleSheets(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "preCompileStyleSheets: Wrong number of arguments");
  }
  if (!info[0].IsArray()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be an array");
  }
  auto env = info.Env();
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
  nlohmann::json args;
  args[0] = Convert::convertValue2Json(env, sheets);
  auto result = WebSocket::callDynamicSync(m_instanceId, __func__, args);
  auto returnValue = result["returnValue"];

  return Convert::convertJson2Value(env, returnValue);
}

void PageContext::recalcStyle(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "recalcStyle: Not implemented");
}

void PageContext::release(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "release: Not implemented");
}

void PageContext::setAsTab(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "setAsTab: Not implemented");
}

void PageContext::setNavigateBackInterception(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "setNavigateBackInterception: Not implemented");
}

/**
 * 1个Callback参数
 */
void PageContext::startRender(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "startRender: Wrong number of arguments");
  }
  if (!info[0].IsFunction()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be a function");
  } 
  // 发送消息到 WebSocket
  nlohmann::json args;
  auto env = info.Env();
  args[0] = Convert::convertValue2Json(env, info[0]);
  WebSocket::callDynamicSync(m_instanceId, __func__, args);
}

void PageContext::updateRouteConfig(const Napi::CallbackInfo &info) {
  throw Napi::Error::New(info.Env(), "updateRouteConfig: Not implemented");
}

} // namespace Skyline