#include "./skyline_debug_info.hh"
#include <string>
#include "socket_client.hh"
#include "../common/protobuf_converter.hh"
#include "../common/logger.hh"

using Logger::logger;
namespace SkylineDebugInfo{
    void InitSkylineDebugInfo(Napi::Env env, Napi::Object exports) {
    
    // Protobuf 版本的主函数
    Napi::Function func = Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        try {
            logger->info("Call GetVersion with Protobuf...");
            std::vector<skyline::Value> reqData; // 使用空参数数组
            auto result = SocketClient::callStaticSync("global", "SkylineDebugInfo", reqData);
            logger->info("Protobuf SkylineDebugInfo result: {}", result.DebugString());
            // 转换 protobuf Value 到 Napi::Value
            auto env = info.Env();
            auto obj = ProtobufConverter::protobufValueToNapi(env, result);
            
            // 如果结果是对象，添加协议标识
            if (obj.IsObject()) {
                auto objRef = obj.As<Napi::Object>();
                objRef.Set("protocol", Napi::String::New(env, "Protobuf"));
            }
            
            return obj;
        }
        catch (const std::exception& e) {
            spdlog::error("Protobuf Error: {}", e.what());
            throw Napi::Error::New(info.Env(), e.what());
        } catch (...) {
            spdlog::error("Unknown error occurred during Protobuf SkylineDebugInfo.");
            throw Napi::Error::New(info.Env(), "Unknown error occurred during Protobuf SkylineDebugInfo.");
        }
    });
    // 使用Object.defineProperty为global对象添加getter
    Napi::Object global = env.Global();
    Napi::Object object = global.Get("Object").As<Napi::Object>();
    Napi::Function defineProperty = object.Get("defineProperty").As<Napi::Function>();
    
    // 设置主要的 Protobuf 版本为 SkylineDebugInfo
    Napi::Object descriptor = Napi::Object::New(env);
    descriptor.Set("get", func);
    descriptor.Set("enumerable", Napi::Boolean::New(env, true));
    
    defineProperty.Call({global, Napi::String::New(env, "SkylineDebugInfo"), descriptor});

    }
}
