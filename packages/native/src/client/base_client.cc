#include "base_client.hh"
#include "napi.h"
#include "client_action.hh"
#include <spdlog/spdlog.h>
#include "../common/convert.hh"
#include "../common/logger.hh"

using Logger::logger;

namespace HTML {
  std::shared_ptr<Napi::FunctionReference> errorCallbackRef;
  
  Napi::Value BaseClient::getInstanceId(const Napi::CallbackInfo &info) {
    return Napi::Number::New(info.Env(), m_instanceId);
  }
  Napi::Value BaseClient::sendToServerSync(const Napi::CallbackInfo &info, const std::string &methodName) {
    auto env = info.Env();
    nlohmann::json args;
    for (int i = 0; i < info.Length(); i++) {
      args[i] = Convert::convertValue2Json(env, info[i]);
    }
    try {
      auto result = ClientAction::callDynamicSync(m_instanceId, methodName, args);
      auto returnValue = result["returnValue"];
      return Convert::convertJson2Value(env, returnValue);
    } catch (const std::exception &e) {
      if (errorCallbackRef && !errorCallbackRef->IsEmpty()) {
        logger->error("Error in sendToServerSync: {}", e.what());
        errorCallbackRef->Call({Napi::String::New(env, e.what())});
      }
      throw Napi::Error::New(env, e.what());
    }
    catch (...) {
      if (errorCallbackRef && !errorCallbackRef->IsEmpty()) {
        errorCallbackRef->Call({Napi::String::New(env, "Unknown error occurred")});
      }
      throw Napi::Error::New(env, "Unknown error occurred");
    }
  }
  Napi::Value BaseClient::sendToServerAsync(const Napi::CallbackInfo &info, const std::string &methodName) {
    auto env = info.Env();
    nlohmann::json args;
    for (int i = 0; i < info.Length(); i++) {
      args[i] = Convert::convertValue2Json(env, info[i]);
    }
    try {
      ClientAction::callDynamicAsync(m_instanceId, methodName, args);
      return env.Undefined();
    } catch (const std::exception &e) {
      if (errorCallbackRef && !errorCallbackRef->IsEmpty()) {
        errorCallbackRef->Call({Napi::String::New(env, e.what())});
      }
      throw Napi::Error::New(env, e.what());
    }
    catch (...) {
      if (errorCallbackRef && !errorCallbackRef->IsEmpty()) {
        errorCallbackRef->Call({Napi::String::New(env, "Unknown error occurred")});
      }
      throw Napi::Error::New(env, "Unknown error occurred");
    }
  }
  int64_t BaseClient::sendConstructorToServerSync(const Napi::CallbackInfo &info, const std::string &className) {
    auto env = info.Env();
    nlohmann::json args;
    for (int i = 0; i < info.Length(); i++) {
      args[i] = Convert::convertValue2Json(env, info[i]);
    }
    try {
      auto result = ClientAction::callConstructorSync(className, args);
      if (!result.contains("instanceId")) {
        throw Napi::Error::New(env, "No instanceId in result");
      }
      return result["instanceId"].get<int64_t>();
    } catch (const std::exception &e) {
      logger->error("Error in sendConstructorToServerSync: {}", e.what());
      if (errorCallbackRef && !errorCallbackRef->IsEmpty()) {
        logger->info("callback");
        errorCallbackRef->Call({Napi::String::New(env, e.what())});
      }
      throw Napi::Error::New(env, e.what());
    }
    catch (...) {
      logger->error("Unknown error occurred in sendConstructorToServerSync");
      if (errorCallbackRef && !errorCallbackRef->IsEmpty()) {
        errorCallbackRef->Call({Napi::String::New(env, "Unknown error occurred")});
      }
      throw Napi::Error::New(env, "Unknown error occurred");
    }
  }
  void BaseClient::setProperty(const Napi::CallbackInfo &info, const std::string &propertyName) {
    auto env = info.Env();
    nlohmann::json args;
    args[0] = Convert::convertValue2Json(env, info[0]);
    try {
      auto result = ClientAction::callDynamicPropertySetSync(m_instanceId, propertyName, args);
    } catch (const std::exception &e) {
      if (errorCallbackRef && !errorCallbackRef->IsEmpty()) {
        errorCallbackRef->Call({Napi::String::New(env, e.what())});
      }
      throw Napi::Error::New(env, e.what());
    }
    catch (...) {
      if (errorCallbackRef && !errorCallbackRef->IsEmpty()) {
        errorCallbackRef->Call({Napi::String::New(env, "Unknown error occurred")});
      }
      throw Napi::Error::New(env, "Unknown error occurred");
    }
  }
  Napi::Value BaseClient::getProperty(const Napi::CallbackInfo &info, const std::string &propertyName) {
    auto env = info.Env();
    nlohmann::json args;
    args[0] = Convert::convertValue2Json(env, info[0]);
    try {
      auto result = ClientAction::callDynamicPropertyGetSync(m_instanceId, propertyName);
      auto returnValue = result["returnValue"];
      return Convert::convertJson2Value(env, returnValue);
    } catch (const std::exception &e) {
      if (errorCallbackRef && !errorCallbackRef->IsEmpty()) {
        errorCallbackRef->Call({Napi::String::New(env, e.what())});
      }
      throw Napi::Error::New(env, e.what());
    }
    catch (...) {
      if (errorCallbackRef && !errorCallbackRef->IsEmpty()) {
        errorCallbackRef->Call({Napi::String::New(env, "Unknown error occurred")});
      }
      throw Napi::Error::New(env, "Unknown error occurred");
    }
  }

} // namespace SkylineShell