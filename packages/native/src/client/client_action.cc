#include <cstdint>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <memory>
#include <future>
#include "client_action.hh"
#include "../common/logger.hh"
#include "../common/convert.hh"
#include "client_socket.hh"

using Logger::logger;

namespace ClientAction {
    static std::mutex socketRequestMutex;  // Add mutex for thread synchronization
    static std::unordered_map<int64_t, std::shared_ptr<std::promise<std::string>>> socketRequest;
    static std::queue<nlohmann::json> callbackQueue;
    static std::mutex callbackQueueMutex;
    static int64_t requestId = 1;
    static std::shared_ptr<SkylineClient::Client> client;

    void processMessage(const std::string &message) {
        logger->info("Received message: {}", message);
        if (message.empty()) {
            logger->error("Received message is empty!");
            return;
        }

        nlohmann::json json = nlohmann::json::parse(message);
        if (json["type"].empty() && json.contains("id")) {
            // 响应体一定没有type，只有id
            // 但是不能用包含id就去查找请求，因为服务器的emitCallback请求也是包含id的
            // 这时找不到，浪费性能，所以用type为空排除emitCallback的id查找，提高效率
            // Response message
            auto id = json["id"].get<int64_t>();
            logger->debug("Received message id: {}", id);

            std::shared_ptr<std::promise<std::string>> promise;
            {
                std::lock_guard<std::mutex> lock(socketRequestMutex);  // Lock during map access
                if (auto target = socketRequest.find(id); target != socketRequest.end()) {
                    promise = std::move(target->second);
                    socketRequest.erase(target);
                } else {
                    logger->error("id not found: {}", id);
                }
            }
            if (promise) {
                promise->set_value(message);
            }
        } else if(!json["type"].empty()) {

            if (json["type"].get<std::string>() == "emitCallback") {
                auto callbackId = json["callbackId"].get<int64_t>();
                auto block = json["data"]["block"];
                {
                    // 直接丢进队列，可能send那边会处理，也可能是下面的tsfn处理
                    std::lock_guard<std::mutex> lock(callbackQueueMutex);
                    logger->debug("Push callback msg to queue...");
                    callbackQueue.push(std::move(json));
                }
                auto ptr = Convert::find_callback(callbackId);
                if (ptr != nullptr) {
                    logger->debug("callbackId found: {}", callbackId);
                    if (block.is_boolean() && block.get<bool>() == false) {
                        logger->debug("Block value: false...");
                        ptr->tsfn.NonBlockingCall([](Napi::Env env, Napi::Function jsCallback) {
                            logger->debug("NonBlockingCall...");
                            nlohmann::json json;
                            {
                                std::lock_guard<std::mutex> lock(callbackQueueMutex);
                                if (callbackQueue.empty()) {
                                    logger->warn("CallbackQueue is empty when processing blocking callback.");
                                    return;
                                }
                                json = callbackQueue.front();
                                callbackQueue.pop();
                                logger->debug("Pop msg from queue...");
                            }
                            auto args = json["data"]["args"];
                            auto callbackId = json["callbackId"].get<int64_t>();
                            Napi::HandleScope scope(env);
                            std::vector<Napi::Value> argsVec;
                            argsVec.reserve(args.size());
                            
                            for (auto& arg : args) {
                                argsVec.push_back(Convert::convertJson2Value(env, arg));
                            }
                            logger->debug("call callback function...");
                            auto resultValue = jsCallback.Call(argsVec);
                            logger->debug("call callback function end...");

                            auto resultJson = Convert::convertValue2Json(env, resultValue);
                            if (json.contains("id")) {
                                logger->info("reply callback...");
                                // Add newline as message delimiter
                                client->sendMessage(nlohmann::json{
                                    {"id", json["id"].get<int64_t>()},
                                    {"type", "callbackReply"},
                                    {"result", std::move(resultJson)},
                                }.dump());
                            }
                        });
                    } else {
                        logger->debug("Block value: true...");
                        ptr->tsfn.BlockingCall([](Napi::Env env, Napi::Function jsCallback) {
                            logger->debug("BlockingCall...");
                            nlohmann::json json;
                            {
                                std::lock_guard<std::mutex> lock(callbackQueueMutex);
                                if (callbackQueue.empty()) {
                                    logger->warn("CallbackQueue is empty when processing blocking callback.");
                                    return;
                                }
                                json = callbackQueue.front();
                                callbackQueue.pop();
                                logger->debug("Pop msg from queue...");
                            }
                            auto args = json["data"]["args"];
                            auto callbackId = json["callbackId"].get<int64_t>();
                            Napi::HandleScope scope(env);
                            std::vector<Napi::Value> argsVec;
                            argsVec.reserve(args.size());
                            
                            for (auto& arg : args) {
                                argsVec.push_back(Convert::convertJson2Value(env, arg));
                            }
                            logger->debug("Call callback function...");
                            auto resultValue = jsCallback.Call(argsVec);
                            logger->debug("Call callback function end...");

                            auto resultJson = Convert::convertValue2Json(env, resultValue);
                            if (json.contains("id")) {
                                logger->debug("reply callback...");
                                // Add newline as message delimiter
                                client->sendMessage(nlohmann::json{
                                    {"id", json["id"].get<int64_t>()},
                                    {"type", "callbackReply"},
                                    {"result", std::move(resultJson)},
                                }.dump());
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
            client = std::make_shared<SkylineClient::ClientSocket>();
            client->Init(env);

            // Start a thread for reading messages
            std::thread([&]() {
                try {
                    while (true) {
                        std::string message = client->receiveMessage();
                        processMessage(message);
                    }
                } catch (std::exception& e) {
                    logger->error("Read message error: {}", e.what());
                } catch (...) {
                    logger->error("Unknown error occurred in message reading thread.");
                }
            }).detach();

        } catch (std::exception& e) {
            logger->error("Connection error: {}", e.what());
            throw Napi::Error::New(env, std::string("Socket connection failed: ") + e.what());
        }
    }

    nlohmann::json sendMessageSync(nlohmann::json& data) {
        if (requestId >= INT64_MAX) {
            requestId = 1;
        }
        auto id = requestId++;
        data["id"] = id;
        logger->info("Send to server {}", id);

        auto promiseObj = std::make_shared<std::promise<std::string>>();
        std::future<std::string> futureObj = promiseObj->get_future();
        
        // Lock the mutex while manipulating the map
        {
            std::lock_guard<std::mutex> lock(socketRequestMutex);
            auto insertResult = socketRequest.emplace(id, promiseObj);
            if (!insertResult.second) {
                logger->error("insert failed: {}", id);
                throw std::runtime_error("id insert to request map failed: " + std::to_string(id));
            }
        }

        logger->info("Sending message to server: {}", id);
        client->sendMessage(std::move(data.dump()));
        logger->info("Message sent, waiting for response: {}", id);

        auto start = std::chrono::steady_clock::now();
        while (true) {
            if (!callbackQueue.empty()) {
                auto json = callbackQueue.front();
                callbackQueue.pop();
                logger->debug("Pop msg from queue, start to handle callback.");
                auto callbackId = json["callbackId"].get<int64_t>();
                auto ptr = Convert::find_callback(callbackId);
                if (ptr != nullptr) {
                    logger->info("callbackId found: {}", callbackId);
                    auto env = ptr->funcRef->Env();
                    auto args = json["data"]["args"];
                    auto callbackId = json["callbackId"].get<int64_t>();
                    Napi::HandleScope scope(env);
                    std::vector<Napi::Value> argsVec;
                    argsVec.reserve(args.size());
                    
                    for (auto& arg : args) {
                        argsVec.push_back(Convert::convertJson2Value(env, arg));
                    }
                    std::shared_ptr<Napi::FunctionReference> funcRef = ptr->funcRef;
                    auto resultValue = funcRef->Value().Call(argsVec);

                    auto resultJson = Convert::convertValue2Json(env, resultValue);
                    if (json.contains("id")) {
                        logger->info("reply callback...");
                        // Add newline as message delimiter
                        client->sendMessage(nlohmann::json{
                            {"id", json["id"].get<int64_t>()},
                            {"type", "callbackReply"},
                            {"result", std::move(resultJson)},
                        }.dump());
                    }
                } else {
                    logger->error("CallbackId not found: {}", callbackId);
                }
            }

            auto delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>
                (std::chrono::steady_clock::now() - start).count();
            if (delta_ms > 5000) {
                logger->error("Operation timed out after 5 seconds, request data:\n{}", data.dump());
                throw std::runtime_error("Operation timed out after 5 seconds, request data:\n" + data.dump());
            }
            
            if (futureObj.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                break;
            }
        }

        std::string result = futureObj.get();
        logger->info("Result: {}", result);

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
        if (requestId >= INT64_MAX) {
            requestId = 1;
        }
        auto id = requestId++;
        data["id"] = id;
        logger->info("send to server {}", id);

        client->sendMessage(data.dump());
    }

    void callDynamicAsync(int64_t instanceId, const std::string& action, nlohmann::json& args) {
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

    nlohmann::json callDynamicSync(int64_t instanceId, const std::string& action, nlohmann::json& args) {
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

    nlohmann::json callDynamicPropertySetSync(int64_t instanceId, const std::string& action, nlohmann::json& args) {
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

    nlohmann::json callDynamicPropertyGetSync(int64_t instanceId, const std::string& action) {
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
