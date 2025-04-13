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
    nlohmann::json callDynamicPropertySetSync(const std::string& instanceId, const std::string& action, nlohmann::json& data);
    nlohmann::json callDynamicPropertyGetSync(const std::string& instanceId, const std::string& action, nlohmann::json& data);
    void callDynamicAsync(const std::string& instanceId, const std::string& action, nlohmann::json& data);
    nlohmann::json registerDynamicCallbackSync(const std::string& instanceId, const std::string& action, Napi::Function& func);
    nlohmann::json registerDynamicBlockCallbackSync(const std::string& instanceId, const std::string& action, Napi::Function& func); 
    // nlohmann::json registerStaticCallbackSync(const std::string& clazz, const std::string& action, Napi::Function& func);
    nlohmann::json callCustomHandleSync(const std::string& action, nlohmann::json& data);

}

#endif

