#include <boost/asio.hpp>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <memory>
#include <future>
#include "socket_client.hh"
#include "../common/logger.hh"
#include "../common/snowflake.hh"
#include "../common/convert.hh"

using Logger::logger;
using boost::asio::ip::tcp;

namespace SocketClient {
    static boost::asio::io_context io_context;
    static std::shared_ptr<tcp::socket> socket;
    static std::map<std::string, std::shared_ptr<std::promise<std::string>>> socketRequest;
    static std::queue<nlohmann::json> callbackQueue;
    static bool blocked = false;
    static std::string serverAddress = "127.0.0.1";
    static int serverPort = 3001;
    using snowflake_t = snowflake<1534832906275L>;
    static snowflake_t serverCommunicationUuid;

    // Helper function to read a complete message from the socket
    std::string readMessage() {
        boost::asio::streambuf buf;
        boost::asio::read_until(*socket, buf, "\n");
        std::string data{std::istreambuf_iterator<char>(&buf),
                        std::istreambuf_iterator<char>()};
        return data;
    }

    void processMessage(std::string &message) {
        logger->info("received message: {}", message);
        if (message.empty()) {
            logger->error("received message is empty!");
            return;
        }

        nlohmann::json json = nlohmann::json::parse(message);
        if (json["type"].empty() && json.contains("id")) {
            // Response message
            auto id = json["id"];
            logger->info("received message: {}", json["id"].get<std::string>());
            if (socketRequest.find(id) != socketRequest.end()) {
                socketRequest[id]->set_value(message);
                socketRequest.erase(id);
            } else {
                logger->error("id not found: {}", id.get<std::string>());
            }
        } else if(!json["type"].empty()) {
            if (blocked) {
                logger->info("blocked, push to queue...");
                callbackQueue.push(json);
                return;
            }

            std::string type = json["type"].get<std::string>();
            if (type == "emitCallback") {
                std::string callbackId = json["callbackId"].get<std::string>();
                auto ptr = Convert::find_callback(callbackId);
                if (ptr != nullptr) {
                    logger->info("callbackId found: {}", callbackId);
                    auto block = json["data"]["block"];
                    if (block.is_boolean() && block.get<bool>() == false) {
                        ptr->tsfn.NonBlockingCall([json](Napi::Env env, Napi::Function jsCallback) {
                            auto args = json["data"]["args"];
                            std::string callbackId = json["callbackId"].get<std::string>();
                            Napi::HandleScope scope(env);
                            std::vector<Napi::Value> argsVec;
                            
                            for (auto& arg : args) {
                                argsVec.push_back(Convert::convertJson2Value(env, arg));
                            }
                            logger->info("call callback function...");
                            auto resultValue = jsCallback.Call(argsVec);
                            logger->info("call callback function end...");

                            auto resultJson = Convert::convertValue2Json(env, resultValue);
                            if (json.contains("id")) {
                                logger->info("reply callback...");
                                nlohmann::json callbackResult = {
                                    {"id", json["id"].get<std::string>()},
                                    {"type", "callbackReply"},
                                    {"result", resultJson},
                                };
                                // Add newline as message delimiter
                                std::string message = callbackResult.dump() + "\n";
                                boost::asio::write(*socket, boost::asio::buffer(message));
                            }
                        });
                    } else {
                        ptr->tsfn.BlockingCall([json](Napi::Env env, Napi::Function jsCallback) {
                            auto args = json["data"]["args"];
                            std::string callbackId = json["callbackId"].get<std::string>();
                            Napi::HandleScope scope(env);
                            std::vector<Napi::Value> argsVec;
                            
                            for (auto& arg : args) {
                                argsVec.push_back(Convert::convertJson2Value(env, arg));
                            }
                            logger->info("call callback function...");
                            auto resultValue = jsCallback.Call(argsVec);
                            logger->info("call callback function end...");

                            auto resultJson = Convert::convertValue2Json(env, resultValue);
                            if (json.contains("id")) {
                                logger->info("reply callback...");
                                nlohmann::json callbackResult = {
                                    {"id", json["id"].get<std::string>()},
                                    {"type", "callbackReply"},
                                    {"result", resultJson},
                                };
                                // Add newline as message delimiter
                                std::string message = callbackResult.dump() + "\n";
                                boost::asio::write(*socket, boost::asio::buffer(message));
                            }
                        });
                    }
                } else {
                    logger->error("callbackId not found: {}", callbackId);
                }
            }
        }
    }

    void initSocket(Napi::Env &env) {
        try {
            serverCommunicationUuid.init(1, 1);

            tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve(serverAddress, std::to_string(serverPort));

            socket = std::make_shared<tcp::socket>(io_context);
            boost::asio::connect(*socket, endpoints);

            // Start a thread for the io_context
            std::thread([&]() {
                try {
                    io_context.run();
                } catch (std::exception& e) {
                    logger->error("IO context error: {}", e.what());
                }
            }).detach();

            // Start a thread for reading messages
            std::thread([&]() {
                try {
                    while (true) {
                        std::string message = readMessage();
                        processMessage(message);
                    }
                } catch (std::exception& e) {
                    logger->error("Read message error: {}", e.what());
                }
            }).detach();

            logger->info("Socket connected to {}:{}", serverAddress, serverPort);
        } catch (std::exception& e) {
            logger->error("Connection error: {}", e.what());
            throw Napi::Error::New(env, std::string("Socket connection failed: ") + e.what());
        }
    }

    nlohmann::json sendMessageSync(nlohmann::json& data) {
        std::string id = std::to_string(serverCommunicationUuid.nextid());
        data["id"] = id;
        logger->info("send to server {}", data.dump());
        
        if (!socket->is_open()) {
            throw std::runtime_error("Socket is not open");
        }

        blocked = true;
        // Add newline as message delimiter
        std::string message = data.dump() + "\n";
        boost::asio::write(*socket, boost::asio::buffer(message));

        auto promiseObj = std::make_shared<std::promise<std::string>>();
        std::future<std::string> futureObj = promiseObj->get_future();
        socketRequest.emplace(id, promiseObj);

        auto start = std::chrono::steady_clock::now();
        while (true) {
            if (!callbackQueue.empty()) {
                auto json = callbackQueue.front();
                callbackQueue.pop();
                logger->info("start to handle callback.");
                std::string callbackId = json["callbackId"].get<std::string>();
                auto ptr = Convert::find_callback(callbackId);
                if (ptr != nullptr) {
                    logger->info("callbackId found: {}", callbackId);
                    auto env = ptr->funcRef->Env();
                    auto args = json["data"]["args"];
                    std::string callbackId = json["callbackId"].get<std::string>();
                    Napi::HandleScope scope(env);
                    std::vector<Napi::Value> argsVec;
                    
                    for (auto& arg : args) {
                        argsVec.push_back(Convert::convertJson2Value(env, arg));
                    }
                    std::shared_ptr<Napi::FunctionReference> funcRef = ptr->funcRef;
                    auto resultValue = funcRef->Value().Call(argsVec);

                    auto resultJson = Convert::convertValue2Json(env, resultValue);
                    if (json.contains("id")) {
                        logger->info("reply callback...");
                        nlohmann::json callbackResult = {
                            {"id", json["id"].get<std::string>()},
                            {"type", "callbackReply"},
                            {"result", resultJson},
                        };
                        // Add newline as message delimiter
                        std::string message = callbackResult.dump() + "\n";
                        boost::asio::write(*socket, boost::asio::buffer(message));
                    }
                } else {
                    logger->error("callbackId not found: {}", callbackId);
                }
            }

            auto delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>
                (std::chrono::steady_clock::now() - start).count();
            if (delta_ms > 5000) {
                blocked = false;
                throw std::runtime_error("Operation timed out after 5 seconds, request data:\n" + data.dump());
            }
            
            if (futureObj.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                break;
            }
        }

        std::string result = futureObj.get();
        blocked = false;
        logger->info("result: {}", result);

        if (result.empty()) {
            throw std::runtime_error("Server response is empty");
        }

        auto resp = nlohmann::json::parse(result);
        if (resp.contains("error")) {
            throw std::runtime_error("Server response error: " + resp["error"].get<std::string>());
        }

        return resp["result"];
    }

    void sendMessageAsync(nlohmann::json& data) {
        std::string id = std::to_string(serverCommunicationUuid.nextid());
        data["id"] = id;
        logger->info("send to server {}", data.dump());

        if (!socket->is_open()) {
            throw std::runtime_error("Socket is not open");
        }

        // Add newline as message delimiter
        std::string message = data.dump() + "\n";
        boost::asio::write(*socket, boost::asio::buffer(message));
    }

    void callDynamicAsync(const std::string& instanceId, const std::string& action, nlohmann::json& args) {
        nlohmann::json json {
            {"type", "dynamic"},
            {"action", action},
            {"data", {
                {"instanceId", instanceId},
                {"params", args}
            }}
        };
        
        sendMessageAsync(json);
    }

    nlohmann::json callConstructorSync(const std::string& clazz, nlohmann::json& args) {
        nlohmann::json json {
            {"type", "constructor"},
            {"clazz", clazz},
            {"data", {
                {"params", args}
            }}
        };
        
        return sendMessageSync(json);
    }

    nlohmann::json callDynamicSync(const std::string& instanceId, const std::string& action, nlohmann::json& args) {
        nlohmann::json json {
            {"type", "dynamic"},
            {"action", action},
            {"data", {
                {"instanceId", instanceId},
                {"params", args}
            }}
        };
        
        return sendMessageSync(json);
    }

    nlohmann::json callDynamicPropertySetSync(const std::string& instanceId, const std::string& action, nlohmann::json& args) {
        nlohmann::json json {
            {"type", "dynamicProperty"},
            {"action", action},
            {"data", {
                {"instanceId", instanceId},
                {"params", args},
                {"propertyAction", "set"},
            }}
        };
        
        return sendMessageSync(json);
    }

    nlohmann::json callDynamicPropertyGetSync(const std::string& instanceId, const std::string& action) {
        nlohmann::json json {
            {"type", "dynamicProperty"},
            {"action", action},
            {"data", {
                {"instanceId", instanceId},
                {"propertyAction", "get"},
            }}
        };
        
        return sendMessageSync(json);
    }

    nlohmann::json callStaticSync(const std::string& clazz, const std::string& action, nlohmann::json& args) {
        nlohmann::json json {
            {"type", "static"},
            {"clazz", clazz},
            {"action", action},
            {"data", {
                {"params", args}
            }}
        };
        
        return sendMessageSync(json);
    }

    nlohmann::json callCustomHandleSync(const std::string& action, nlohmann::json& args) {
        return callStaticSync("customHandle", action, args);
    }
}
