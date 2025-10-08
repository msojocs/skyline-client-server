#ifndef __MEMORY_CLIENT_HH__
#define __MEMORY_CLIENT_HH__
#include <string>
#include <napi.h>
#include "messages.pb.h"

namespace ClientAction {
    /**
     * 初始化Socket，并连接到服务器
     */
    void initSocket(Napi::Env &env);
    
    // Protobuf接口（原名称）
    skyline::Value callConstructorSync(const std::string& clazz, const std::vector<skyline::Value>& params);
    skyline::Value callStaticSync(const std::string& clazz, const std::string& action, const std::vector<skyline::Value>& params);
    skyline::Value callDynamicSync(const std::string& instanceId, const std::string& action, const std::vector<skyline::Value>& params);
    skyline::Value callDynamicPropertySetSync(const std::string& instanceId, const std::string& action, const std::vector<skyline::Value>& params);
    skyline::Value callDynamicPropertyGetSync(const std::string& instanceId, const std::string& action);
    void callDynamicAsync(const std::string& instanceId, const std::string& action, const std::vector<skyline::Value>& params);
    skyline::Value callCustomHandleSync(const std::string& action, const std::vector<skyline::Value>& params);
    
    // 底层消息发送接口
    skyline::Message sendMessageSync(const skyline::Message& message);
    void sendMessageAsync(const skyline::Message& message);
}

#endif // __MEMORY_CLIENT_HH__
