#ifndef __PROTOBUF_CONVERTER_HH__
#define __PROTOBUF_CONVERTER_HH__

#include <string>
#include <napi.h>
#include "messages.pb.h"

namespace ProtobufConverter {

// Napi::Value与Protobuf Value之间的转换
skyline::Value napiToProtobufValue(Napi::Env& env, const Napi::Value& value);
Napi::Value protobufValueToNapi(Napi::Env& env, const skyline::Value& value);

// 消息序列化和反序列化
std::string serializeMessage(const skyline::Message& message);
bool deserializeMessage(const std::string& data, skyline::Message& message);

// 创建各种类型的消息
skyline::Message createConstructorMessage(const std::string& clazz, const std::vector<skyline::Value>& params);
skyline::Message createStaticMessage(const std::string& clazz, const std::string& action, const std::vector<skyline::Value>& params);
skyline::Message createDynamicMessage(const std::string& instanceId, const std::string& action, const std::vector<skyline::Value>& params);
skyline::Message createDynamicPropertyMessage(const std::string& instanceId, const std::string& action, skyline::PropertyAction propertyAction, const std::vector<skyline::Value>& params);
skyline::Message createCallbackMessage(const std::string& callbackId, const std::vector<skyline::Value>& args, bool block);
skyline::Message createCallbackReplyMessage(const skyline::Value& result);
skyline::Message createResponseMessage(const skyline::Value& returnValue, const std::string& instanceId = "", const std::string& error = "");

}

#endif // __PROTOBUF_CONVERTER_HH__
