#include <napi.h>
#include <sys/types.h>
#include "process.hh"
#include "../common/logger.hh"

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#endif

using Logger::logger;

static Napi::Object Init(Napi::Env env, Napi::Object exports) {
  #ifdef _WIN32
  // Required on Windows for sockets
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    throw Napi::Error::New(env, "Failed to initialize WinSock");
  }
  #endif
  
  Logger::Init();
  logger->info("initServer end");
  
  exports.Set("start", Napi::Function::New(env, Process::start));
  exports.Set("stop", Napi::Function::New(env, Process::stop));
  exports.Set("setMessageCallback", Napi::Function::New(env, Process::setMessageCallback));
  exports.Set("sendMessageSync", Napi::Function::New(env, Process::sendMessageSync));
  exports.Set("sendMessageSingle", Napi::Function::New(env, Process::sendMessageSingle));
  logger->info("return result");
  return exports;
}

NODE_API_MODULE(skyline_server, Init)