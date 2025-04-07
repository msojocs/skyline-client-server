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

namespace WebSocket {
    static ix::WebSocket webSocket;
    static std::map<std::string, std::shared_ptr<std::promise<std::string>>> wsRequest;

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
    std::string sendMessageSync(const std::string& clazz, const std::string& action, nlohmann::json& data) {
        std::string id = generate_hex(16);
        nlohmann::json json {
            {"id", id},
            {"clazz", clazz},
            {"action", action},
            {"data", data}
        };
        spdlog::info("send to server");
        if (webSocket.getReadyState() != ix::ReadyState::Open)
        {
            return "";
        }
        webSocket.send(json.dump());
        auto promiseObj = std::make_shared<std::promise<std::string>>();
        std::future<std::string> futureObj = promiseObj->get_future();
        wsRequest.emplace(id, promiseObj);
        
        std::string result = futureObj.get();
        spdlog::info("result: {}", result.c_str());
        return result;
    }
}