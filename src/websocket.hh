#ifndef __WEBSOCKET_HH__
#define __WEBSOCKET_HH__
#include <string>
#include <nlohmann/json.hpp>
#include <napi.h>

namespace WebSocket {

    /**
     * 初始化WebSocket，并连接到服务器
     */
    void initWebSocket();
    
    nlohmann::json callConstructorSync(const std::string& clazz, nlohmann::json& data);
    nlohmann::json callStaticSync(const std::string& clazz, const std::string& action, nlohmann::json& data);
    nlohmann::json callDynamicSync(const std::string& instanceId, const std::string& action, nlohmann::json& data);
    nlohmann::json registerCallbackSync(const std::string& instanceId, const std::string& action, Napi::Function& func);
    nlohmann::json callCustomHandleSync(const std::string& action, nlohmann::json& data);

}

#endif

