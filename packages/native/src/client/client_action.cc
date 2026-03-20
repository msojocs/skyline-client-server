#include <cstdint>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <memory>
#include <future>
#include <condition_variable>
#include <algorithm>
#include "client_action.hh"
#include "../common/logger.hh"
#include "../common/convert.hh"
#include "client_socket.hh"

using Logger::logger;

namespace ClientAction {
    struct CallbackQueueItem {
        nlohmann::json payload;
        int64_t messageId;
    };
    static std::mutex socketRequestMutex;  // Add mutex for thread synchronization
    static std::condition_variable socketEventCv;
    static std::unordered_map<int64_t, std::shared_ptr<std::promise<std::string>>> socketRequest;
    static std::queue<CallbackQueueItem> callbackQueue;
    static std::mutex callbackQueueMutex;
    static int64_t requestId = 1;
    static std::shared_ptr<SkylineClient::Client> client;

    void processMessage(const std::string &message, int64_t messageId = 0) {
        logger->debug("Received message length: {}", message.size());
        if (message.empty()) {
            logger->error("Received message is empty!");
            return;
        }

        if (messageId > 0 && (messageId & 1LL) == 1LL) {
            std::shared_ptr<std::promise<std::string>> promise;
            {
                std::lock_guard<std::mutex> lock(socketRequestMutex);
                if (auto target = socketRequest.find(messageId); target != socketRequest.end()) {
                    promise = std::move(target->second);
                    socketRequest.erase(target);
                } else {
                    logger->error("response messageId not found: {}", messageId);
                }
            }
            if (promise) {
                promise->set_value(message);
                socketEventCv.notify_all();
            }
            return;
        }

        nlohmann::json json = nlohmann::json::parse(message);
        if (!json["type"].empty() && json["type"].get<std::string>() == "emitCallback") {
                auto callbackId = json["callbackId"].get<int64_t>();
                auto block = json["data"]["block"];
                {
                    // 直接丢进队列，可能send那边会处理，也可能是下面的tsfn处理
                    std::lock_guard<std::mutex> lock(callbackQueueMutex);
                    logger->debug("Push callback msg to queue...");
                    callbackQueue.push(CallbackQueueItem{std::move(json), messageId});
                }
                socketEventCv.notify_all();
                auto ptr = Convert::find_callback(callbackId);
                if (ptr != nullptr) {
                    logger->debug("callbackId found: {}", callbackId);
                    if (block.is_boolean() && block.get<bool>() == false) {
                        logger->debug("Block value: false...");
                        ptr->tsfn.NonBlockingCall([](Napi::Env env, Napi::Function jsCallback) {
                            logger->debug("NonBlockingCall...");
                            CallbackQueueItem item;
                            {
                                std::lock_guard<std::mutex> lock(callbackQueueMutex);
                                if (callbackQueue.empty()) {
                                    logger->warn("CallbackQueue is empty when processing blocking callback.");
                                    return;
                                }
                                item = std::move(callbackQueue.front());
                                callbackQueue.pop();
                                logger->debug("Pop msg from queue...");
                            }
                            auto &json = item.payload;
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
                            if (item.messageId > 0) {
                                logger->info("reply callback...");
                                client->sendMessage(nlohmann::json{
                                    {"type", "callbackReply"},
                                    {"result", std::move(resultJson)},
                                }.dump(), item.messageId);
                            }
                        });
                    } else {
                        logger->debug("Block value: true...");
                        ptr->tsfn.BlockingCall([](Napi::Env env, Napi::Function jsCallback) {
                            logger->debug("BlockingCall...");
                            CallbackQueueItem item;
                            {
                                std::lock_guard<std::mutex> lock(callbackQueueMutex);
                                if (callbackQueue.empty()) {
                                    logger->warn("CallbackQueue is empty when processing blocking callback.");
                                    return;
                                }
                                item = std::move(callbackQueue.front());
                                callbackQueue.pop();
                                logger->debug("Pop msg from queue...");
                            }
                            auto &json = item.payload;
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
                            if (item.messageId > 0) {
                                logger->debug("reply callback...");
                                client->sendMessage(nlohmann::json{
                                    {"type", "callbackReply"},
                                    {"result", std::move(resultJson)},
                                }.dump(), item.messageId);
                            }
                        });
                    }
                } else {
                    logger->error("callbackId not found: {}", callbackId);
                }
        }
    }

    void initSocket(Napi::Env &env) {
        try {
            if (client && client->IsConnected()) {
                logger->info("Already connected to server.");
                return;
            }
            client = std::make_shared<SkylineClient::ClientSocket>();
            logger->info("Connecting to server...");
            std::string address = "127.0.0.1";
            client->Init(address, 3001);
            logger->info("Connected to server, starting handshake...");

            // Copy the shared_ptr into a local variable so the thread lambda
            // can capture it by value (static variables cannot be captured).
            // This keeps the ref count > 0 while the thread is running,
            // preventing ~ClientSocket() from resetting the vtable to the
            // abstract base (which would cause "pure virtual method called").
            auto clientLocal = client;
            std::thread([clientLocal]() {
                try {
                    while (true) {
                        int64_t messageId = 0;
                        std::string message = clientLocal->receiveMessage(&messageId);
                        processMessage(message, messageId);
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
        if (!client || !client->IsConnected()) {
            throw std::runtime_error("Not connected to server. Call connect() first.");
        }
        if (requestId >= INT64_MAX - 1) {
            requestId = 1;
        }
        auto id = requestId;
        requestId += 2;
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
        client->sendMessage(std::move(data.dump()), id);
        logger->debug("Message sent, waiting for response: {}", id);

        auto start = std::chrono::steady_clock::now();
        auto handleOneCallback = [&]() {
            CallbackQueueItem item;
            {
                std::lock_guard<std::mutex> lock(callbackQueueMutex);
                if (callbackQueue.empty()) {
                    return false;
                }
                item = std::move(callbackQueue.front());
                callbackQueue.pop();
            }

            auto &json = item.payload;
            logger->debug("Pop msg from queue, start to handle callback.");
            auto callbackId = json["callbackId"].get<int64_t>();
            auto ptr = Convert::find_callback(callbackId);
            if (ptr != nullptr) {
                logger->info("callbackId found: {}", callbackId);
                auto env = ptr->funcRef->Env();
                auto args = json["data"]["args"];
                Napi::HandleScope scope(env);
                std::vector<Napi::Value> argsVec;
                argsVec.reserve(args.size());

                for (auto &arg : args) {
                    argsVec.push_back(Convert::convertJson2Value(env, arg));
                }
                std::shared_ptr<Napi::FunctionReference> funcRef = ptr->funcRef;
                auto resultValue = funcRef->Value().Call(argsVec);

                auto resultJson = Convert::convertValue2Json(env, resultValue);
                if (item.messageId > 0) {
                    logger->info("reply callback...");
                    client->sendMessage(nlohmann::json{
                        {"type", "callbackReply"},
                        {"result", std::move(resultJson)},
                    }.dump(), item.messageId);
                }
            } else {
                logger->error("CallbackId not found: {}", callbackId);
            }
            return true;
        };

        while (true) {
            while (handleOneCallback()) {
            }

            if (futureObj.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                break;
            }

            auto delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>
                (std::chrono::steady_clock::now() - start).count();
            if (delta_ms > 5000) {
                logger->error("Operation timed out after 5 seconds, request data:\n{}", data.dump());
                throw std::runtime_error("Operation timed out after 5 seconds, request data:\n" + data.dump());
            }

            auto remain_ms = 5000 - delta_ms;
            auto wait_ms = std::min<int64_t>(remain_ms, 50);
            std::unique_lock<std::mutex> lock(socketRequestMutex);
            socketEventCv.wait_for(lock, std::chrono::milliseconds(wait_ms), [&]() {
                if (futureObj.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                    return true;
                }
                std::lock_guard<std::mutex> qLock(callbackQueueMutex);
                return !callbackQueue.empty();
            });
            if (futureObj.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                break;
            }
        }

        std::string result = futureObj.get();
        logger->debug("Received response payload length: {}", result.size());

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
        if (!client || !client->IsConnected()) {
            throw std::runtime_error("Not connected to server. Call connect() first.");
        }
        logger->info("send to server async");
        client->sendMessage(data.dump(), 0);
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
