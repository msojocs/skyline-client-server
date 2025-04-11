#include "../include/async_style_sheets.hh"
#include "../websocket.hh"
#include "napi.h"

namespace Skyline {
  Napi::FunctionReference* AsyncStyleSheets::GetClazz(Napi::Env env) {

    Napi::Function func = DefineClass(
        env, "AsyncStyleSheets",
        {InstanceMethod("setScopeId", &AsyncStyleSheets::setScopeId)});
    Napi::FunctionReference *constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);

    return constructor;
  }

  AsyncStyleSheets::AsyncStyleSheets(const Napi::CallbackInfo& info)
      : Napi::ObjectWrap<AsyncStyleSheets>(info) {
    if (info.Length() < 1) {
      throw Napi::TypeError::New(info.Env(), "Constructor: Wrong number of arguments");
    }
    if (!info[0].IsString()) {
      throw Napi::TypeError::New(info.Env(), "Constructor: First argument must be a string");
    }
    m_instanceId = info[0].As<Napi::String>().Utf8Value();
  }

  Napi::Value AsyncStyleSheets::getInstanceId(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), m_instanceId);
  }

  void AsyncStyleSheets::setScopeId(const Napi::CallbackInfo& info) {
    if (info.Length() < 1) {
      throw Napi::TypeError::New(info.Env(), "setScopeId: Wrong number of arguments");
    }
    if (!info[0].IsNumber()) {
      throw Napi::TypeError::New(info.Env(), "setScopeId: First argument must be a number");
    }
    auto scopeId = info[0].As<Napi::Number>().Int32Value();
    nlohmann::json args;
    args[0] = scopeId;
    WebSocket::callDynamicSync(m_instanceId, __func__, args);
  }
}