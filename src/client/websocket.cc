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
#include <synchapi.h>
#include <napi.h>
#include "../include/snowflake.hh"
#include "../include/convert.hh"
#include "../include/logger.hh"
using Logger::logger;

namespace WebSocket {
    static ix::WebSocket webSocket;
    static std::map<std::string, std::shared_ptr<std::promise<std::string>>> wsRequest;
    static std::map<std::string, Convert::CallbackData> callback;

    using snowflake_t = snowflake<1534832906275L>;
    snowflake_t serverCommunicationUuid;
    snowflake_t callbackUuid;
    std::string serverUrl = "ws://127.0.0.1:3001"; // Added server URL variable
    std::queue<nlohmann::json> callbackQueue;
    bool blocked = false;

    void initWebSocket() {
        ix::initNetSystem();
        serverCommunicationUuid.init(1, 1);
        callbackUuid.init(2, 1);
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
                    logger->info("received message: {}", msg->str);
                    // Logger::log("received message: %s", msg->str.c_str());
                    nlohmann::json json = nlohmann::json::parse(msg->str);
                    if (json["type"].empty() && json.contains("id"))
                    {
                        // type 为空, id不空，是回复
                        auto id = json["id"];
                        logger->info("received message: {}", json["id"].get<std::string>());
                        if (wsRequest.find(id) != wsRequest.end())
                        {
                            wsRequest[id]->set_value(msg->str);
                            wsRequest.erase(id);
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
                            auto it = callback.find(callbackId);
                            if (ptr == nullptr) {
                                if (it == callback.end()) {
                                    logger->error("callbackId not found: {}", callbackId);
                                    return;
                                }
                                ptr = &it->second;
                            }
                            if (ptr != nullptr) {
                                logger->info("callbackId found: {}", callbackId);
                                // 在正确的线程上调用回调
                                logger->info("create ThreadSafeFunction success!");
                                auto callResult = ptr->tsfn.BlockingCall([json](Napi::Env env, Napi::Function jsCallback) {
                                    
                                    auto args = json["data"]["args"];
                                    std::string callbackId = json["callbackId"].get<std::string>();
                                    Napi::HandleScope scope(env);
                                    std::vector<Napi::Value> argsVec;
                                    
                                    for (auto& arg : args) {
                                        argsVec.push_back(Convert::convertJson2Value(env, arg));
                                    }
                                    logger->info("call callback function...");
                                    auto consoleLog = env.Global().Get("console").As<Napi::Object>().Get("log").As<Napi::Function>();
                                    consoleLog.Call({Napi::String::New(env, "Callback function start..." + callbackId)});
                                    auto resultValue = jsCallback.Call(argsVec);
                                    consoleLog.Call({Napi::String::New(env, "Callback function end!" + callbackId)});
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
                                logger->error("callbackId not found: {}", callbackId);
                            }
                        }
                    }
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
        for (int i=0; i< 60; i++) {
            Sleep(1000);
            if (webSocket.getReadyState() == ix::ReadyState::Open)
            {
                return;
            }
        }
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
        while (true) {
            // logger->info("callbackQueue size: {}", callbackQueue.size());
            if (callbackQueue.size() > 0) {
                auto json = callbackQueue.front();
                callbackQueue.pop();
                logger->info("start to handle callback.");
                std::string callbackId = json["callbackId"].get<std::string>();
                auto ptr = Convert::find_callback(callbackId);
                auto it = callback.find(callbackId);
                if (ptr == nullptr) {
                    if (it == callback.end()) {
                        logger->error("callbackId not found: {}", callbackId);
                        continue;
                    }
                    
                    ptr = &it->second;
                }
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
            if (futureObj.wait_for(std::chrono::milliseconds(10)) == std::future_status::ready) {
                break;
            }
        }
        // TODO: 3秒超时
        std::string result = futureObj.get();
        blocked = false;
        logger->info("result: {}", result.c_str());
        auto resp = nlohmann::json::parse(result);
        if (resp.contains("error")) {
            throw std::runtime_error(resp["error"].get<std::string>());
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
    nlohmann::json callDynamicPropertyGetSync(const std::string& instanceId, const std::string& action, nlohmann::json& args) {
        nlohmann::json json {
            {"type", "dynamicProperty"},
            {"action", action},
            {"data", {
                {"instanceId", instanceId},
                {"params", args},
                {"propertyAction", "get"},
            }}
        };
        
        return sendMessageSync(json);
    }
    nlohmann::json registerDynamicCallbackSync(const std::string& instanceId, const std::string& action, Napi::Function& func) {
        std::string callbackId = std::to_string(callbackUuid.nextid());
        nlohmann::json json {
            {"type", "registerCallback"},
            {"action", action},
            {"data", {
                {"instanceId", instanceId},
                {"callbackId", callbackId}
            }}
        };
        // 把callbackId保存到map中，收到消息时调用Callback函数
        auto result = sendMessageSync(json);
        auto tsfn = Napi::ThreadSafeFunction::New(func.Env(), func, "WebSocket Callback", 0, 1);
        callback.emplace(callbackId, Convert::CallbackData{std::make_shared<Napi::FunctionReference>(Napi::Persistent(func)), tsfn});
        return result;
    }
    nlohmann::json registerDynamicBlockCallbackSync(const std::string& instanceId, const std::string& action, Napi::Function& func) {
        std::string callbackId = std::to_string(callbackUuid.nextid());
        nlohmann::json json {
            {"type", "registerCallback"},
            {"action", action},
            {"data", {
                {"instanceId", instanceId},
                {"callbackId", callbackId},
                {"syncCallback", true}
            }}
        };
        // 把callbackId保存到map中，收到消息时调用Callback函数
        auto result = sendMessageSync(json);
        auto tsfn = Napi::ThreadSafeFunction::New(func.Env(), func, "WebSocket Callback", 0, 1);
        callback.emplace(callbackId, Convert::CallbackData{std::make_shared<Napi::FunctionReference>(Napi::Persistent(func)), tsfn});
        return result;
    }
    // nlohmann::json registerStaticCallbackSync(const std::string& clazz, const std::string& action, Napi::Function& func) {
    //     std::string callbackId = std::to_string(callbackUuid.nextid());
    //     nlohmann::json json {
    //         {"type", "registerCallback"},
    //         {"action", action},
    //         {"data", {
    //             {"clazz", clazz},
    //             {"callbackId", callbackId}
    //         }}
    //     };
    //     // 把callbackId保存到map中，收到消息时调用Callback函数
    //     auto result = sendMessageSync(json);
    //     auto tsfn = Napi::ThreadSafeFunction::New(func.Env(), func, "WebSocket Callback", 0, 1);
    //     callback.emplace(callbackId, Convert::CallbackData{std::make_shared<Napi::FunctionReference>(Napi::Persistent(func)), tsfn});
    //     return result;
    // }
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