#include <chrono>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>
#include <map>
#include <queue>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <future>
#include <memory>
#include <string>
#include <napi.h>
#include <thread>
#include "../common/snowflake.hh"
#include "../common/convert.hh"
#include "../common/logger.hh"
using Logger::logger;

namespace WebSocket {
    static ix::WebSocket webSocket;
    static std::map<std::string, std::shared_ptr<std::promise<std::string>>> wsRequest;

    using snowflake_t = snowflake<1534832906275L>;
    snowflake_t serverCommunicationUuid;
    std::string serverUrl = "ws://127.0.0.1:3001"; // Added server URL variable
    std::queue<nlohmann::json> callbackQueue;
    bool blocked = false;

    void processMessage(std::string &message) {

        logger->info("received message: {}", message);
        // Logger::log("received message: %s", msg->str.c_str());
        if (message.length() == 0) {
            logger->error("received message is empty!");
            return;
        }
        nlohmann::json json = nlohmann::json::parse(message);
        if (json["type"].empty() && json.contains("id"))
        {
            // type 为空, id不空，是回复
            auto id = json["id"];
            logger->info("received message: {}", json["id"].get<std::string>());
            if (wsRequest.find(id) != wsRequest.end())
            {
                wsRequest[id]->set_value(message);
                wsRequest.erase(id);
            }
            else {
                logger->error("id not found: {}", id.get<std::string>());
            }
        }
        else if(!json["type"].empty())
        {
            if (blocked) {
                // 如果当前处于阻塞状态，直接放入队列中
                logger->info("blocked, push to queue...");
                callbackQueue.push(json);
                return;
            }
            // 通知消息
            std::string type = json["type"].get<std::string>();
            if (type == "emitCallback") {
                // 回调触发
                std::string callbackId = json["callbackId"].get<std::string>();
                // 触发回调函数
                auto ptr = Convert::find_callback(callbackId);
                if (ptr != nullptr) {
                    logger->info("callbackId found: {}", callbackId);
                    // 在正确的线程上调用回调
                    logger->info("create ThreadSafeFunction success!");
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
                            // auto consoleLog = env.Global().Get("console").As<Napi::Object>().Get("log").As<Napi::Function>();
                            // consoleLog.Call({Napi::String::New(env, "Callback function start..." + callbackId)});
                            auto resultValue = jsCallback.Call(argsVec);
                            // consoleLog.Call({Napi::String::New(env, "Callback function end!" + callbackId)});
                            logger->info("call callback function end...");
                            // 发送回调结果
                            auto resultJson = Convert::convertValue2Json(env, resultValue);
                            if (json.contains("id")) {
                                logger->info("reply callback...");
                                nlohmann::json callbakcResult = {
                                    {"id", json["id"].get<std::string>()},
                                    {"type", "callbackReply"},
                                    {"result", resultJson},
                                };
                                webSocket.send(callbakcResult.dump());
                            }
                        });
                    }
                    else {
                        ptr->tsfn.BlockingCall([json](Napi::Env env, Napi::Function jsCallback) {
                            
                            auto args = json["data"]["args"];
                            std::string callbackId = json["callbackId"].get<std::string>();
                            Napi::HandleScope scope(env);
                            std::vector<Napi::Value> argsVec;
                            
                            for (auto& arg : args) {
                                argsVec.push_back(Convert::convertJson2Value(env, arg));
                            }
                            logger->info("call callback function...");
                            // auto consoleLog = env.Global().Get("console").As<Napi::Object>().Get("log").As<Napi::Function>();
                            // consoleLog.Call({Napi::String::New(env, "Callback function start..." + callbackId)});
                            auto resultValue = jsCallback.Call(argsVec);
                            // consoleLog.Call({Napi::String::New(env, "Callback function end!" + callbackId)});
                            logger->info("call callback function end...");
                            // 发送回调结果
                            auto resultJson = Convert::convertValue2Json(env, resultValue);
                            if (json.contains("id")) {
                                logger->info("reply callback...");
                                nlohmann::json callbakcResult = {
                                    {"id", json["id"].get<std::string>()},
                                    {"type", "callbackReply"},
                                    {"result", resultJson},
                                };
                                webSocket.send(callbakcResult.dump());
                            }
                        });
                    }
                }
                else {
                    logger->error("callbackId not found: {}", callbackId);
                }
            }
        }
    }
    void initWebSocket(Napi::Env &env) {
        ix::initNetSystem();
        serverCommunicationUuid.init(1, 1);
        // Connect to a server with encryption
        // See https://machinezone.github.io/IXWebSocket/usage/#tls-support-and-configuration
        std::string url(serverUrl);
        webSocket.setUrl(url);
        // Setup a callback to be fired (in a background thread, watch out for race conditions !)
        // when a message or an event (open, close, error) is received
        webSocket.setOnMessageCallback([](const ix::WebSocketMessagePtr& msg)
            {
                if (msg->type == ix::WebSocketMessageType::Message)
                {
                    std::string message = msg->str;
                    processMessage(message);
                }
                else if (msg->type == ix::WebSocketMessageType::Open)
                {
                    logger->info("Connection established");
                }
                else if (msg->type == ix::WebSocketMessageType::Error)
                {
                    // Maybe SSL is not configured properly
                    logger->error("Connection error: {}", msg->errorInfo.reason);
                }
            }
        );
        logger->info("start");
        // Now that our callback is setup, we can start our background thread and receive messages
        webSocket.start();
        logger->info("start end");
        for (int i=0; i< 10; i++) {
            #ifdef _WIN32
            Sleep(500);
            #else
            usleep(500000);
            #endif
            if (webSocket.getReadyState() == ix::ReadyState::Open)
            {
                return;
            }
        }
        throw Napi::Error::New(env, "WebSocket connection failed");
    }
    nlohmann::json sendMessageSync(nlohmann::json& data) {
        std::string id = std::to_string(serverCommunicationUuid.nextid());
        data["id"] = id;
        logger->info("send to server {}", data.dump());
        if (webSocket.getReadyState() != ix::ReadyState::Open)
        {
            throw std::runtime_error("WebSocket is not open");
        }
        blocked = true;
        webSocket.send(data.dump());
        auto promiseObj = std::make_shared<std::promise<std::string>>();
        std::future<std::string> futureObj = promiseObj->get_future();
        wsRequest.emplace(id, promiseObj);
        auto start = std::chrono::steady_clock::now();
        while (true) {
            // logger->info("callbackQueue size: {}", callbackQueue.size());
            if (callbackQueue.size() > 0) {
                auto json = callbackQueue.front();
                callbackQueue.pop();
                logger->info("start to handle callback.");
                std::string callbackId = json["callbackId"].get<std::string>();
                auto ptr = Convert::find_callback(callbackId);
                if (ptr != nullptr) {
                    logger->info("callbackId found: {}", callbackId);
                    // 在正确的线程上调用回调
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

                    // 发送回调结果
                    auto resultJson = Convert::convertValue2Json(env, resultValue);
                    if (json.contains("id")) {
                        logger->info("reply callback...");
                        nlohmann::json callbakcResult = {
                            {"id", json["id"].get<std::string>()},
                            {"type", "callbackReply"},
                            {"result", resultJson},
                        };
                        webSocket.send(callbakcResult.dump());
                    }
                }
                else {
                    logger->error("callbackId not found: {}", callbackId);
                }
            }

            // Implement timeout handling logic here
            auto delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
            if (delta_ms > 5000) {
                blocked = false;
                throw std::runtime_error("Operation timed out after 3 seconds, request data:\n" + data.dump());
            }
            
            // Implement timeout handling logic here
            if (futureObj.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                break;
            }
        }
        std::string result = futureObj.get();
        blocked = false;
        logger->info("result: {}", result.c_str());
        if (result.length() == 0) {
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
        if (webSocket.getReadyState() != ix::ReadyState::Open)
        {
            throw std::runtime_error("WebSocket is not open");
        }
        webSocket.send(data.dump());
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