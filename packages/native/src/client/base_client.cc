#include "base_client.hh"
#include "../common/convert.hh"
#include "../common/protobuf_converter.hh"
#include "napi.h"
#include "client_action.hh"
#include <spdlog/spdlog.h>


namespace Skyline {

Napi::Value BaseClient::getInstanceId(const Napi::CallbackInfo &info) {
    return Napi::String::New(info.Env(), m_instanceId);
}
Napi::Value BaseClient::sendToServerSync(const Napi::CallbackInfo &info,
                                         const std::string &methodName) {
    auto env = info.Env();
    std::vector<skyline::Value> args;
    for (int i = 0; i < info.Length(); i++) {
        args.push_back(ProtobufConverter::napiToProtobufValue(env, info[i]));
    }
    try {
        auto result =
            ClientAction::callDynamicSync(m_instanceId, methodName, args);
        return ProtobufConverter::protobufValueToNapi(env, result);
    } catch (const std::exception &e) {
        throw Napi::Error::New(env, e.what());
    } catch (...) {
        throw Napi::Error::New(env, "Unknown error occurred");
    }
}
Napi::Value BaseClient::sendToServerAsync(const Napi::CallbackInfo &info,
                                          const std::string &methodName) {
    auto env = info.Env();
    std::vector<skyline::Value> args;
    for (int i = 0; i < info.Length(); i++) {
        args.push_back(ProtobufConverter::napiToProtobufValue(env, info[i]));
    }
    try {
        ClientAction::callDynamicAsync(m_instanceId, methodName, args);
        return env.Undefined();
    } catch (const std::exception &e) {
        throw Napi::Error::New(env, e.what());
    } catch (...) {
        throw Napi::Error::New(env, "Unknown error occurred");
    }
}
std::string
BaseClient::sendConstructorToServerSync(const Napi::CallbackInfo &info,
                                        const std::string &className) {
    auto env = info.Env();
    std::vector<skyline::Value> args;
    for (int i = 0; i < info.Length(); i++) {
        args.push_back(ProtobufConverter::napiToProtobufValue(env, info[i]));
    }
    try {
        auto result = ClientAction::callConstructorSync(className, args);
        // The result should be a skyline::Value containing the instance ID as a
        // string
        if (result.has_string_value()) {
            return result.string_value();
        } else {
            throw Napi::Error::New(env, "No instanceId in result");
        }
    } catch (const std::exception &e) {
        throw Napi::Error::New(env, e.what());
    } catch (...) {
        throw Napi::Error::New(env, "Unknown error occurred");
    }
}
void BaseClient::setProperty(const Napi::CallbackInfo &info,
                             const std::string &propertyName) {
    auto env = info.Env();
    std::vector<skyline::Value> args;
    args.push_back(ProtobufConverter::napiToProtobufValue(env, info[0]));
    try {
        auto result = ClientAction::callDynamicPropertySetSync(
            m_instanceId, propertyName, args);
    } catch (const std::exception &e) {
        throw Napi::Error::New(env, e.what());
    } catch (...) {
        throw Napi::Error::New(env, "Unknown error occurred");
    }
}
Napi::Value BaseClient::getProperty(const Napi::CallbackInfo &info,
                                    const std::string &propertyName) {
    auto env = info.Env();
    try {
        auto result = ClientAction::callDynamicPropertyGetSync(m_instanceId,
                                                               propertyName);
        return ProtobufConverter::protobufValueToNapi(env, result);
    } catch (const std::exception &e) {
        throw Napi::Error::New(env, e.what());
    } catch (...) {
        throw Napi::Error::New(env, "Unknown error occurred");
    }
}

} // namespace Skyline