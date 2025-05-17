#include "socket_server.hh"
#include <boost/asio.hpp>
#include <thread>
#include <mutex>
#include <memory>
#include "../common/logger.hh"
#include "../common/snowflake.hh"
#include "../common/convert.hh"
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
    static bool isBlock = false;
    using snowflake_t = snowflake<1534832906275L>;
    static snowflake_t communicationUuid;

    void processMessage(std::shared_ptr<tcp::socket> client, const std::string &message) {
        try {
            logger->info("received message: {}", message);
            
            if (message.empty()) {
                logger->error("received message is empty!");
                return;
            }

            nlohmann::json json = nlohmann::json::parse(message);
            if (!json["id"].empty()) {
            auto id = json["id"];
              if (socketRequest.find(id) != socketRequest.end())
              {
                logger->info("found id: {}", id.get<std::string>());
                // server发出的消息的回复
                socketRequest[id]->set_value(message);
                socketRequest.erase(id);
                return;
              }
            }
            if (isBlock) {
                logger->info("blocked, push to queue...");
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
                    for (auto& client : clients) {
                        if (client && client->is_open()) {
                            boost::asio::write(*client, boost::asio::buffer(message + "\n"));
                        }
                    }
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

    void handleClient(std::shared_ptr<tcp::socket> client) {
        try {
            boost::asio::streambuf buffer;
            while (true) {
                boost::asio::read_until(*client, buffer, '\n');
                std::string message{std::istreambuf_iterator<char>(&buffer),
                                  std::istreambuf_iterator<char>()};
                if (!message.empty()) {
                    processMessage(client, message);
                }
            }
        } catch (boost::system::system_error& e) {
            logger->info("Client disconnected: {}", e.what());
            clients.erase(std::remove_if(clients.begin(), clients.end(),
                [&client](const std::shared_ptr<tcp::socket>& s) { return s == client; }),
                clients.end());
        }
    }

    int startInner(std::string &host, int port) {
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
        communicationUuid.init(3, 1);
        return Napi::Number::New(info.Env(), startInner(host, port));
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
      
      if (clients.size() == 0) {
        throw Napi::Error::New(info.Env(), "No clients connected");
      }
      logger->info("send to client: {}", json.dump());
      for (auto& client : clients) {
          if (client && client->is_open()) {
              boost::asio::write(*client, boost::asio::buffer(json.dump() + "\n"));
          }
      }
      auto promiseObj = std::make_shared<std::promise<std::string>>();
      std::future<std::string> futureObj = promiseObj->get_future();
      socketRequest.emplace(json["id"], promiseObj); // Updated to use json["id"]
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
            ws.Set("send", Napi::Function::New(env, [](const Napi::CallbackInfo &info) {
                    if (info.Length() < 1 || !info[0].IsString()) {
                        throw Napi::TypeError::New(info.Env(), "send: Wrong argument type");
                    }
                    auto message = info[0].As<Napi::String>().Utf8Value();
                    logger->info("send message to all clients:{}", message);
                    for (auto& client : clients) {
                        if (client && client->is_open()) {
                            boost::asio::write(*client, boost::asio::buffer(message + "\n"));
                        }
                    }
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
