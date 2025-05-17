#include <napi.h>
#include <sys/types.h>
#include "socket_server.hh"
#include "../common/logger.hh"

#ifdef _WIN32
#include <winsock2.h>
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
  logger->info("initSocket end");
  
  exports.Set("start", Napi::Function::New(env, SocketServer::start));
  exports.Set("stop", Napi::Function::New(env, SocketServer::stop));
  exports.Set("setMessageCallback", Napi::Function::New(env, SocketServer::setMessageCallback));
  exports.Set("sendMessageSync", Napi::Function::New(env, SocketServer::sendMessageSync));
  logger->info("return result");
  return exports;
}

NODE_API_MODULE(skyline_server, Init)