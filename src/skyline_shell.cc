#include "skyline_shell.hh"
#include "napi.h"
#include "websocket.hh"
#include <spdlog/spdlog.h>
#include <utility>
#include <windows.h>
#include "skyline_global.hh"
namespace SkylineShell {


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
                      &SkylineShell::setSafeAreaEdgeInsets),
        InstanceMethod("notifyAppLaunch", &SkylineShell::notifyAppLaunch),
        InstanceMethod("onPlatformBrightnessChanged",
                      &SkylineShell::onPlatformBrightnessChanged),
        InstanceMethod("dispatchTouchStartEvent",
                      &SkylineShell::dispatchTouchStartEvent),
        InstanceMethod("dispatchTouchEndEvent",
                      &SkylineShell::dispatchTouchEndEvent),
        InstanceMethod("dispatchTouchMoveEvent",
                      &SkylineShell::dispatchTouchMoveEvent),
        InstanceMethod("dispatchTouchCancelEvent",
                      &SkylineShell::dispatchTouchCancelEvent),
        InstanceMethod("dispatchKeyboardEvent",
                      &SkylineShell::dispatchKeyboardEvent),
                    });
  
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  env.Global().Set(Napi::String::New(env, "SkylineShell"), func);
}

SkylineShell::SkylineShell(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<SkylineShell>(info) {
      spdlog::info("SkylineShell constructor");
  nlohmann::json data;
  auto result = WebSocket::callConstructorSync("SkylineShell", data);
  this->m_instanceId = result["instanceId"].get<std::string>();
}
void SkylineShell::setNotifyBootstrapDoneCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  // 使用指针来存储 callback
  auto callbackPtr = new Napi::FunctionReference();
  *callbackPtr = Napi::Persistent(info[0].As<Napi::Function>());

  auto func1 = Napi::Function::New(env, [callbackPtr](const Napi::CallbackInfo &info) {
    auto env = info.Env();
    try {
      nlohmann::json _t;
      WebSocket::callCustomHandleSync("registerSkylineGlobalClazzRequest", _t);
      SkylineGlobal::Init(env);
      callbackPtr->Value().Call({});
    } catch (const Napi::Error& e) {
      Napi::Error::Fatal("SkylineShell::setNotifyBootstrapDoneCallback", e.Message().c_str());
    }
  });
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::registerCallbackSync(m_instanceId, __func__, func1);
}
void SkylineShell::setNotifyWindowReadyCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  auto func = info[0].As<Napi::Function>();
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::registerCallbackSync(m_instanceId, __func__, func);
}
void SkylineShell::setNotifyRouteDoneCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }

  auto func = info[0].As<Napi::Function>();
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::registerCallbackSync(m_instanceId, __func__, func);
}
void SkylineShell::setNavigateBackCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  auto func = info[0].As<Napi::Function>();
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::registerCallbackSync(m_instanceId, __func__, func);
}
void SkylineShell::setNavigateBackDoneCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  auto func = info[0].As<Napi::Function>();
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::registerCallbackSync(m_instanceId, __func__, func);
}
void SkylineShell::setLoadResourceCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  auto func = info[0].As<Napi::Function>();
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::registerCallbackSync(m_instanceId, __func__, func);
}
void SkylineShell::setLoadResourceAsyncCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  auto func = info[0].As<Napi::Function>();
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::registerCallbackSync(m_instanceId, __func__, func);
}
void SkylineShell::setHttpRequestCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  auto func = info[0].As<Napi::Function>();
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::registerCallbackSync(m_instanceId, __func__, func);
}
void SkylineShell::setSendLogCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsFunction()) {
    throw Napi::Error::New(env, "参数必须为Function类型");
  }
  
  auto func = info[0].As<Napi::Function>();
  
  // 发送消息到 WebSocket
  nlohmann::json data;
  WebSocket::registerCallbackSync(m_instanceId, __func__, func);
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
  WebSocket::callDynamicSync(m_instanceId, __func__, data);
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
  auto windowId = info[0].As<Napi::Number>().Int32Value();
  auto bundlePath = info[1].As<Napi::String>().Utf8Value();
  auto width = info[2].As<Napi::Number>().Int32Value();
  auto height = info[3].As<Napi::Number>().Int32Value();
  auto devicePixelRatio = info[4].As<Napi::Number>().Int32Value();
  auto hideWindow = info[5].As<Napi::Boolean>().Value();
  auto sharedMemoryKey = info[6].As<Napi::String>().Utf8Value();

  // 获取Server端的skyline路径
  nlohmann::json data1;
  auto resp = WebSocket::callCustomHandleSync("getSkylineAddonPath", data1);
  auto skylineAddonPath = resp.get<std::string>();

  // 两个path要替换为server端路径
  nlohmann::json data{
    windowId,
    skylineAddonPath + "\\bundle",
    width,
    height,
    devicePixelRatio,
    hideWindow,
    sharedMemoryKey,
    skylineAddonPath + "\\build\\skyline.node",
  };
  // 发送消息到 WebSocket
  WebSocket::callDynamicSync(m_instanceId, __func__, data);
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
  data[0] = info[0].As<Napi::Number>().Int32Value();
  // 发送消息到 WebSocket
  WebSocket::callDynamicSync(m_instanceId, __func__, data);
}
void SkylineShell::notifyAppLaunch(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 3) {
    throw Napi::Error::New(env, "参数长度必须为3");
  }
  if (!info[0].IsNumber()) {
    throw Napi::Error::New(env, "参数0 windowId必须为Number类型");
  }
  if (!info[1].IsNumber()) {
    throw Napi::Error::New(env, "参数1 pageId必须为Number类型");
  }
  if (!info[2].IsObject()) {
    throw Napi::Error::New(env, "参数2 pageConfig必须为Object类型");
  }
  Sleep(500);
  nlohmann::json data;
  data[0] = info[0].As<Napi::Number>().Int32Value();
  data[1] = info[1].As<Napi::Number>().Int32Value();
  data[2] = {};
  auto pageConfig = info[2].As<Napi::Object>();
  for (auto config: pageConfig) {
    Napi::Value v = config.second;
    auto key = config.first.As<Napi::String>().Utf8Value();
    if (v.IsString()) {
      data[2][key] = v.As<Napi::String>().Utf8Value();
    } else if (v.IsNumber()) {
      data[1] = v.As<Napi::Number>().Int32Value();
    } else if (v.IsBoolean()) {
      data[2][key] = v.As<Napi::Boolean>().Value();
    } else if (v.IsObject()) {
      data[2][key] = nlohmann::json::parse(v.As<Napi::String>().Utf8Value());
    }
  }
  // 发送消息到 WebSocket
  WebSocket::callDynamicSync(m_instanceId, __func__, data);
}
void SkylineShell::onPlatformBrightnessChanged(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsNumber()) {
    throw Napi::Error::New(env, "参数0 brightness必须为Number类型(1:Light;2:Dark)");
  }
  nlohmann::json data;
  data[0] = info[0].As<Napi::Number>().Int32Value();
  // 发送消息到 WebSocket
  WebSocket::callDynamicSync(m_instanceId, __func__, data);
}
void SkylineShell::dispatchTouchStartEvent(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 3) {
    throw Napi::Error::New(env, "参数长度必须为3");
  }
  if (!info[0].IsNumber()) {
    throw Napi::Error::New(env, "参数0 windowId必须为Object类型");
  }
  if (!info[1].IsNumber()) {
    throw Napi::Error::New(env, "参数1 x必须为Number类型");
  }
  if (!info[2].IsNumber()) {
    throw Napi::Error::New(env, "参数2 y必须为Number类型");
  }
  nlohmann::json data;
  data[0] = info[0].As<Napi::Number>().Int32Value();
  data[1] = info[1].As<Napi::Number>().DoubleValue();
  data[2] = info[2].As<Napi::Number>().DoubleValue();
  // 发送消息到 WebSocket
  WebSocket::callDynamicSync(m_instanceId, __func__, data);
}
void SkylineShell::dispatchTouchEndEvent(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 3) {
    throw Napi::Error::New(env, "参数长度必须为3");
  }
  if (!info[0].IsNumber()) {
    throw Napi::Error::New(env, "参数0 windowId必须为Object类型");
  }
  if (!info[1].IsNumber()) {
    throw Napi::Error::New(env, "参数1 x必须为Number类型");
  }
  if (!info[2].IsNumber()) {
    throw Napi::Error::New(env, "参数2 y必须为Number类型");
  }
  nlohmann::json data;
  data[0] = info[0].As<Napi::Number>().Int32Value();
  data[1] = info[1].As<Napi::Number>().DoubleValue();
  data[2] = info[2].As<Napi::Number>().DoubleValue();
  // 发送消息到 WebSocket
  WebSocket::callDynamicSync(m_instanceId, __func__, data);
}
void SkylineShell::dispatchTouchMoveEvent(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 3) {
    throw Napi::Error::New(env, "参数长度必须为3");
  }
  if (!info[0].IsNumber()) {
    throw Napi::Error::New(env, "参数0 windowId必须为Number类型");
  }
  if (!info[1].IsNumber()) {
    throw Napi::Error::New(env, "参数1 x必须为Number类型");
  }
  if (!info[2].IsNumber()) {
    throw Napi::Error::New(env, "参数2 y必须为Number类型");
  }
  nlohmann::json data;
  data[0] = info[0].As<Napi::Number>().Int32Value();
  data[1] = info[1].As<Napi::Number>().DoubleValue();
  data[2] = info[2].As<Napi::Number>().DoubleValue();
  // 发送消息到 WebSocket
  WebSocket::callDynamicSync(m_instanceId, __func__, data);
}
void SkylineShell::dispatchTouchCancelEvent(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 1) {
    throw Napi::Error::New(env, "参数长度必须为1");
  }
  if (!info[0].IsNumber()) {
    throw Napi::Error::New(env, "参数0 windowId必须为Number类型");
  }
  nlohmann::json data;
  data[0] = info[0].As<Napi::Number>().Int32Value();
  // 发送消息到 WebSocket
  WebSocket::callDynamicSync(m_instanceId, __func__, data);
}
void SkylineShell::dispatchKeyboardEvent(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() != 5) {
    throw Napi::Error::New(env, "参数长度必须为5");
  }
  if (!info[0].IsNumber()) {
    throw Napi::Error::New(env, "参数0 windowId必须为Number类型");
  }
  if (!info[1].IsNumber()) {
    throw Napi::Error::New(env, "参数1 key必须为Number类型");
  }
  if (!info[2].IsNumber()) {
    throw Napi::Error::New(env, "参数2 scancode必须为Number类型");
  }
  if (!info[3].IsNumber()) {
    throw Napi::Error::New(env, "参数3 action必须为Number类型");
  }
  if (!info[4].IsNumber()) {
    throw Napi::Error::New(env, "参数4 mods必须为Number类型");
  }
  nlohmann::json data;
  data[0] = info[0].As<Napi::Number>().Int32Value();
  data[1] = info[1].As<Napi::Number>().Int32Value();
  data[2] = info[2].As<Napi::Number>().Int32Value();
  data[3] = info[3].As<Napi::Number>().Int32Value();
  data[4] = info[4].As<Napi::Number>().Int32Value();

  // 发送消息到 WebSocket
  WebSocket::callDynamicSync(m_instanceId, __func__, data);
}
} // namespace SkylineShell