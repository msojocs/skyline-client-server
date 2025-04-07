#ifndef __WEBSOCKET_HH__
#define __WEBSOCKET_HH__
#include <string>
#include <nlohmann/json.hpp>

namespace WebSocket {

    /**
     * 初始化WebSocket，并连接到服务器
     */
    void initWebSocket();

    /**
     * 发送消息到服务器，并等待回复
     */
    std::string sendMessageSync(const std::string& clazz, const std::string& action, nlohmann::json& data);
    
}

#endif

