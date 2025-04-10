#include "websocket.hh"
#include "napi.h"
#include <ixwebsocket/IXWebSocketServer.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <future>
#include "../snowflake.hh"
#include "../include/convert.hh"

namespace WebSocketServer {

std::shared_ptr<ix::WebSocketServer> server;
Napi::ThreadSafeFunction tsfn;
static std::map<std::string, std::shared_ptr<std::promise<std::string>>> wsRequest;

using snowflake_t = snowflake<1534832906275L>;
snowflake_t communicationUuid;
void sendMsg(Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be a string");
  }
  auto message = info[0].As<Napi::String>().Utf8Value();
  auto clients = server->getClients();
  if (clients.size() == 0) {
    throw Napi::Error::New(info.Env(), "No clients connected");
  }
  spdlog::info("send to client: {}", message);
  clients.begin()->get()->send(message);
}
int startInner(std::string &host, int port) {
  server = std::make_shared<ix::WebSocketServer>(port, host);

  auto res = server->listen();
  if (!res.first) {
    spdlog::error("Failed to listen on {}:{} - {}", host, port, res.second);
    return 1;
  }

  spdlog::info("WebSocket server listening on {}:{}", host, port);

  server->setOnClientMessageCallback(
    [](std::shared_ptr<ix::ConnectionState> connectionState,
       ix::WebSocket &webSocket, const ix::WebSocketMessagePtr &msg) {
      // The ConnectionState object contains information about the connection
      spdlog::info("Remote ip: {}", connectionState->getRemoteIp());

      if (msg->type == ix::WebSocketMessageType::Message) {
        spdlog::info("Received: {}", msg->str);

        try {
          // Capture the message
          std::string message = msg->str;

          nlohmann::json json = nlohmann::json::parse(message);
          if (!json["id"].empty()) {
            auto id = json["id"];
            spdlog::info("received message: {}", json["id"].get<std::string>());
            if (wsRequest.find(id) != wsRequest.end())
            {
              // server发出的消息的回复
              wsRequest[id]->set_value(msg->str);
              wsRequest.erase(id);
              return;
            }
            else {
              spdlog::info("id not found: {}", id.get<std::string>());
            }
          }

          // Client发来的消息
          // Call the JavaScript callback through the thread-safe function
          auto callback = [message](Napi::Env env, Napi::Function jsCallback) {
            auto ws = Napi::Object::New(env);
            ws.Set("send", Napi::Function::New(env, sendMsg));
            jsCallback.Call({ws, Napi::String::New(env, message)});
          };

          tsfn.BlockingCall(callback);
        }
        catch (const std::exception &e) {
          spdlog::error("Error parsing JSON: {}", e.what());
        }

      }
    });
  
  // Disable per message deflate for better performance on low power devices
  server->disablePerMessageDeflate();

  // Run the server in the background
  server->start();
  return 0;
}

Napi::Number start(Napi::CallbackInfo &info) {
  if (info.Length() < 2) {
    throw Napi::TypeError::New(info.Env(), "Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be a string");
  }
  if (!info[1].IsNumber()) {
    throw Napi::TypeError::New(info.Env(), "Second argument must be a number");
  }
  auto host = info[0].As<Napi::String>().Utf8Value();
  auto port = info[1].As<Napi::Number>().Int32Value();
  communicationUuid.init(3, 1);
  return Napi::Number::New(info.Env(), startInner(host, port));
}
void stop(Napi::CallbackInfo &info) {
  if (server) {
    // Release the thread-safe function
    if (tsfn) {
      tsfn.Release();
    }

    server->stop();
    spdlog::info("WebSocket server stopped");
  }
}

void setMessageCallback(Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "Wrong number of arguments");
  }
  if (!info[0].IsFunction()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be a function");
  }

  // Create a ThreadSafeFunction
  tsfn = Napi::ThreadSafeFunction::New(
      info.Env(),
      info[0].As<Napi::Function>(), // JavaScript function from caller
      "WebSocket Callback",         // Name
      0,                            // Unlimited queue
      1                             // Only one thread will use this initially
  );

  spdlog::info("Set message callback");
}
Napi::Value sendMessageSync(Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be a string");
  }
  spdlog::info("sendMessageSync: {}", info[0].As<Napi::String>().Utf8Value());
  // 解析json字符串
  auto message = info[0].As<Napi::String>().Utf8Value();
  nlohmann::json json = nlohmann::json::parse(message);
  json["id"] = std::to_string(communicationUuid.nextid());
  auto clients = server->getClients();
  if (clients.size() == 0) {
    throw Napi::Error::New(info.Env(), "No clients connected");
  }
  spdlog::info("send to client: {}", json.dump());
  clients.begin()->get()->send(json.dump());
  auto promiseObj = std::make_shared<std::promise<std::string>>();
  std::future<std::string> futureObj = promiseObj->get_future();
  wsRequest.emplace(json["id"], promiseObj); // Updated to use json["id"]
  // TODO: 3秒超时
  std::string result = futureObj.get();
  auto env = info.Env();
  auto resp = nlohmann::json::parse(result);
  auto v = Convert::convertJson2Value(env, resp["result"]);
  return v;
}
} // namespace WebSocketServer
