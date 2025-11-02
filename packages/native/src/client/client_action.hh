#ifndef __SOCKET_CLIENT_HH__
#define __SOCKET_CLIENT_HH__
#include <string>
#include <nlohmann/json.hpp>
#include <napi.h>

namespace ClientAction {
    /**
     * 初始化Socket，并连接到服务器
     */
    void initSocket(Napi::Env &env);
    nlohmann::json callConstructorSync(const std::string& clazz, nlohmann::json& data);
    nlohmann::json callStaticSync(const std::string& clazz, const std::string& action, nlohmann::json& data);
    nlohmann::json callDynamicSync(int64_t instanceId, const std::string& action, nlohmann::json& data);
    nlohmann::json callDynamicPropertySetSync(int64_t instanceId, const std::string& action, nlohmann::json& data);
    nlohmann::json callDynamicPropertyGetSync(int64_t instanceId, const std::string& action);
    void callDynamicAsync(int64_t instanceId, const std::string& action, nlohmann::json& data);
    nlohmann::json callCustomHandleSync(const std::string& action, nlohmann::json& data);
}

#endif // __SOCKET_CLIENT_HH__
