#include "controller.hh"
#include <spdlog/spdlog.h>
#include "../client_action.hh"
#include "../common/logger.hh"
#include "js_native_api_types.h"

using Logger::logger;

namespace HTML {
Napi::FunctionReference *Controller::GetClazz(Napi::Env env) {
  std::vector<Napi::ClassPropertyDescriptor<Controller>> methods;
  methods.push_back(Napi::InstanceWrap<Controller>::InstanceMethod("mount", &Controller::mount));
  methods.push_back(Napi::InstanceWrap<Controller>::InstanceMethod("unmount", &Controller::unmount));
  methods.push_back(Napi::InstanceWrap<Controller>::InstanceAccessor("webview", &Controller::getWebview, nullptr, static_cast<napi_property_attributes>(napi_configurable | napi_writable)));
  methods.push_back(Napi::ObjectWrap<Controller>::StaticMethod("connect", &Controller::connect));

  Napi::Function func = DefineClass(env, "Controller", methods);

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);

  return constructor;
}
Controller::Controller(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<Controller>(info) {
  logger->info("Controller constructor called");
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: Wrong number of arguments");
  }
  if (!info[0].IsFunction()) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: Argument 0 must be a function");
  }
  errorCallbackRef = std::make_shared<Napi::FunctionReference>(Napi::Persistent(info[0].As<Napi::Function>()));
  m_instanceId = sendConstructorToServerSync(info, __func__);
  logger->info("Controller instanceId: {}", m_instanceId);
}
Napi::Value Controller::connect(const Napi::CallbackInfo &info) {
  try {
    auto env = info.Env();
    std::string address = "127.0.0.1";
    int port = 3001;
    if (info.Length() > 0 && !info[0].IsString()) {
      throw Napi::TypeError::New(env, "connect: Argument 0 must be a string");
    }
    if (info.Length() > 1 && !info[1].IsNumber()) {
      throw Napi::TypeError::New(env, "connect: Argument 1 must be a number");
    }
    if (info.Length() > 0) {
      address = info[0].As<Napi::String>().Utf8Value();
    }
    if (info.Length() > 1) {
      port = info[1].As<Napi::Number>().Int32Value();
    }

    ClientAction::initSocket(address, port);
    return env.Undefined();
  } catch (const std::exception &e) {
    logger->error("Error in connect: {}", e.what());
    throw Napi::Error::New(info.Env(), e.what());
  }
  catch (...) {
    logger->error("Unknown error occurred in connect");
    throw Napi::Error::New(info.Env(), "Unknown error occurred");
  }
}
Napi::Value Controller::getWebview(const Napi::CallbackInfo &info) {
  return getProperty(info, "webview");
}
Napi::Value Controller::mount(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value Controller::unmount(const Napi::CallbackInfo  &info) {
  return sendToServerSync(info, __func__);
}
}