#include "./skyline_debug_info.hh"
#include <string>
#include "websocket.hh"
#include <spdlog/spdlog.h>

namespace SkylineDebugInfo{
    void InitSkylineDebugInfo(Napi::Env env, Napi::Object exports) {
    
    // 创建一个函数，每次调用时返回最新的时间戳
    Napi::Function func = Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        try {
            spdlog::info("Call GetVersion sub...");
            nlohmann::json reqData;
            auto result = WebSocket::callStaticSync("global", "SkylineDebugInfo", reqData);
            auto returnValue = result["returnValue"];
            
            auto gitRev = returnValue["skyline_git_rev"].get<std::string>();
            auto flutterEngineRev = returnValue["flutter_engine_git_rev"].get<std::string>();
            auto skylineVersion = returnValue["skyline_version"].get<std::string>();
            auto obj = Napi::Object::New(info.Env());
            obj.Set("skyline_git_rev", Napi::String::New(info.Env(), gitRev));
            obj.Set("flutter_engine_git_rev", Napi::String::New(info.Env(), flutterEngineRev));
            obj.Set("skyline_version", Napi::String::New(info.Env(), skylineVersion));
            return obj;
        }
        catch (const std::exception& e) {
            spdlog::error("Error: {}", e.what());
            throw Napi::Error::New(info.Env(), e.what());
        } catch (...) {
            spdlog::error("Unknown error occurred during SkylineDebugInfo.");
            throw Napi::Error::New(info.Env(), "Unknown error occurred during SkylineDebugInfo.");
        }
    });
    // 使用Object.defineProperty为global对象添加一个getter
    Napi::Object global = env.Global();
    Napi::Object object = global.Get("Object").As<Napi::Object>();
    Napi::Function defineProperty = object.Get("defineProperty").As<Napi::Function>();
    
    Napi::Object descriptor = Napi::Object::New(env);
    descriptor.Set("get", func);
    descriptor.Set("enumerable", Napi::Boolean::New(env, true));
    
    defineProperty.Call({global, Napi::String::New(env, "SkylineDebugInfo"), descriptor});

    }
}
