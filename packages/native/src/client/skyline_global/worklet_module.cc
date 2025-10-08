#include "worklet_module.hh"
#include "../client_action.hh"
#include "../../common/protobuf_converter.hh"
#include "napi.h"

namespace Skyline {
namespace WorkletModule {  Napi::Value sendToServerSync(const Napi::CallbackInfo &info, const std::string &methodName) {
    auto env = info.Env();
    std::vector<skyline::Value> args;
    for (int i = 0; i < info.Length(); i++) {
      args.push_back(ProtobufConverter::napiToProtobufValue(env, info[i]));
    }
    try {
      auto result = ClientAction::callStaticSync("SkylineWorkletModule", methodName, args);
      return ProtobufConverter::protobufValueToNapi(env, result);
    } catch (const std::exception &e) {
      throw Napi::Error::New(env, e.what());
    }
    catch (...) {
      throw Napi::Error::New(env, "Unknown error occurred");
    }
  }
  Napi::Value installCoreFunctions(const Napi::CallbackInfo &info) {
    return sendToServerSync(info, __func__);
  }
  Napi::Value makeShareable(const Napi::CallbackInfo &info) {
    return sendToServerSync(info, __func__);
  }  Napi::Value makeMutable(const Napi::CallbackInfo &info) {
    return sendToServerSync(info, __func__);
  }  Napi::Value registerEventHandler(const Napi::CallbackInfo &info) {
    return sendToServerSync(info, __func__);
  }
  Napi::Value unregisterEventHandler(const Napi::CallbackInfo &info) {
    return sendToServerSync(info, __func__);
  }
  Napi::Value startMapper(const Napi::CallbackInfo &info) {
    return sendToServerSync(info, __func__);
  }
  Napi::Value stopMappers(const Napi::CallbackInfo &info) {
    return sendToServerSync(info, __func__);
  }
  Napi::Value makeRemote(const Napi::CallbackInfo &info) {
    return sendToServerSync(info, __func__);
  }
  void Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "installCoreFunctions"), Napi::Function::New(env, installCoreFunctions));
    exports.Set(Napi::String::New(env, "makeShareable"), Napi::Function::New(env, makeShareable));
    exports.Set(Napi::String::New(env, "makeMutable"), Napi::Function::New(env, makeMutable));
    exports.Set(Napi::String::New(env, "registerEventHandler"), Napi::Function::New(env, registerEventHandler));
    exports.Set(Napi::String::New(env, "unregisterEventHandler"), Napi::Function::New(env, unregisterEventHandler));
    exports.Set(Napi::String::New(env, "startMapper"), Napi::Function::New(env, startMapper));
    exports.Set(Napi::String::New(env, "stopMappers"), Napi::Function::New(env, stopMappers));
    exports.Set(Napi::String::New(env, "makeRemote"), Napi::Function::New(env, makeRemote));
  }
}
}