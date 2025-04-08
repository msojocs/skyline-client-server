#include "skyline_shell.hh"
#include "napi.h"
#include "websocket.hh"
#include <spdlog/spdlog.h>
#include <utility>
namespace SkylineShell {

static std::map<std::string, Napi::ThreadSafeFunction> callback;

void SkylineShell::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
      env, "SkylineShell",
      {InstanceMethod("setNotifyBootstrapDoneCallback",
                      &SkylineShell::setNotifyBootstrapDoneCallback),
       InstanceMethod("setNotifyWindowReadyCallback",
                      &SkylineShell::setNotifyWindowReadyCallback),
       InstanceMethod("setNotifyRouteDoneCallback",
                      &SkylineShell::setNotifyRouteDoneCallback),
       InstanceMethod("setNavigateBackCallback",
                      &SkylineShell::setNavigateBackCallback),
       InstanceMethod("setNavigateBackDoneCallback",
                      &SkylineShell::setNavigateBackDoneCallback),
       InstanceMethod("setLoadResourceCallback",
                      &SkylineShell::setLoadResourceCallback),
       InstanceMethod("setLoadResourceAsyncCallback",
                      &SkylineShell::setLoadResourceAsyncCallback),
       InstanceMethod("setHttpRequestCallback",
                      &SkylineShell::setHttpRequestCallback),
       InstanceMethod("setSendLogCallback", &SkylineShell::setSendLogCallback),
       InstanceMethod("createWindow", &SkylineShell::createWindow),
       InstanceMethod("destroyWindow", &SkylineShell::destroyWindow),
       InstanceMethod("setSafeAreaEdgeInsets",
                      &SkylineShell::setSafeAreaEdgeInsets)});
  
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  env.Global().Set(Napi::String::New(env, "SkylineShell"), func);
}
void SkylineShell::DispatchCallback(std::string &action, nlohmann::json &data) {
  auto it = callback.find(action);
  if (it != callback.end()) {
    // 在正确的线程上调用回调
    it->second.BlockingCall([data](Napi::Env env, Napi::Function jsCallback) {
      Napi::HandleScope scope(env);
      Napi::Value jsData = Napi::String::New(env, data.dump());
      jsCallback.Call({jsData});
    });
  } else {
    spdlog::info("SkylineShell: {} not found", action);
  }
}

SkylineShell::SkylineShell(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<SkylineShell>(info) {
      spdlog::info("SkylineShell constructor");
  callback.clear();
  nlohmann::json data;
  WebSocket::sendMessageSync("SkylineShell", "constructor", data);
}
void SkylineShell::setNotifyBootstrapDoneCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  // 使用ThreadSafeFunction保存回调函数
  Napi::ThreadSafeFunction tsfn = Napi::ThreadSafeFunction::New(
      env,
      info[0].As<Napi::Function>(),
      "SkylineShell Callback",
      0,
      1);
  
  callback["setNotifyBootstrapDoneCallback"] = tsfn;
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::sendMessageSync("SkylineShell", "setNotifyBootstrapDoneCallback", data);
}
void SkylineShell::setNotifyWindowReadyCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  // 使用ThreadSafeFunction保存回调函数
  Napi::ThreadSafeFunction tsfn = Napi::ThreadSafeFunction::New(
      env,
      info[0].As<Napi::Function>(),
      "SkylineShell Callback",
      0,
      1);
  
  callback["setNotifyWindowReadyCallback"] = tsfn;
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::sendMessageSync("SkylineShell", "setNotifyWindowReadyCallback", data);
}
void SkylineShell::setNotifyRouteDoneCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  // 使用ThreadSafeFunction保存回调函数
  Napi::ThreadSafeFunction tsfn = Napi::ThreadSafeFunction::New(
      env,
      info[0].As<Napi::Function>(),
      "SkylineShell Callback",
      0,
      1);
  
  callback["setNotifyRouteDoneCallback"] = tsfn;
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::sendMessageSync("SkylineShell", "setNotifyRouteDoneCallback", data);
}
void SkylineShell::setNavigateBackCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  // 使用ThreadSafeFunction保存回调函数
  Napi::ThreadSafeFunction tsfn = Napi::ThreadSafeFunction::New(
      env,
      info[0].As<Napi::Function>(),
      "SkylineShell Callback",
      0,
      1);
  
  callback["setNavigateBackCallback"] = tsfn;
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::sendMessageSync("SkylineShell", "setNavigateBackCallback", data);
}
void SkylineShell::setNavigateBackDoneCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  // 使用ThreadSafeFunction保存回调函数
  Napi::ThreadSafeFunction tsfn = Napi::ThreadSafeFunction::New(
      env,
      info[0].As<Napi::Function>(),
      "SkylineShell Callback",
      0,
      1);
  
  callback["setNavigateBackDoneCallback"] = tsfn;
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::sendMessageSync("SkylineShell", "setNavigateBackDoneCallback", data);
}
void SkylineShell::setLoadResourceCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  // 使用ThreadSafeFunction保存回调函数
  Napi::ThreadSafeFunction tsfn = Napi::ThreadSafeFunction::New(
      env,
      info[0].As<Napi::Function>(),
      "SkylineShell Callback",
      0,
      1);
  
  callback["setLoadResourceCallback"] = tsfn;
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::sendMessageSync("SkylineShell", "setLoadResourceCallback", data);
}
void SkylineShell::setLoadResourceAsyncCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  // 使用ThreadSafeFunction保存回调函数
  Napi::ThreadSafeFunction tsfn = Napi::ThreadSafeFunction::New(
      env,
      info[0].As<Napi::Function>(),
      "SkylineShell Callback",
      0,
      1);
  
  callback["setLoadResourceAsyncCallback"] = tsfn;
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::sendMessageSync("SkylineShell", "setLoadResourceAsyncCallback", data);
}
void SkylineShell::setHttpRequestCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  // 使用ThreadSafeFunction保存回调函数
  Napi::ThreadSafeFunction tsfn = Napi::ThreadSafeFunction::New(
      env,
      info[0].As<Napi::Function>(),
      "SkylineShell Callback",
      0,
      1);
  
  callback["setHttpRequestCallback"] = tsfn;
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::sendMessageSync("SkylineShell", "setHttpRequestCallback", data);
}
void SkylineShell::setSendLogCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  // 使用ThreadSafeFunction保存回调函数
  Napi::ThreadSafeFunction tsfn = Napi::ThreadSafeFunction::New(
      env,
      info[0].As<Napi::Function>(),
      "SkylineShell Callback",
      0,
      1);
  
  callback["setSendLogCallback"] = tsfn;
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::sendMessageSync("SkylineShell", "setSendLogCallback", data);
}
void SkylineShell::setSafeAreaEdgeInsets(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 4) {
    throw Napi::Error::New(env, "参数长度必须为4");
  }
  if (!info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber() ||
      !info[3].IsNumber()) {
    throw Napi::Error::New(env, "所有参数必须都为Number类型");
  }
  nlohmann::json data;
  data[0] = info[0].As<Napi::Number>().Int32Value();
  data[1] = info[1].As<Napi::Number>().Int32Value();
  data[2] = info[2].As<Napi::Number>().Int32Value();
  data[3] = info[3].As<Napi::Number>().Int32Value();
  // 发送消息到 WebSocket
  WebSocket::sendMessageSync("SkylineShell", "setSafeAreaEdgeInsets", data);
}
/**
 * createWindow(
        window_id: number
        , bundle_path: string
        , width: number
        , height: number
        , device_pixel_ratio: number
        , hide_window: boolean
        , shared_memory_key: string
        , skyline_addon_path: string
    )
 *
 */
void SkylineShell::createWindow(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 8) {
    throw Napi::Error::New(env, "参数长度必须为8");
  }
  if (!info[0].IsNumber()) {
    throw Napi::Error::New(env, "参数0 windowId必须为Number类型");
  }
  if (!info[1].IsString()) {
    throw Napi::Error::New(env, "参数1 bundlePath必须为String类型");
  }
  if (!info[2].IsNumber()) {
    throw Napi::Error::New(env, "参数2 width必须为Number类型");
  }
  if (!info[3].IsNumber()) {
    throw Napi::Error::New(env, "参数3 height必须为Number类型");
  }
  if (!info[4].IsNumber()) {
    throw Napi::Error::New(env, "参数4 devicePixelRatio必须为Number类型");
  }
  if (!info[5].IsBoolean()) {
    throw Napi::Error::New(env, "参数5 hideWindow必须为Boolean类型");
  }
  if (!info[6].IsString()) {
    throw Napi::Error::New(env, "参数6 sharedMemoryKey必须为String类型");
  }
  if (!info[7].IsString()) {
    throw Napi::Error::New(env, "参数7 skylineAddonPath必须为String类型");
  }
  nlohmann::json data;
  data["windowId"] = info[0].As<Napi::Number>().Int32Value();
  data["bundlePath"] = info[1].As<Napi::String>().Utf8Value();
  data["width"] = info[2].As<Napi::Number>().Int32Value();
  data["height"] = info[3].As<Napi::Number>().Int32Value();
  data["devicePixelRatio"] = info[4].As<Napi::Number>().Int32Value();
  data["hideWindow"] = info[5].As<Napi::Boolean>().Value();
  data["sharedMemoryKey"] = info[6].As<Napi::String>().Utf8Value();
  data["skylineAddonPath"] = info[7].As<Napi::String>().Utf8Value();
  // 发送消息到 WebSocket
  WebSocket::sendMessageSync("SkylineShell", "createWindow", data);
}
void SkylineShell::destroyWindow(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsNumber()) {
    throw Napi::Error::New(env, "参数0 windowId必须为Number类型");
  }
  nlohmann::json data;
  data["windowId"] = info[0].As<Napi::Number>().Int32Value();
  // 发送消息到 WebSocket
  WebSocket::sendMessageSync("SkylineShell", "destroyWindow", data);
}
} // namespace SkylineShell