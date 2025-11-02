#include "server_action.hh"
#include "server_socket.hh"
#include <boost/asio.hpp>
#include <cstdint>
#include <thread>
#include <mutex>
#include <memory>
#include "../common/logger.hh"
#include "../common/snowflake.hh"
#include "../common/convert.hh"
#include "server.hh"
#include <nlohmann/json.hpp>

using Logger::logger;

namespace ServerAction {
    static Napi::ThreadSafeFunction messageHandleTsfn;
    static std::shared_ptr<Napi::FunctionReference> messageHandleRef;
    static std::unordered_map<int64_t, std::shared_ptr<std::promise<std::string>>> socketRequest;
    static std::queue<std::string> blockQueue;
    static bool isBlock = false;
    static int64_t requestId = 1;
    static std::shared_ptr<SkylineServer::Server> server;

    void processMessage(const std::string &message) {
        try {
            logger->info("Received message: {}", message);
            
            if (message.empty()) {
                logger->error("Received message is empty!");
                return;
            }

            nlohmann::json json = nlohmann::json::parse(message);
            if (!json["id"].empty()) {
              auto id = json["id"];
              if (socketRequest.find(id) != socketRequest.end())
              {
                logger->info("found id: {}", id.get<int64_t>());
                // server发出的消息的回复
                socketRequest[id]->set_value(message);
                socketRequest.erase(id);
                return;
              }
            }
            if (isBlock) {
                logger->info("blocked, push to queue... {}", message);
                blockQueue.push(message);
                return;
            }

            // Call the JavaScript callback through the thread-safe function
            auto callback = [message](Napi::Env env, Napi::Function jsCallback) {
                auto ws = Napi::Object::New(env);
                ws.Set("send", Napi::Function::New(env, [](const Napi::CallbackInfo &info) {
                    if (info.Length() < 1 || !info[0].IsString()) {
                        throw Napi::TypeError::New(info.Env(), "send: Wrong argument type");
                    }
                    auto message = info[0].As<Napi::String>().Utf8Value();
                    logger->info("send message to all clients:{}", message);
                    server->sendMessage(message);
                }));

                try {
                    jsCallback.Call({ws, Napi::String::New(env, message)});
                } catch (const std::exception &e) {
                    logger->error("Error in callback: {}", e.what());
                } catch (...) {
                    logger->error("Unknown error occurred in callback");
                }
            };

            messageHandleTsfn.BlockingCall(callback);
        } catch (const std::exception &e) {
            logger->error("Error processing message: {}", e.what());
        } catch (...) {
            logger->error("Unknown error occurred while processing message");
        }
    }

    int startInner(const Napi::CallbackInfo &info) {
        try {
            server = std::make_shared<SkylineServer::ServerSocket>();
            server->Init(info);

            std::thread([&]() {
                while (true) {
                    try {
                        // Handle client in a separate thread
                        auto msg = server->receiveMessage();
                        if (msg.empty()) {
                            continue;
                        }
                        processMessage(msg);
                    } catch (const std::exception &e) {
                        logger->error("Error in message processing thread: {}", e.what());
                        break;
                    } catch (...) {
                        logger->error("Unknown error occurred in message processing thread");
                        break;
                    }
                }
            }).detach();
            return 0;
        } catch (std::exception& e) {
            logger->error("Failed to start server: {}", e.what());
            return 1;
        }
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
        
        return Napi::Number::New(info.Env(), startInner(info));
    }

    void stop(const Napi::CallbackInfo &info) {
        
    }

    void setMessageCallback(const Napi::CallbackInfo &info) {
        if (info.Length() < 1) {
            throw Napi::TypeError::New(info.Env(), "setMessageCallback: Wrong number of arguments");
        }
        if (!info[0].IsFunction()) {
            throw Napi::TypeError::New(info.Env(), "First argument must be a function");
        }

        // Create a ThreadSafeFunction
        messageHandleTsfn = Napi::ThreadSafeFunction::New(
            info.Env(),
            info[0].As<Napi::Function>(),
            "Socket Callback",
            0,
            1
        );
        messageHandleRef = std::make_shared<Napi::FunctionReference>(
            Napi::Persistent(info[0].As<Napi::Function>())
        );

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
      if (requestId >= INT64_MAX) {
        requestId = 1;
      }
      auto id = requestId++;
      json["id"] = id;

      // 先存储，再发送
      auto promiseObj = std::make_shared<std::promise<std::string>>();
      std::future<std::string> futureObj = promiseObj->get_future();
      socketRequest.emplace(id, promiseObj); // Updated to use json["id"]
      logger->info("send to client: {}", id);
      server->sendMessage(json.dump());
      // 3秒超时
      auto start = std::chrono::high_resolution_clock::now();
      while (true) {
        if (!blockQueue.empty()) {
          auto msg = blockQueue.front();
          blockQueue.pop();
          try {
            // debug消息
            // Client发来的消息
            logger->info("start to handle blocked message: {}", msg);
            // Call the JavaScript callback through the thread-safe function
            auto ws = Napi::Object::New(env);
            ws.Set("send", Napi::Function::New(env, [](const Napi::CallbackInfo &info) {
                    if (info.Length() < 1 || !info[0].IsString()) {
                        throw Napi::TypeError::New(info.Env(), "send: Wrong argument type");
                    }
                    auto message = info[0].As<Napi::String>().Utf8Value();
                    logger->info("send message to all clients:{}", message);
                    server->sendMessage(message);
                }));
            messageHandleRef->Value().Call({ws, Napi::String::New(env, msg)});
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
          socketRequest.erase(json["id"]);
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
}
