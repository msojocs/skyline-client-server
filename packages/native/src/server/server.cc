#include <napi.h>
#include <sys/types.h>
#include "websocket.hh"
#include <ixwebsocket/IXWebSocketServer.h>
#include "../common/logger.hh"
using Logger::logger;

static Napi::Object Init(Napi::Env env, Napi::Object exports) {

  #ifdef _WIN32
  // Required on Windows
  ix::initNetSystem();
  #endif
  
  Logger::Init();
  logger->info("initWebSocket end");
  exports.Set("start", Napi::Function::New(env, WebSocketServer::start));
  exports.Set("stop", Napi::Function::New(env, WebSocketServer::stop));
  exports.Set("setMessageCallback", Napi::Function::New(env, WebSocketServer::setMessageCallback));
  exports.Set("sendMessageSync", Napi::Function::New(env, WebSocketServer::sendMessageSync));
  logger->info("return result");
  return exports;
}

NODE_API_MODULE(skyline_server, Init)