#include <spdlog/spdlog.h>
#include <napi.h>
#include <sys/types.h>
#include "skyline_shell.hh"
#include "client_action.hh"
#include "skyline_debug_info.hh"
#include "http_client.hh"
#include "../common/logger.hh"
#include "../common/convert.hh"
#include "crash_handler.hh"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

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
    Logger::Init();

    auto log = env.Global().Get("console").As<Napi::Object>().Get("log").As<Napi::Function>();

    // 检查服务器状态并执行重启流程
    // 执行destroy操作
    log.Call({Napi::String::New(env, "正在停止Skyline服务器...")});
    std::string destroy_result = HttpClient::sendHttpRequest("/destroy", "destroy");

    if (destroy_result == "ok") {
        logger->info("服务器停止成功");
        log.Call({Napi::String::New(env, "Skyline服务器停止: ok")});
    } else {
        logger->warn("服务器停止失败. 响应: '{}'", destroy_result);
        throw std::runtime_error("Skyline服务器停止失败: " + destroy_result);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // TODO: Memory先初始化再启动，Socket先启动再初始化
    logger->info("initClient start");
    ClientAction::initSocket(env);
    logger->info("initClient end");

    // 执行start操作
    log.Call({Napi::String::New(env, "正在启动Skyline服务器...")});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::string start_result = HttpClient::sendHttpRequest("/start", "start");
    
    if (start_result == "ok") {
        logger->info("服务器启动成功");
        log.Call({Napi::String::New(env, "Skyline服务器启动: ok")});
    } else {
        logger->warn("服务器启动失败. 响应: '{}'", start_result);
        throw std::runtime_error("Skyline服务器启动失败: " + start_result);
    }
    
    SkylineDebugInfo::InitSkylineDebugInfo(env, exports);
    SkylineShell::SkylineShell::Init(env, exports);
    Convert::RegisteInstanceType(env);
    // exports.Set("setConsole", Napi::Function::New(env, Logger::set_console));
    logger->info("return result");
  } catch (const std::exception& e) {
    logger->error("Error: {}", e.what());
    throw Napi::Error::New(env, e.what());
  }catch (...) {
    logger->error("Unknown error occurred during initialization.");
    throw Napi::Error::New(env, "Unknown error occurred during initialization.");
  }
  return exports;
}

NODE_API_MODULE(cmnative, Init)