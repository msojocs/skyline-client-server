#include <spdlog/spdlog.h>
#include <napi.h>
#include <sys/types.h>
#include "websocket.hh"
#include <ixwebsocket/IXWebSocketServer.h>


static Napi::Object Init(Napi::Env env, Napi::Object exports) {
  ix::initNetSystem();
  spdlog::info("initWebSocket end");
  exports.Set("start", Napi::Function::New(env, WebSocketServer::start));
  exports.Set("stop", Napi::Function::New(env, WebSocketServer::stop));
  exports.Set("setMessageCallback", Napi::Function::New(env, WebSocketServer::setMessageCallback));
  exports.Set("sendMessageSync", Napi::Function::New(env, WebSocketServer::sendMessageSync));
  spdlog::info("return result");
  return exports;
}

NODE_API_MODULE(cmnative, Init)