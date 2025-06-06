
#include "gesture_handler_module.hh"
#include "../../common/protobuf_converter.hh"
#include "../socket_client.hh"

namespace Skyline {
namespace GestureHandlerModule {
    Napi::Value sendToServerSync(const Napi::CallbackInfo &info, const std::string &methodName) {
    auto env = info.Env();
    std::vector<skyline::Value> args;
    for (int i = 0; i < info.Length(); i++) {
      args.push_back(ProtobufConverter::napiToProtobufValue(env, info[i]));
    }
    try {
      auto result = SocketClient::callStaticSync("SkylineGestureModule", methodName, args);
      return ProtobufConverter::protobufValueToNapi(env, result);
    } catch (const std::exception &e) {
      throw Napi::Error::New(env, e.what());
    }
    catch (...) {
      throw Napi::Error::New(env, "Unknown error occurred");
    }
  }
  /**
   * attachGestureHandler: ƒ attachGestureHandler()
attachNativeViewGestureHandler: ƒ attachNativeViewGestureHandler()
dropGestureHandler: ƒ dropGestureHandler()
proxyScrollViewGestureHandler: ƒ proxyScrollViewGestureHandler()
updateGestureHandler: ƒ updateGestureHandler()
   */
  Napi::Value attachGestureHandler(const Napi::CallbackInfo &info) {
    return sendToServerSync(info, __func__);
  }
  Napi::Value attachNativeViewGestureHandler(const Napi::CallbackInfo &info) {
    return sendToServerSync(info, __func__);
  }
  Napi::Value dropGestureHandler(const Napi::CallbackInfo &info) {
    return sendToServerSync(info, __func__);
  }
  Napi::Value proxyScrollViewGestureHandler(const Napi::CallbackInfo &info) {
    return sendToServerSync(info, __func__);
  }
  Napi::Value updateGestureHandler(const Napi::CallbackInfo &info) {
    return sendToServerSync(info, __func__);
  }
  void Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "attachGestureHandler"), Napi::Function::New(env, attachGestureHandler));
    exports.Set(Napi::String::New(env, "updateGestureHandler"), Napi::Function::New(env, updateGestureHandler));
    exports.Set(Napi::String::New(env, "dropGestureHandler"), Napi::Function::New(env, dropGestureHandler));
    exports.Set(Napi::String::New(env, "proxyScrollViewGestureHandler"), Napi::Function::New(env, proxyScrollViewGestureHandler));
    exports.Set(Napi::String::New(env, "attachNativeViewGestureHandler"), Napi::Function::New(env, attachNativeViewGestureHandler));
  }
}

}
