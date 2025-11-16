#include "server_action.hh"
#include "server_socket.hh"
#include <boost/asio.hpp>
#include <condition_variable>
#include <cstdint>
#include <thread>
#include <queue>
#include <memory>
#include "../common/logger.hh"
#include "../common/convert.hh"
#include "server.hh"
#include <nlohmann/json.hpp>

using Logger::logger;

namespace ServerAction {
    static Napi::ThreadSafeFunction messageHandleTsfn;
    static std::shared_ptr<Napi::FunctionReference> messageHandleRef;
    static std::unordered_map<int64_t, std::shared_ptr<std::promise<std::string>>> socketRequest;
    static std::queue<std::string> blockQueue;
    static std::mutex blockQueueMutex;
    static int64_t requestId = 1;
    static std::shared_ptr<SkylineServer::Server> server;

    static std::condition_variable cv_blockUntilNextMessage;
    static std::mutex cv_blockUntilNextMessage_mtx;

    void processMessage(const std::string &message) {
        try {
            logger->debug("Received message: {}", message);
            
            if (message.empty()) {
                logger->error("Received message is empty!");
                return;
            }

            nlohmann::json json = nlohmann::json::parse(message);
            int64_t id = 0;
            if (!json["id"].empty()) {
              id = json["id"].get<int64_t>();
              if (auto target = socketRequest.find(id); target != socketRequest.end()) {
                logger->debug("found id: {}", id);
                // server发出的消息的回复
                target->second->set_value(message);
                socketRequest.erase(target);
                return;
              }
            }
            {
                // 丢到阻塞队列中，可能在sendMessageSync处理，也可能在下面messageHandleTsfn中处理
                std::lock_guard<std::mutex> lock(blockQueueMutex);
                logger->debug("blocked, push to queue... {}", message);
                blockQueue.push(std::move(message));
            }

            // Call the JavaScript callback through the thread-safe function
            auto callback = [](Napi::Env env, Napi::Function jsCallback) {
                // 阻塞时，此层也会阻塞
                std::string message;
                {
                    std::lock_guard<std::mutex> lock(blockQueueMutex);
                    if (blockQueue.empty()) {
                        logger->warn("Block queue is empty when processing message");
                        return;
                    }
                    message = blockQueue.front();
                    blockQueue.pop();
                }

                try {
                    logger->debug("Calling JS callback with message: {}", message);
                    jsCallback.Call({Napi::String::New(env, message)});
                    logger->debug("JS callback executed successfully");
                } catch (const std::exception &e) {
                    logger->error("Error in callback: {}", e.what());
                } catch (...) {
                    logger->error("Unknown error occurred in callback");
                }
            };

            logger->debug("Invoking ThreadSafeFunction callback, id: {}", id);
            messageHandleTsfn.BlockingCall(callback);
        } catch (const std::exception &e) {
            logger->error("Error processing message: {}\noriginal message: {}", e.what(), message);
        } catch (...) {
            logger->error("Unknown error occurred while processing message\noriginal message: {}", message);
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
                        cv_blockUntilNextMessage.notify_all();
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
     * 给客户端发送消息，等待返回
     */
    Napi::Value sendMessageSync(const Napi::CallbackInfo &info) {
      if (info.Length() < 1) {
        throw Napi::TypeError::New(info.Env(), "sendMessageSync: Wrong number of arguments");
      }
      
      if (!info[0].IsString()) {
        throw Napi::TypeError::New(info.Env(), "First argument must be a string");
      }
      auto env = info.Env();
      auto message = info[0].As<Napi::String>().Utf8Value();
      logger->info("sendMessageSync: {}", message);
      // TODO：减少反序列化开销，id作为额外数据进行传递（比如拼接到message中）
      // 解析json字符串
      nlohmann::json json = nlohmann::json::parse(message);
      if (requestId >= INT64_MAX) {
        requestId = 1;
      }
      auto id = requestId++;
      json["id"] = id;

      // 先存储，再发送
      auto promiseObj = std::make_shared<std::promise<std::string>>();
      std::future<std::string> futureObj = promiseObj->get_future();
      socketRequest.emplace(id, promiseObj);
      logger->info("Sending to client: {}", id);
      server->sendMessage(json.dump());
      // 3秒超时
      auto start = std::chrono::high_resolution_clock::now();
      while (true) {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        if (elapsed.count() > 3) {
          socketRequest.erase(id);
          throw Napi::Error::New(info.Env(), "Request to client timeout, request data:\n" + json.dump());
        }
        if (futureObj.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
          break;
        }
        if (blockQueue.empty()) {
            continue;
        }
        auto msg = blockQueue.front();
        blockQueue.pop();
        try {
            // Client发来的消息
            logger->info("start to handle blocked message: {}", msg);
            // Call the JavaScript callback through the thread-safe function
            messageHandleRef->Value().Call({Napi::String::New(env, msg)});
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
      std::string result = futureObj.get();
      auto resp = nlohmann::json::parse(result);
      auto v = Convert::convertJson2Value(env, resp["result"]);
      return v;
    }
    /**
     * 给客户端发送消息
     */
    Napi::Value sendMessageSingle(const Napi::CallbackInfo &info) {
        if (info.Length() < 1) {
            throw Napi::TypeError::New(info.Env(), "sendMessageSingle: Wrong number of arguments");
        }
        
        if (!info[0].IsString()) {
            throw Napi::TypeError::New(info.Env(), "First argument must be a string");
        }
        server->sendMessage(std::move(info[0].As<Napi::String>().Utf8Value()));
        return info.Env().Undefined();
    }
    /**
     * 阻塞至收到客户端消息
     */
    Napi::Value blockUntilNextMessage(const Napi::CallbackInfo &info) {
        std::unique_lock<std::mutex> lock(cv_blockUntilNextMessage_mtx);
        cv_blockUntilNextMessage.wait(lock);
        return info.Env().Undefined();
    }
}
