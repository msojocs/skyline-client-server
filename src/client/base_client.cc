#include "include/base_client.hh"
#include "napi.h"
#include "websocket.hh"
#include <spdlog/spdlog.h>
#include <windows.h>
namespace Skyline {

  Napi::Value BaseClient::getInstanceId(const Napi::CallbackInfo &info) {
    return Napi::String::New(info.Env(), m_instanceId);
  }

} // namespace SkylineShell