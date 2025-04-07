#include "./skyline_debug_info.hh"
#include <string>
#include <ctime>
#include "websocket.hh"
#include <spdlog/spdlog.h>

namespace SkylineDebugInfo{
    void InitSkylineDebugInfo(Napi::Env env, Napi::Object exports) {
    
    // 创建一个函数，每次调用时返回最新的时间戳
    Napi::Function func = Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        time_t now = time(nullptr);
        spdlog::info("Call GetVersion sub...");
        nlohmann::json resp;
        auto result = WebSocket::sendMessageSync("SkylineDebugInfo", "", resp);
        auto d = resp.parse(result);
        auto data = d["data"];
        auto obj = Napi::Object::New(info.Env());
        obj.Set("skyline_git_rev", Napi::String::New(info.Env(), data["skyline_git_rev"].get<std::string>()));
        obj.Set("flutter_engine_git_rev", Napi::String::New(info.Env(), data["flutter_engine_git_rev"].get<std::string>()));
        obj.Set("skyline_version", Napi::String::New(info.Env(), data["skyline_version"].get<std::string>()));
        return obj;
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
