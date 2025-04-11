#include "include/base_client.hh"
#include "napi.h"
#include "websocket.hh"
#include <spdlog/spdlog.h>
#include <windows.h>
namespace Skyline {

  Napi::Value BaseClient::getInstanceId(const Napi::CallbackInfo &info) {
    return info.Env().Undefined();
  }

} // namespace SkylineShell