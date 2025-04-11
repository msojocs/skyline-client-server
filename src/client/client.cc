#include <spdlog/spdlog.h>
#include <napi.h>
#include <sys/types.h>
#include "skyline_shell.hh"
#include "websocket.hh"
#include "skyline_debug_info.hh"
#include "../include/logger.hh"
#include "../include/convert.hh"

using Logger::logger;


static Napi::Object Init(Napi::Env env, Napi::Object exports) {
  Logger::Init();
  logger->info("initWebSocket start");
  WebSocket::initWebSocket();

  logger->info("initWebSocket end");
  SkylineDebugInfo::InitSkylineDebugInfo(env, exports);
  SkylineShell::SkylineShell::Init(env, exports);
  Convert::RegisteInstanceType(env);
  // exports.Set("setConsole", Napi::Function::New(env, Logger::set_console));
  logger->info("return result");
  return exports;
}

NODE_API_MODULE(cmnative, Init)