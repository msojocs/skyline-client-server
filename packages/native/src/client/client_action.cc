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
    static bool blocked = false;
    static int64_t requestId = 1;
    static std::shared_ptr<SkylineClient::Client> client;

    void processMessage(std::string &message) {
        logger->info("Received message: {}", message);
        if (message.empty()) {
            logger->error("Received message is empty!");
            return;
        }

        nlohmann::json json = nlohmann::json::parse(message);
        if (json["type"].empty() && json.contains("id")) {
            // Response message
            auto id = json["id"];
            logger->info("Received message id: {}", json["id"].get<int64_t>());
            
            std::lock_guard<std::mutex> lock(socketRequestMutex);  // Lock during map access
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
                auto callbackId = json["callbackId"].get<int64_t>();
                auto ptr = Convert::find_callback(callbackId);
                if (ptr != nullptr) {
                    logger->info("callbackId found: {}", callbackId);
                    auto block = json["data"]["block"];
                    if (block.is_boolean() && block.get<bool>() == false) {
                        ptr->tsfn.NonBlockingCall([json](Napi::Env env, Napi::Function jsCallback) {
                            auto args = json["data"]["args"];
                            auto callbackId = json["callbackId"].get<int64_t>();
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
                                    {"id", json["id"].get<int64_t>()},
                                    {"type", "callbackReply"},
                                    {"result", resultJson},
                                };
                                // Add newline as message delimiter
                                client->sendMessage(callbackResult.dump());
                            }
                        });
                    } else {
                        ptr->tsfn.BlockingCall([json](Napi::Env env, Napi::Function jsCallback) {
                            auto args = json["data"]["args"];
                            auto callbackId = json["callbackId"].get<int64_t>();
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
                                    {"id", json["id"].get<int64_t>()},
                                    {"type", "callbackReply"},
                                    {"result", resultJson},
                                };
                                // Add newline as message delimiter
                                client->sendMessage(callbackResult.dump());
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

        blocked = true;
        // Add newline as message delimiter
        std::string message = data.dump();

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
            logger->info("is exists: {}", socketRequest.find(id) != socketRequest.end());
        }

        client->sendMessage(message);

        auto start = std::chrono::steady_clock::now();
        while (true) {
            if (!callbackQueue.empty()) {
                auto json = callbackQueue.front();
                callbackQueue.pop();
                logger->info("start to handle callback.");
                auto callbackId = json["callbackId"].get<int64_t>();
                auto ptr = Convert::find_callback(callbackId);
                if (ptr != nullptr) {
                    logger->info("callbackId found: {}", callbackId);
                    auto env = ptr->funcRef->Env();
                    auto args = json["data"]["args"];
                    auto callbackId = json["callbackId"].get<int64_t>();
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
                            {"id", json["id"].get<int64_t>()},
                            {"type", "callbackReply"},
                            {"result", resultJson},
                        };
                            // Add newline as message delimiter
                            client->sendMessage(callbackResult.dump());
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
