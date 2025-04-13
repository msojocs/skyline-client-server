#ifndef __CONVERT_HH__
#define __CONVERT_HH__
#include <napi.h>
#include <nlohmann/json.hpp>

namespace Convert {
struct CallbackData {
  std::shared_ptr<Napi::FunctionReference> funcRef;
  Napi::ThreadSafeFunction tsfn;
};
nlohmann::json convertValue2Json(Napi::Env &env, const Napi::Value &value);
nlohmann::json convertValue2Json(Napi::Env &env, const Napi::Value &value, bool isSyncCallback);
Napi::Value convertJson2Value(Napi::Env &env, const nlohmann::json &data);
void RegisteInstanceType(Napi::Env &env);
void remove_callback(const std::string &callbackId);
// find
CallbackData * find_callback(const std::string &callbackId);
} // namespace Convert

#endif