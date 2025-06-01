#include "skyline_shell.hh"
#include "napi.h"
#include "socket_client.hh"
#include <cstdint>
#include <cstring>
#include <exception>
#include <spdlog/spdlog.h>
#include "skyline_global.hh"
#include "../common/protobuf_converter.hh"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace SkylineShell {


void SkylineShell::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(
      env, "SkylineShell",
  {
      InstanceMethod("createWindow", &SkylineShell::createWindow),
      InstanceMethod("destroyWindow", &SkylineShell::destroyWindow),
      InstanceMethod("dispatchCharEvent", &SkylineShell::dispatchCharEvent),
      InstanceMethod("dispatchKeyboardEvent", &SkylineShell::dispatchKeyboardEvent),
      InstanceMethod("dispatchTouchCancelEvent", &SkylineShell::dispatchTouchCancelEvent),
      InstanceMethod("dispatchTouchEndEvent", &SkylineShell::dispatchTouchEndEvent),
      InstanceMethod("dispatchTouchMoveEvent", &SkylineShell::dispatchTouchMoveEvent),
      InstanceMethod("dispatchTouchStartEvent", &SkylineShell::dispatchTouchStartEvent),
      InstanceMethod("dispatchWheelEvent", &SkylineShell::dispatchWheelEvent),
        // dispatchTouchOverEvent
      InstanceMethod("dispatchTouchOverEvent", &SkylineShell::dispatchTouchOverEvent),
      InstanceMethod("notifyAppLaunch", &SkylineShell::notifyAppLaunch),
      InstanceMethod("notifyResourceLoad", &SkylineShell::notifyResourceLoad),
      InstanceMethod("notifySwitchTab", &SkylineShell::notifySwitchTab),
      InstanceMethod("notifyNavigateTo", &SkylineShell::notifyNavigateTo),
        // notifyHttpRequestComplete
      InstanceMethod("notifyHttpRequestComplete", &SkylineShell::notifyHttpRequestComplete),
      InstanceMethod("onPlatformBrightnessChanged", &SkylineShell::onPlatformBrightnessChanged),
      InstanceMethod("setNotifyBootstrapDoneCallback", &SkylineShell::setNotifyBootstrapDoneCallback),
      InstanceMethod("setNotifyWindowReadyCallback", &SkylineShell::setNotifyWindowReadyCallback),
      InstanceMethod("setNotifyRouteDoneCallback", &SkylineShell::setNotifyRouteDoneCallback),
      InstanceMethod("setNavigateBackCallback", &SkylineShell::setNavigateBackCallback),
      InstanceMethod("setNavigateBackDoneCallback", &SkylineShell::setNavigateBackDoneCallback),
      InstanceMethod("setLoadResourceCallback", &SkylineShell::setLoadResourceCallback),
      InstanceMethod("setLoadResourceAsyncCallback", &SkylineShell::setLoadResourceAsyncCallback),
      InstanceMethod("setHttpRequestCallback", &SkylineShell::setHttpRequestCallback),
      InstanceMethod("setSafeAreaEdgeInsets", &SkylineShell::setSafeAreaEdgeInsets),
      InstanceMethod("setSendLogCallback", &SkylineShell::setSendLogCallback),
  });
  
  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  env.Global().Set(Napi::String::New(env, "SkylineShell"), func);
}

SkylineShell::SkylineShell(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<SkylineShell>(info) {
      auto env = info.Env();
      if (!env.Global().Has("__sharedMemory")) {
        throw Napi::Error::New(env, "共享内存模块未加载！");
      }
  m_instanceId = sendConstructorToServerSync(info, __func__);
}

void SkylineShell::setNotifyBootstrapDoneCallback(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  try {
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
      auto env = info.Env();      try {        std::vector<skyline::Value> _t;
        SocketClient::callCustomHandleSync("registerSkylineGlobalClazzRequest", _t);
        //* 客户端初始化SkylineGlobal
        SkylineGlobal::Init(env);
        callbackPtr->Value().Call({});
      } catch (const Napi::Error& e) {
        throw e;
      } catch (const std::exception &e) {
        throw Napi::Error::New(env, e.what());
      } catch (...) {
        throw Napi::Error::New(env, "Unknown error occurred");
      }
    });    // 发送消息到Socket
    std::vector<skyline::Value> args;
    args.push_back(ProtobufConverter::napiToProtobufValue(env, func1));
    SocketClient::callDynamicSync(m_instanceId, __func__, args);
  } catch (const std::exception &e) {
    throw Napi::Error::New(env, e.what());
  }
  catch (...) {
    throw Napi::Error::New(env, "Unknown error occurred");
  }
}
void SkylineShell::setNotifyWindowReadyCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void SkylineShell::setNotifyRouteDoneCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void SkylineShell::setNavigateBackCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void SkylineShell::setNavigateBackDoneCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
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
  //* 标记该回调使用同步方式调用
  func.Set(Napi::String::New(env, "__syncCallback"), Napi::Boolean::New(env, true));

  // 发送消息到Socket
  sendToServerSync(info, __func__);
}
void SkylineShell::setLoadResourceAsyncCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void SkylineShell::setHttpRequestCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void SkylineShell::setSendLogCallback(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void SkylineShell::setSafeAreaEdgeInsets(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
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
  try {
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
    if (!info[6].IsArrayBuffer()) {
      throw Napi::Error::New(env, "参数6 sharedMemoryKey必须为ArrayBuffer类型");
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
    auto sharedMemory = info[6].As<Napi::ArrayBuffer>();
    char * addr = (char *)sharedMemory.Data();
    std::string sharedMemoryKey;
    if (addr[0] == 'k' && addr[1] == 'e' && addr[2] == 'y' && addr[3] == ':') {
      // 共享内存key
      sharedMemoryKey = std::string(addr + 4);
    } else {
      throw Napi::Error::New(env, "共享内存key错误");
    }    // 获取Server端的skyline路径
    std::vector<skyline::Value> data1;
    auto resp = SocketClient::callCustomHandleSync("getSkylineAddonPath", data1);
    
    // Extract string value from the protobuf response
    std::string skylineAddonPath;
    if (resp.has_string_value()) {
        skylineAddonPath = resp.string_value();
    } else {
        throw Napi::Error::New(env, "Failed to get skyline addon path");
    }// 两个path要替换为server端路径
    std::vector<skyline::Value> data;
      // Convert each parameter to skyline::Value
    skyline::Value windowIdValue;
    windowIdValue.set_int_value(windowId);
    data.push_back(windowIdValue);
    
    skyline::Value bundlePathValue;
    bundlePathValue.set_string_value(skylineAddonPath + "\\bundle");
    data.push_back(bundlePathValue);
    
    skyline::Value widthValue;
    widthValue.set_int_value(width);
    data.push_back(widthValue);
    
    skyline::Value heightValue;
    heightValue.set_int_value(height);
    data.push_back(heightValue);
    
    skyline::Value devicePixelRatioValue;
    devicePixelRatioValue.set_double_value(devicePixelRatio);
    data.push_back(devicePixelRatioValue);
    
    skyline::Value hideWindowValue;
    hideWindowValue.set_bool_value(hideWindow);
    data.push_back(hideWindowValue);
    
    skyline::Value sharedMemoryKeyValue;
    sharedMemoryKeyValue.set_string_value(sharedMemoryKey);
    data.push_back(sharedMemoryKeyValue);
    
    skyline::Value nodePathValue;
    nodePathValue.set_string_value(skylineAddonPath + "\\build\\skyline.node");
    data.push_back(nodePathValue);
    
    // 发送消息到Socket
    SocketClient::callDynamicSync(m_instanceId, __func__, data);
  } catch (const std::exception &e) {
    throw Napi::Error::New(env, e.what());
  }
  catch (...) {
    throw Napi::Error::New(env, "Unknown error occurred");
  }
}
void SkylineShell::destroyWindow(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void SkylineShell::notifyAppLaunch(const Napi::CallbackInfo &info) {
  #ifdef _WIN32
  Sleep(500);
  #else
  usleep(500000);
  #endif
  sendToServerSync(info, __func__);
}
void SkylineShell::onPlatformBrightnessChanged(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void SkylineShell::dispatchTouchStartEvent(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void SkylineShell::dispatchTouchEndEvent(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void SkylineShell::dispatchTouchMoveEvent(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
void SkylineShell::dispatchTouchCancelEvent(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
Napi::Value SkylineShell::dispatchKeyboardEvent(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
void SkylineShell::dispatchWheelEvent(const Napi::CallbackInfo &info) {
  sendToServerSync(info, __func__);
}
Napi:: Value SkylineShell::notifyHttpRequestComplete(const Napi::CallbackInfo &info) {
  auto env = info.Env();
  if (!env.Global().Has("__sharedMemory")) {
    throw Napi::Error::New(env, "共享内存模块未加载！");
  }
  try {
    std::vector<skyline::Value> args;
    
    // Convert first 4 parameters to skyline::Value
    for (int i = 0; i < 4; i++) {
      args.push_back(ProtobufConverter::napiToProtobufValue(env, info[i]));
    }
    
    auto sharedMemory = env.Global().Get("__sharedMemory").As<Napi::Object>();
    std::string key = "resource_" + std::to_string(info[0].As<Napi::Number>().Int32Value());
    
    // Add the key as 5th parameter
    skyline::Value keyValue;
    keyValue.set_string_value(key);
    args.push_back(keyValue);
    
    auto buffer = info[4].As<Napi::Buffer<uint8_t>>();
    auto mem = sharedMemory.Get("setMemory").As<Napi::Function>().Call({
      Napi::String::New(env, key),
      Napi::Number::New(env, buffer.Length())
    }).As<Napi::ArrayBuffer>();
    // Uint8Array包裹
    Napi::TypedArrayOf<uint8_t> typedArray = Napi::TypedArrayOf<uint8_t>::New(env, buffer.Length(), mem, 0);
    // 写入数据
    memcpy(typedArray.Data(), buffer.Data(), buffer.Length());
    
    SocketClient::callDynamicSync(m_instanceId, __func__, args);
    return env.Undefined();
  } catch (const std::exception& e) {
    throw Napi::Error::New(env, e.what());
  }
  return env.Undefined();
}
Napi:: Value SkylineShell::dispatchTouchOverEvent(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value SkylineShell::notifyResourceLoad(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value SkylineShell::notifyNavigateTo(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value SkylineShell::dispatchCharEvent(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
Napi::Value SkylineShell::notifySwitchTab(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
} // namespace SkylineShell