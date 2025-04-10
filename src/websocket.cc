#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>
#include <map>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <future>
#include <memory>
#include <string>
#include <synchapi.h>
#include <napi.h>
#include "snowflake.hh"
#include "include/convert.hh"

namespace WebSocket {
    static ix::WebSocket webSocket;
    static std::map<std::string, std::shared_ptr<std::promise<std::string>>> wsRequest;
    static std::map<std::string, Napi::ThreadSafeFunction> callback;

    using snowflake_t = snowflake<1534832906275L>;
    snowflake_t serverCommunicationUuid;
    snowflake_t callbackUuid;
    std::string serverUrl = "ws://127.0.0.1:3001"; // Added server URL variable 

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
                    spdlog::info("received message: {}", msg->str);
                    nlohmann::json json = nlohmann::json::parse(msg->str);
                    if (json["type"].empty() && json.contains("id"))
                    {
                        // type 为空, id不空，是回复
                        auto id = json["id"];
                        spdlog::info("received message: {}", json["id"].get<std::string>());
                        if (wsRequest.find(id) != wsRequest.end())
                        {
                            wsRequest[id]->set_value(msg->str);
                            wsRequest.erase(id);
                        }
                    }
                    else if(!json["type"].empty())
                    {
                        // 通知消息
                        std::string type = json["type"].get<std::string>();
                        if (type == "emitCallback") {
                            // 回调触发
                            std::string callbackId = json["callbackId"].get<std::string>();
                            // TODO: 触发回调函数
                            auto it = callback.find(callbackId);
                            if (it != callback.end()) {
                                spdlog::info("callbackId found: {}", callbackId);
                                // 在正确的线程上调用回调
                                it->second.BlockingCall([json](Napi::Env env, Napi::Function jsCallback) {
                                    
                                    auto args = json["data"]["args"];
                                    Napi::HandleScope scope(env);
                                    std::vector<Napi::Value> argsVec;
                                    
                                    for (auto& arg : args) {
                                        argsVec.push_back(Convert::convertJson2Value(env, arg));
                                    }
                                    spdlog::info("call callback function...");
                                    auto resultValue = jsCallback.Call(argsVec);
                                    spdlog::info("call callback function end...");
                                    // 发送回调结果
                                    auto resultJson = Convert::convertValue2Json(resultValue);
                                    if (json.contains("id")) {
                                        spdlog::info("reply callback...");
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
                                spdlog::error("callbackId not found: {}", callbackId);
                            }
                        }
                    }
                }
                else if (msg->type == ix::WebSocketMessageType::Open)
                {
                    spdlog::info("Connection established");
                }
                else if (msg->type == ix::WebSocketMessageType::Error)
                {
                    // Maybe SSL is not configured properly
                    spdlog::error("Connection error: {}", msg->errorInfo.reason);
                }
            }
        );
        spdlog::info("start");
        // Now that our callback is setup, we can start our background thread and receive messages
        webSocket.start();
        spdlog::info("start end");
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
        spdlog::info("send to server {}", data.dump());
        if (webSocket.getReadyState() != ix::ReadyState::Open)
        {
            throw std::runtime_error("WebSocket is not open");
        }
        webSocket.send(data.dump());
        auto promiseObj = std::make_shared<std::promise<std::string>>();
        std::future<std::string> futureObj = promiseObj->get_future();
        wsRequest.emplace(id, promiseObj);
        // TODO: 3秒超时
        std::string result = futureObj.get();
        spdlog::info("result: {}", result.c_str());
        auto resp = nlohmann::json::parse(result);
        if (resp.contains("error")) {
            throw std::runtime_error(resp["error"].get<std::string>());
        }
        return resp["result"];
    }
    void sendMessageAsync(nlohmann::json& data) {
        std::string id = std::to_string(serverCommunicationUuid.nextid());
        data["id"] = id;
        spdlog::info("send to server {}", data.dump());
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
        callback.emplace(callbackId, Napi::ThreadSafeFunction::New(
            func.Env(),
            func,
            "WebSocket Callback",
            0,
            1));
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
        callback.emplace(callbackId, Napi::ThreadSafeFunction::New(
            func.Env(),
            func,
            "WebSocket Callback",
            0,
            1));
        return result;
    }
    nlohmann::json registerStaticCallbackSync(const std::string& clazz, const std::string& action, Napi::Function& func) {
        std::string callbackId = std::to_string(callbackUuid.nextid());
        nlohmann::json json {
            {"type", "registerCallback"},
            {"action", action},
            {"data", {
                {"clazz", clazz},
                {"callbackId", callbackId}
            }}
        };
        // 把callbackId保存到map中，收到消息时调用Callback函数
        auto result = sendMessageSync(json);
        callback.emplace(callbackId, Napi::ThreadSafeFunction::New(
            func.Env(),
            func,
            "WebSocket Callback",
            0,
            1));
        return result;
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