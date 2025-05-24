#include "socket_server.hh"
#include <boost/asio.hpp>
#include <thread>
#include <memory>
#include "../common/logger.hh"
#include "../common/snowflake.hh"
#include "../common/convert.hh"
#include "napi.h"
#include <nlohmann/json.hpp>

using Logger::logger;
using boost::asio::ip::tcp;

namespace SocketServer {
    static boost::asio::io_context io_context;
    static std::shared_ptr<tcp::acceptor> acceptor;
    static std::vector<std::shared_ptr<tcp::socket>> clients;
    static Napi::ThreadSafeFunction messageHandleTsfn;
    static std::shared_ptr<Napi::FunctionReference> messageHandleRef;
    static std::map<std::string, std::shared_ptr<std::promise<std::string>>> socketRequest;
    static std::queue<std::string> blockQueue;
    using snowflake_t = snowflake<1534832906275L>;
    static snowflake_t communicationUuid;
    
    void processMessage(std::shared_ptr<tcp::socket> client, const std::string &message) {
        try {
            logger->info("received message: {}", message);

            // 使用string_view以避免不必要的字符串拷贝
            std::string_view msg_view(message);
            
            // 优化ID查找逻辑
            if (message.find("id") != std::string::npos) {
                static const std::string_view id_prefix = "\"id\":\"";
                auto startPos = message.find(id_prefix);
                if (startPos != std::string::npos) {
                    startPos += id_prefix.length();
                    auto endPos = message.find("\"", startPos);
                    if (endPos == std::string::npos) {
                        logger->error("End quote for id not found in message: {}", message);
                        return; // 如果没有找到结束引号，直接返回
                    }
                    
                    std::string id = message.substr(startPos, endPos - startPos);
                    
                    // 使用find而非operator[]，避免不必要的插入操作
                    auto it = socketRequest.find(id);
                    if (it != socketRequest.end()) {
                        logger->info("found id: {}", id);
                        // server发出的消息的回复
                        it->second->set_value(message);
                        socketRequest.erase(it);
                        return;
                    }
                }
            }
            logger->info("push to queue... {}", message);
            blockQueue.push(message);

            // Call the JavaScript callback through the thread-safe function
            auto callback = [message](Napi::Env env, Napi::Function jsCallback) {
                auto firstMsg = blockQueue.front();
                if (firstMsg != message) {
                    logger->warn("Message mismatch: expected {}, got {}", firstMsg, message);
                    return;
                }

                try {
                    blockQueue.pop();  // Remove the message from the queue after processing
                    logger->info("Handle message in js start. {}", message);
                    jsCallback.Call({Napi::String::New(env, message)});
                    logger->info("Handle message in js end. {}", message);
                } catch (const std::exception &e) {
                    logger->error("Error in callback: {}", e.what());
                } catch (...) {
                    logger->error("Unknown error occurred in callback");
                }
            };

            logger->info("blocking call start: {}", message);
            messageHandleTsfn.BlockingCall(callback);
            logger->info("blocking call end: {}", message);
        } catch (const std::exception &e) {
            logger->error("Error processing message: {}", e.what());
        } catch (...) {
            logger->error("Unknown error occurred while processing message");
        }
    }
    
    void handleClient(std::shared_ptr<tcp::socket> client) {
        try {
            boost::asio::streambuf buffer;
            std::istream stream(&buffer);
            std::string message;

            while (true) {
                boost::asio::read_until(*client, buffer, '\n');
                std::getline(stream, message);  // This properly reads up to the delimiter
                if (!message.empty()) {
                    processMessage(client, message);
                }
            }
        } catch (boost::system::system_error& e) {
            logger->error("Client disconnected: {}", e.what());
            clients.erase(std::remove_if(clients.begin(), clients.end(),
                [&client](const std::shared_ptr<tcp::socket>& s) { return s == client; }),
                clients.end());
        }
    }

    int startInner(const Napi::Env &env, std::string &host, int port) {
        try {
            acceptor = std::make_shared<tcp::acceptor>(io_context, tcp::endpoint(tcp::v4(), port));
            
            // Start accepting connections
            std::thread([&]() {
                while (true) {
                    auto client = std::make_shared<tcp::socket>(io_context);
                    acceptor->accept(*client);
                    
                    {
                        clients.push_back(client);
                    }
                    
                    // Handle client in a separate thread
                    std::thread([client]() {
                        handleClient(client);
                    }).detach();
                }
            }).detach();

            // Start the io_context in a separate thread
            std::thread([&]() {
                io_context.run();
            }).detach();

            logger->info("Socket server listening on {}:{}", host, port);
            return 0;
        } catch (std::exception& e) {
            logger->error("Failed to start server: {}", e.what());
            throw Napi::Error::New(env, e.what());
        }
    }

    Napi::Value start(const Napi::CallbackInfo &info) {
        if (info.Length() < 2) {
            throw Napi::TypeError::New(info.Env(), "start: Wrong number of arguments");
        }
        if (!info[0].IsString()) {
            throw Napi::TypeError::New(info.Env(), "First argument must be a string");
        }
        if (!info[1].IsNumber()) {
            throw Napi::TypeError::New(info.Env(), "Second argument must be a number");
        }
        auto env = info.Env();
        auto host = info[0].As<Napi::String>().Utf8Value();
        auto port = info[1].As<Napi::Number>().Int32Value();
        communicationUuid.init(3, 1);
        startInner(env, host, port);
        return env.Undefined();
    }

    void stop(const Napi::CallbackInfo &info) {
        if (acceptor) {
            if (messageHandleTsfn) {
                messageHandleTsfn.Release();
            }
            
            {
                for (auto& client : clients) {
                    if (client && client->is_open()) {
                        client->close();
                    }
                }
                clients.clear();
            }
            
            acceptor->close();
            io_context.stop();
            logger->info("Socket server stopped");
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
    Napi::Value sendMessageSingle(const Napi::CallbackInfo &info) {
        if (info.Length() < 1) {
            throw Napi::TypeError::New(info.Env(), "sendMessageSingle: Wrong number of arguments");
        }
        
        if (!info[0].IsString()) {
            throw Napi::TypeError::New(info.Env(), "First argument must be a string");
        }
        auto message = info[0].As<Napi::String>().Utf8Value();
        logger->info("send message to all clients:{}", message);
        for (auto& client : clients) {
            if (client && client->is_open()) {
                boost::asio::write(*client, boost::asio::buffer(message + "\n"));
            }
        }
        return info.Env().Undefined();
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
      
      // 提前获取消息内容，避免重复调用
      auto messageStr = info[0].As<Napi::String>().Utf8Value();
      logger->info("sendMessageSync: {}", messageStr);
      
      // 解析json字符串
      nlohmann::json json = nlohmann::json::parse(messageStr);
      
      auto id = std::to_string(communicationUuid.nextid());
      json["id"] = id;
      
      if (clients.empty()) {
        throw Napi::Error::New(info.Env(), "No clients connected");
      }
      
      // 先存储，再发送
      auto promiseObj = std::make_shared<std::promise<std::string>>();
      std::future<std::string> futureObj = promiseObj->get_future();
      
      // 提前将JSON序列化，避免多次执行
      std::string jsonString = json.dump() + "\n";
      
      {
        // 使用作用域限制插入操作的临界区范围
        socketRequest.emplace(id, promiseObj);
      }
      
      logger->info("send to client: {}", jsonString);
      
      // 对所有客户端一次性发送消息
      for (auto& client : clients) {
          if (client && client->is_open()) {
              boost::asio::write(*client, boost::asio::buffer(jsonString));
          }
      }
      
      // 使用更高效的等待方式
      auto start = std::chrono::steady_clock::now();
      std::chrono::duration<double> timeoutDuration(3.0); // 3秒超时
      
      while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - start;
        
        if (elapsed > timeoutDuration) {
          socketRequest.erase(id);
          throw Napi::Error::New(info.Env(), "Request to client timeout, request data:\n" + jsonString);
        }
        
        auto status = futureObj.wait_for(std::chrono::milliseconds(0));
        if (status == std::future_status::ready) {
          break;
        }
        
        if (!blockQueue.empty()) {
          auto msg = blockQueue.front();
          blockQueue.pop();
          try {
            // debug消息
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
      }
      
      logger->info("futureObj wait end, id: {}", id);
      std::string result = futureObj.get();
      
    auto resp = nlohmann::json::parse(result);
    auto v = Convert::convertJson2Value(env, resp["result"]);
    return v;
    }
}
