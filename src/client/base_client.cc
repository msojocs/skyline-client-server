#include "include/base_client.hh"
#include "napi.h"
#include "websocket.hh"
#include <spdlog/spdlog.h>
#include <windows.h>
#include "../include/convert.hh"

namespace Skyline {

  Napi::Value BaseClient::getInstanceId(const Napi::CallbackInfo &info) {
    return Napi::String::New(info.Env(), m_instanceId);
  }
  Napi::Value BaseClient::sendToServerSync(const Napi::CallbackInfo &info, const std::string &methodName) {
    auto env = info.Env();
    nlohmann::json args;
    for (int i = 0; i < info.Length(); i++) {
      args[i] = Convert::convertValue2Json(env, info[i]);
    }
    try {
      auto result = WebSocket::callDynamicSync(m_instanceId, methodName, args);
      auto returnValue = result["returnValue"];
      return Convert::convertJson2Value(env, returnValue);
    } catch (const std::exception &e) {
      throw Napi::Error::New(env, e.what());
    }
    catch (...) {
      throw Napi::Error::New(env, "Unknown error occurred");
    }
  }
  Napi::Value BaseClient::sendToServerAsync(const Napi::CallbackInfo &info, const std::string &methodName) {
    auto env = info.Env();
    nlohmann::json args;
    for (int i = 0; i < info.Length(); i++) {
      args[i] = Convert::convertValue2Json(env, info[i]);
    }
    try {
      WebSocket::callDynamicAsync(m_instanceId, methodName, args);
      return env.Undefined();
    } catch (const std::exception &e) {
      throw Napi::Error::New(env, e.what());
    }
    catch (...) {
      throw Napi::Error::New(env, "Unknown error occurred");
    }
  }

} // namespace SkylineShell