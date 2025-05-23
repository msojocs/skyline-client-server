#include "async_style_sheets.hh"
#include "../socket_client.hh"
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

  Napi::Value AsyncStyleSheets::setScopeId(const Napi::CallbackInfo& info) {
    return sendToServerSync(info, __func__);
  }
}