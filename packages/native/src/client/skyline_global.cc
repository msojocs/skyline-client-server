#include "skyline_global.hh"
#include "skyline_global/page_context.hh"
#include "skyline_global/runtime.hh"
#include "skyline_global/worklet_module.hh"
#include "skyline_global/gesture_handler_module.hh"
#include "napi.h"
#include "socket_client.hh"
#include "../common/convert.hh"

namespace SkylineGlobal {
  void Init(Napi::Env env) {
    try {
      auto skylineGlobal = Napi::Object::New(env);
      {
        nlohmann::json arg;
        auto result = SocketClient::callStaticSync("SkylineGlobal", "userAgent", arg);
        auto returnValue = result["returnValue"];
        skylineGlobal.Set(Napi::String::New(env, "userAgent"), Napi::String::New(env, returnValue.get<std::string>()));
      }
      {
        nlohmann::json arg;
        auto result = SocketClient::callStaticSync("SkylineGlobal", "features", arg);
        auto returnValue = result["returnValue"];
        skylineGlobal.Set(Napi::String::New(env, "features"), Convert::convertJson2Value(env, returnValue));
      }
      {
        Skyline::PageContext::Init(env, skylineGlobal);
      }
      {
        Skyline::Runtime::Init(env, skylineGlobal);
      }
      {
        auto workletModule = Napi::Object::New(env);
        Skyline::WorkletModule::Init(env, workletModule);
        skylineGlobal.Set(Napi::String::New(env, "workletModule"), workletModule);
      }
      {
        auto gestureModule = Napi::Object::New(env);
        Skyline::GestureHandlerModule::Init(env, gestureModule);
        skylineGlobal.Set(Napi::String::New(env, "gestureHandlerModule"), gestureModule);
      }
      env.Global().Set(Napi::String::New(env, "SkylineGlobal"), skylineGlobal);
    }
    catch (const std::exception& e) {
        throw Napi::Error::New(env, e.what());
    } catch (...) {
        throw Napi::Error::New(env, "Unknown error occurred during SkylineDebugInfo.");
    }
  }
}
