#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <napi.h>
#include <cstdlib>
#include <ctime>
#include "websocket.hh"
class VersionGetter : public Napi::ObjectWrap<VersionGetter> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "VersionGetter", {
      InstanceAccessor("SkylineDebugInfo", &VersionGetter::GetVersion, nullptr, napi_enumerable),
    });

    // 创建一个实例并设置到全局对象
    Napi::Object instance = func.New({});
    env.Global().Set(Napi::String::New(env, "SkylineDebugInfo"), instance.Get(Napi::String::New(env, "SkylineDebugInfo")));

    return exports;
  }

  VersionGetter(const Napi::CallbackInfo& info) : Napi::ObjectWrap<VersionGetter>(info) {}

private:
  Napi::Value GetVersion(const Napi::CallbackInfo& info) {
    spdlog::info("Call GetVersion...");
    time_t now = time(nullptr);
    nlohmann::json data;
    WebSocket::sendMessageSync("SkylineDebugInfo", "", data);
    return Napi::String::New(info.Env(), std::to_string(now));
  }
};

static Napi::Object Init(Napi::Env env, Napi::Object exports) {
  WebSocket::initWebSocket();

  spdlog::info("initWebSocket end");
  auto result = VersionGetter::Init(env, exports);
  spdlog::info("return result");
  return result;
}

NODE_API_MODULE(cmnative, Init)