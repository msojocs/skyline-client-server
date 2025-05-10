#include "websocket.hh"
#include "napi.h"
#include <ixwebsocket/IXWebSocketServer.h>
#include <memory>
#include "../common/logger.hh"
#include <nlohmann/json.hpp>
#include <future>
#include <queue>
#include "../common/snowflake.hh"
#include "../common/convert.hh"

using Logger::logger;

namespace WebSocketServer {

static std::shared_ptr<ix::WebSocketServer> server;
static Napi::ThreadSafeFunction websocketMessageHandleTsfn;
static std::shared_ptr<Napi::FunctionReference> websocketMessageHandleRef = nullptr;
static std::map<std::string, std::shared_ptr<std::promise<std::string>>> wsRequest;
static std::queue<std::string> blockQueue;
static bool isBlock = false;

using snowflake_t = snowflake<1534832906275L>;
snowflake_t communicationUuid;
void sendMsg(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "sendMsg: Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be a string");
  }
  auto message = info[0].As<Napi::String>().Utf8Value();
  auto clients = server->getClients();
  if (clients.size() == 0) {
    throw Napi::Error::New(info.Env(), "No clients connected");
  }
  logger->info("send to client: {}", message);
  for (auto it = clients.begin(); it != clients.end(); ++it) {
    if (it->get() == nullptr) {
      continue;
    }
    if (it->get()->getReadyState() != ix::ReadyState::Open) {
      continue;
    }
    it->get()->send(message);
  }
  
}
int startInner(std::string &host, int port) {
  server = std::make_shared<ix::WebSocketServer>(port, host);

  auto res = server->listen();
  if (!res.first) {
    logger->error("Failed to listen on {}:{} - {}", host, port, res.second);
    return 1;
  }

  logger->info("WebSocket server listening on {}:{}", host, port);

  server->setOnClientMessageCallback(
    [](std::shared_ptr<ix::ConnectionState> connectionState,
       ix::WebSocket &webSocket, const ix::WebSocketMessagePtr &msg) {
      // The ConnectionState object contains information about the connection
      logger->info("Remote ip: {}", connectionState->getRemoteIp());

      std::string message;
      bool needHandle = false;
      if (msg->type == ix::WebSocketMessageType::Message) {
        logger->info("Received: {}", msg->str);
        // Capture the message
        message = msg->str;
        needHandle = true;
      }
      else if (msg->type == ix::WebSocketMessageType::Close) {
        logger->info("Connection closed: {}", msg->str);
        message = "{\"action\": \"disconnected\"}";
        needHandle = true;
      }
      else if (msg->type == ix::WebSocketMessageType::Error) {
        logger->error("Error: {}", msg->str);
      }

      if (!needHandle) {
        return;
      }

      try {

        nlohmann::json json = nlohmann::json::parse(message);
        if (!json["id"].empty()) {
          auto id = json["id"];
          logger->info("received message: {}", json["id"].get<std::string>());
          if (wsRequest.find(id) != wsRequest.end())
          {
            logger->info("found id: {}", id.get<std::string>());
            // server发出的消息的回复
            wsRequest[id]->set_value(msg->str);
            wsRequest.erase(id);
            return;
          }
          else {
            logger->info("id not found: {}", id.get<std::string>());
          }
        }

        if (isBlock) {
          // 如果当前处于阻塞状态，直接放入队列中
          logger->info("blocked, push to queue...");
          blockQueue.push(message);
          return;
        }

        // Client发来的消息
        // Call the JavaScript callback through the thread-safe function
        auto callback = [message](Napi::Env env, Napi::Function jsCallback) {
          auto ws = Napi::Object::New(env);
          ws.Set("send", Napi::Function::New(env, sendMsg));
          try {
            jsCallback.Call({ws, Napi::String::New(env, message)});
          }catch (const std::exception &e) {
            logger->error("Error in callback: {}", e.what());
          } catch (...) {
            logger->error("Unknown error occurred in callback");
          }
        };

        websocketMessageHandleTsfn.BlockingCall(callback);
      }
      catch (const std::exception &e) {
        logger->error("Error parsing JSON: {}", e.what());
      }
      catch (...) {
        logger->error("Unknown error occurred");
      }
    });
  
  // Disable per message deflate for better performance on low power devices
  server->disablePerMessageDeflate();

  // Run the server in the background
  server->start();
  return 0;
}

Napi::Number start(const Napi::CallbackInfo &info) {
  if (info.Length() < 2) {
    throw Napi::TypeError::New(info.Env(), "start: Wrong number of arguments");
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
void stop(const Napi::CallbackInfo &info) {
  if (server) {
    // Release the thread-safe function
    if (websocketMessageHandleTsfn) {
      websocketMessageHandleTsfn.Release();
    }

    server->stop();
    logger->info("WebSocket server stopped");
  }
}

void setMessageCallback(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "setMessageCallback: Wrong number of arguments");
  }
  if (!info[0].IsFunction()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be a function");
  }

  // Create a ThreadSafeFunction
  websocketMessageHandleTsfn = Napi::ThreadSafeFunction::New(
      info.Env(),
      info[0].As<Napi::Function>(), // JavaScript function from caller
      "WebSocket Callback",         // Name
      0,                            // Unlimited queue
      1                             // Only one thread will use this initially
  );
  websocketMessageHandleRef = std::make_shared<Napi::FunctionReference>(Napi::Persistent(info[0].As<Napi::Function>()));

  logger->info("Set message callback");
}
/**
 * 给客户端发送消息
 */
Napi::Value sendMessageSync(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(), "sendMessageSync: Wrong number of arguments");
  }
  if (!info[0].IsString()) {
    throw Napi::TypeError::New(info.Env(), "First argument must be a string");
  }
  auto env = info.Env();
  isBlock = true;
  logger->info("sendMessageSync: {}", info[0].As<Napi::String>().Utf8Value());
  // 解析json字符串
  auto message = info[0].As<Napi::String>().Utf8Value();
  nlohmann::json json = nlohmann::json::parse(message);
  json["id"] = std::to_string(communicationUuid.nextid());
  auto clients = server->getClients();
  if (clients.size() == 0) {
    throw Napi::Error::New(info.Env(), "No clients connected");
  }
  logger->info("send to client: {}", json.dump());
  clients.begin()->get()->send(json.dump());
  auto promiseObj = std::make_shared<std::promise<std::string>>();
  std::future<std::string> futureObj = promiseObj->get_future();
  wsRequest.emplace(json["id"], promiseObj); // Updated to use json["id"]
  // 3秒超时
  auto start = std::chrono::high_resolution_clock::now();
  while (true) {
    if (blockQueue.size() > 0) {
      auto msg = blockQueue.front();
      blockQueue.pop();
      try {
        // debug消息
        // Client发来的消息
        // Call the JavaScript callback through the thread-safe function
        auto ws = Napi::Object::New(env);
        ws.Set("send", Napi::Function::New(env, sendMsg));
        websocketMessageHandleRef->Value().Call({ws, Napi::String::New(env, msg)});
      }
      catch (const std::exception &e) {
        logger->error("Error parsing JSON: {}", e.what());
        throw Napi::Error::New(info.Env(), e.what());
      }
      catch (...) {
        logger->error("Unknown error occurred");
        throw Napi::Error::New(info.Env(), "Unknown error occurred");
      }
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    if (elapsed.count() > 3) {
      wsRequest.erase(json["id"]);
      isBlock = false;
      throw Napi::Error::New(info.Env(), "Request to client timeout, request data:\n" + json.dump());
    }
    if (futureObj.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
      break;
    }
  }
  isBlock = false;
  std::string result = futureObj.get();
  auto resp = nlohmann::json::parse(result);
  auto v = Convert::convertJson2Value(env, resp["result"]);
  return v;
}
} // namespace WebSocketServer
