#include "../include/view_shadow_node.hh"
#include "../../include/convert.hh"
#include "../websocket.hh"
#include "napi.h"

namespace Skyline {
  Napi::FunctionReference* ViewShadowNode::GetClazz(Napi::Env env) {

    Napi::Function func = DefineClass(
        env, "ViewShadowNode",
        {
            InstanceMethod("setStyleScope", &ViewShadowNode::setStyleScope),
            InstanceMethod("addClass", &ViewShadowNode::addClass),
            InstanceMethod("setStyle", &ViewShadowNode::setStyle),
            InstanceMethod("setEventDefaultPrevented", &ViewShadowNode::setEventDefaultPrevented),
            InstanceMethod("appendChild", &ViewShadowNode::appendChild),
            // getter instanceId
            // InstanceAccessor<&ViewShadowNode::getInstanceId>("instanceId", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
          }
      );
    Napi::FunctionReference *constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);

    return constructor;
  }
  ViewShadowNode::ViewShadowNode(const Napi::CallbackInfo& info)
      : Napi::ObjectWrap<ViewShadowNode>(info) {
    if (info.Length() < 1) {
      throw Napi::TypeError::New(info.Env(), "Constructor: Wrong number of arguments");
    }
    if (!info[0].IsString()) {
      throw Napi::TypeError::New(info.Env(), "Constructor: First argument must be a string");
    }
    m_instanceId = info[0].As<Napi::String>().Utf8Value();
  }
  Napi::Value ViewShadowNode::getInstanceId(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), m_instanceId);
  }
  Napi::Value ViewShadowNode::setStyleScope(const Napi::CallbackInfo& info) {
    if (info.Length() < 2) {
      throw Napi::TypeError::New(info.Env(), "setStyleScope: Wrong number of arguments");
    }
    nlohmann::json args;
    args[0] = Convert::convertValue2Json(info[0]);
    args[1] = Convert::convertValue2Json(info[1]);
    WebSocket::callDynamicSync(m_instanceId, __func__, args);
    return info.Env().Undefined();
  }
  Napi::Value ViewShadowNode::addClass(const Napi::CallbackInfo& info) {
    nlohmann::json args;
    for (int i = 0; i < info.Length(); i++) {
      args[i] = Convert::convertValue2Json(info[i]);
    }
    WebSocket::callDynamicSync(m_instanceId, __func__, args);
    return info.Env().Undefined();
  }
  Napi::Value ViewShadowNode::setStyle(const Napi::CallbackInfo& info) {
    nlohmann::json args;
    for (int i = 0; i < info.Length(); i++) {
      args[i] = Convert::convertValue2Json(info[i]);
    }
    WebSocket::callDynamicSync(m_instanceId, __func__, args);
    return info.Env().Undefined();
  }
  Napi::Value ViewShadowNode::setEventDefaultPrevented(const Napi::CallbackInfo& info) {
    nlohmann::json args;
    for (int i = 0; i < info.Length(); i++) {
      args[i] = Convert::convertValue2Json(info[i]);
    }
    WebSocket::callDynamicSync(m_instanceId, __func__, args);
    return info.Env().Undefined();
  }
  Napi::Value ViewShadowNode::appendChild(const Napi::CallbackInfo& info) {
    try {
      nlohmann::json args;
      
      // 输出调试信息
      auto log = info.Env().Global().Get("console").As<Napi::Object>().Get("log").As<Napi::Function>();
      log.Call(info.Env().Global(), { Napi::String::New(info.Env(), "Appending child...") });

      log.Call(info.Env().Global(), { Napi::String::New(info.Env(), "Arguments length: " + std::to_string(info.Length())) });
      for (int i = 0; i < info.Length(); i++) {
        log.Call(info.Env().Global(), { Napi::String::New(info.Env(), std::string("Convert argument: ") + std::to_string(i)) });
        log.Call(info.Env().Global(), { info[i] });
        // info[i].As<Napi::Object>();
        // args[i] = Convert::convertValue2Json(info[i]);
      }
      log.Call(info.Env().Global(), { Napi::String::New(info.Env(), "Calling WebSocket...") });
      // WebSocket::callDynamicSync(m_instanceId, __func__, args);
      return info.Env().Undefined();
    } catch (const Napi::Error &e) {
      Napi::Error::New(info.Env(),
                       std::string("Error in ") + __FUNCTION__ + ": " + e.Message())
          .ThrowAsJavaScriptException();
      return info.Env().Undefined();
    } catch (const std::exception &e) {

      Napi::Error::New(info.Env(),
                       std::string("Error in ") + __FUNCTION__ + ": " + e.what())
          .ThrowAsJavaScriptException();
      return info.Env().Undefined();
    }
    catch (...) {
      Napi::Error::New(info.Env(),
                       std::string("Unknown error in ") + __FUNCTION__)
          .ThrowAsJavaScriptException();
      return info.Env().Undefined();
    }
  }

}