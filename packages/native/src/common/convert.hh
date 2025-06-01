#ifndef __CONVERT_HH__
#define __CONVERT_HH__
#include <napi.h>
#include "messages.pb.h"
#include "snowflake.hh"

namespace Convert {
struct CallbackData {
  std::shared_ptr<Napi::FunctionReference> funcRef;
  Napi::ThreadSafeFunction tsfn;
};

using snowflake_t = snowflake<1534832906275L>;

void RegisteInstanceType(Napi::Env &env);
// Protobuf conversion functions
Napi::Object convertProtobufToJs(Napi::Env &env, const skyline::Message &message);
skyline::Message convertJsToProtobuf(Napi::Env &env, const Napi::Object &obj);
skyline::MessageType stringToMessageType(const std::string &typeStr);
std::string messageTypeToString(skyline::MessageType type);
// find
CallbackData * find_callback(const std::string &callbackId);
// Helper functions for protobuf converter
snowflake_t& getCallbackUuid();
void storeCallback(const std::string& callbackId, const CallbackData& data);
#ifdef _SKYLINE_CLIENT_
Napi::Value createInstanceFromProtobuf(Napi::Env& env, const std::string& instanceId, const std::string& instanceType);
#endif
} // namespace Convert

#endif