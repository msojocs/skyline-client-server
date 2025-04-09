#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>
#include <map>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <future>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <synchapi.h>
#include <napi.h>

namespace WebSocket {
    static ix::WebSocket webSocket;
    static std::map<std::string, std::shared_ptr<std::promise<std::string>>> wsRequest;
    static std::map<std::string, Napi::ThreadSafeFunction> callback;

    unsigned int random_char() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        return dis(gen);
    }

    std::string generate_hex(const unsigned int len) {
        std::stringstream ss;
        for (auto i = 0; i < len; i++) {
            const auto rc = random_char();
            std::stringstream hexstream;
            hexstream << std::hex << rc;
            auto hex = hexstream.str();
            ss << (hex.length() < 2 ? '0' + hex : hex);
        }
        return ss.str();
    }

    void initWebSocket() {
        ix::initNetSystem();
        // Connect to a server with encryption
        // See https://machinezone.github.io/IXWebSocket/usage/#tls-support-and-configuration
        std::string url("ws://127.0.0.1:3001");
        webSocket.setUrl(url);
         // Setup a callback to be fired (in a background thread, watch out for race conditions !)
        // when a message or an event (open, close, error) is received
        webSocket.setOnMessageCallback([](const ix::WebSocketMessagePtr& msg)
            {
                if (msg->type == ix::WebSocketMessageType::Message)
                {
                    spdlog::info("received message: {}", msg->str);
                    nlohmann::json json = nlohmann::json::parse(msg->str);
                    if (!json["id"].empty())
                    {
                        auto id = json["id"];
                        spdlog::info("received message: {}", json["id"].get<std::string>());
                        if (wsRequest.find(id) != wsRequest.end())
                        {
                            wsRequest[id]->set_value(msg->str);
                        }
                    }
                    else 
                    {
                        // id为空，说明是通知消息
                        std::string type = json["tpye"].get<std::string>();
                        if (type == "emitCallback") {
                            // 回调触发
                            std::string callbackId = json["callbackId"].get<std::string>();
                            auto args = json["tpye"]["args"];
                            // TODO: 触发回调函数
                            auto it = callback.find(callbackId);
                            if (it != callback.end()) {
                                // 在正确的线程上调用回调
                                it->second.BlockingCall([args](Napi::Env env, Napi::Function jsCallback) {
                                    Napi::HandleScope scope(env);
                                    std::vector<napi_value> argsVec;
                                    
                                    for (auto item : args.items()) {
                                        if (item.value().is_number()) {
                                            argsVec.push_back(Napi::Number::New(env, item.value().get<int>()));
                                        } else if (item.value().is_string()) {
                                            argsVec.push_back(Napi::String::New(env, item.value().get<std::string>()));
                                        } else if (item.value().is_boolean()) {
                                            argsVec.push_back(Napi::Boolean::New(env, item.value().get<bool>()));
                                        } else if (item.value().is_object()) {
                                            argsVec.push_back(Napi::String::New(env, item.value().dump()));
                                        }
                                    }
                                    
                                    jsCallback.Call(argsVec);
                                });
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
        // 生成一个随机的16位十六进制字符串作为id，把收到同id的消息返回
        std::string id = generate_hex(16);
        spdlog::info("send to server");
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
        return nlohmann::json::parse(result);
    }
    nlohmann::json callConstructorSync(const std::string& clazz, nlohmann::json& args) {
        // 生成一个随机的16位十六进制字符串作为id，把收到同id的消息返回
        std::string id = generate_hex(16);
        nlohmann::json json {
            {"id", id},
            {"type", "constructor"},
            {"clazz", clazz},
            {"data", {
                {"params", args}
            }}
        };
        
        return sendMessageSync(json);
    }
    nlohmann::json callStaticSync(const std::string& clazz, const std::string& action, nlohmann::json& args) {
        // 生成一个随机的16位十六进制字符串作为id，把收到同id的消息返回
        std::string id = generate_hex(16);
        nlohmann::json json {
            {"id", id},
            {"type", "static"},
            {"clazz", clazz},
            {"action", action},
            {"data", {
                {"params", args}
            }}
        };
        
        return sendMessageSync(json);
    }
    nlohmann::json callDynamicSync(const std::string& instanceId, const std::string& action, nlohmann::json& args) {
        // 生成一个随机的16位十六进制字符串作为id，把收到同id的消息返回
        std::string id = generate_hex(16);
        nlohmann::json json {
            {"id", id},
            {"type", "dynamic"},
            {"action", action},
            {"data", {
                {"instanceId", instanceId},
                {"params", args}
            }}
        };
        
        return sendMessageSync(json);
    }
    nlohmann::json registerCallbackSync(const std::string& instanceId, const std::string& action, Napi::Function& func) {
        // 生成一个随机的16位十六进制字符串作为id，把收到同id的消息返回
        std::string id = generate_hex(16);
        nlohmann::json json {
            {"id", id},
            {"type", "registerCallback"},
            {"action", action},
            {"data", {
                {"instanceId", instanceId},
                {"callbackId", id}
            }}
        };
        // 把callbackId保存到map中，收到消息时调用Callback函数
        auto result = sendMessageSync(json);
        callback.emplace(id, Napi::ThreadSafeFunction::New(
            func.Env(),
            func,
            "WebSocket Callback",
            0,
            1));
        return result;
    }
    nlohmann::json callCustomHandleSync(const std::string& action, nlohmann::json& args) {
        // 生成一个随机的16位十六进制字符串作为id，把收到同id的消息返回
        std::string id = generate_hex(16);
        nlohmann::json json {
            {"id", id},
            {"type", "customHandle"},
            {"action", action},
            {"data", {
                {"params", args}
            }}
        };
        
        return sendMessageSync(json);
    }
}