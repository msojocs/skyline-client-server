#include <spdlog/spdlog.h>
#include <napi.h>
#include <sys/types.h>
#include "skyline_shell.hh"
#include "client_action.hh"
#include "skyline_debug_info.hh"
#include "../common/logger.hh"
#include "../common/convert.hh"
#include "crash_handler.hh"

using Logger::logger;


static Napi::Object Init(Napi::Env env, Napi::Object exports) {
  try {
    // 初始化崩溃处理器
    #ifdef _WIN32
    CrashHandler::init();
    #endif
    
    // 初始化日志
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Starting Skyline Client...");
    
    Logger::Init();    logger->info("initSocket start");
    ClientAction::initSocket(env);
    logger->info("initSocket end");
    SkylineDebugInfo::InitSkylineDebugInfo(env, exports);
    SkylineShell::SkylineShell::Init(env, exports);
    Convert::RegisteInstanceType(env);
    // exports.Set("setConsole", Napi::Function::New(env, Logger::set_console));
    logger->info("return result");
  }catch (const std::exception& e) {
    logger->error("Error: {}", e.what());
    throw Napi::Error::New(env, e.what());
  }catch (...) {
    logger->error("Unknown error occurred during initialization.");
    throw Napi::Error::New(env, "Unknown error occurred during initialization.");
  }
  return exports;
}

NODE_API_MODULE(cmnative, Init)