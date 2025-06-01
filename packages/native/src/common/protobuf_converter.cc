#include "protobuf_converter.hh"
#include "convert.hh"
#include <cmath>

namespace ProtobufConverter {

skyline::Value napiToProtobufValue(Napi::Env& env, const Napi::Value& value) {
    skyline::Value pbValue;
    
    if (value.IsString()) {
        pbValue.set_string_value(value.As<Napi::String>().Utf8Value());
    } else if (value.IsNumber()) {
        double num = value.As<Napi::Number>().DoubleValue();
        // Check if it's an integer
        if (num == floor(num) && num >= INT64_MIN && num <= INT64_MAX) {
            pbValue.set_int_value(static_cast<int64_t>(num));
        } else {
            pbValue.set_double_value(num);
        }
    } else if (value.IsBoolean()) {
        pbValue.set_bool_value(value.As<Napi::Boolean>().Value());
    } else if (value.IsFunction()) {
        // Handle functions by creating a special object structure
        Napi::Function func = value.As<Napi::Function>();
        skyline::ValueObject* object = pbValue.mutable_object_value();
        
        // Generate callbackId
        std::string callbackId = std::to_string(Convert::getCallbackUuid().nextid());
        (*object->mutable_fields())["callbackId"].set_string_value(callbackId);
        
        bool isSync = func.Get(Napi::String::New(env, "__syncCallback")).IsBoolean() ?
            func.Get(Napi::String::New(env, "__syncCallback")).As<Napi::Boolean>().Value() : true;
        (*object->mutable_fields())["syncCallback"].set_bool_value(isSync);
        
        // Store callback in the Convert module
        Convert::storeCallback(callbackId, {
            std::make_shared<Napi::FunctionReference>(Napi::Persistent(func)),
            Napi::ThreadSafeFunction::New(env, func, "Callback", 0, 1)
        });
        
        // Handle worklet properties if they exist
        if (func.Get("__worklet").IsBoolean()) {
            (*object->mutable_fields())["asString"].set_string_value(
                func.Get("asString").As<Napi::String>().Utf8Value());
            (*object->mutable_fields())["__workletHash"].set_int_value(
                func.Get("__workletHash").As<Napi::Number>().Int64Value());
            (*object->mutable_fields())["__location"].set_string_value(
                func.Get("__location").As<Napi::String>().Utf8Value());
            (*object->mutable_fields())["__worklet"].set_bool_value(
                func.Get("__worklet").As<Napi::Boolean>().Value());
            (*object->mutable_fields())["_closure"] = napiToProtobufValue(env, func.Get("_closure"));
        }
    } else if (value.IsBuffer()) {
        Napi::Buffer<uint8_t> buffer = value.As<Napi::Buffer<uint8_t>>();
        std::string bytes(reinterpret_cast<const char*>(buffer.Data()), buffer.Length());
        pbValue.set_bytes_value(bytes);
    } else if (value.IsArrayBuffer()) {
        Napi::ArrayBuffer arrayBuffer = value.As<Napi::ArrayBuffer>();
        std::string bytes(reinterpret_cast<const char*>(arrayBuffer.Data()), arrayBuffer.ByteLength());
        pbValue.set_bytes_value(bytes);
    } else if (value.IsArray()) {
        Napi::Array arr = value.As<Napi::Array>();
        skyline::ValueArray* array = pbValue.mutable_array_value();
        for (uint32_t i = 0; i < arr.Length(); i++) {
            *array->add_values() = napiToProtobufValue(env, arr.Get(i));
        }
    } else if (value.IsObject()) {
        Napi::Object obj = value.As<Napi::Object>();
        skyline::ValueObject* object = pbValue.mutable_object_value();
        
        // Check for special instance objects
        if (obj.Get("instanceId").IsString()) {
            (*object->mutable_fields())["instanceId"].set_string_value(
                obj.Get("instanceId").As<Napi::String>().Utf8Value());
            if (obj.Has("instanceType")) {
                (*object->mutable_fields())["instanceType"].set_string_value(
                    obj.Get("instanceType").As<Napi::String>().Utf8Value());
            }
        } else {
            // Regular object conversion
            Napi::Array propertyNames = obj.GetPropertyNames();
            for (uint32_t i = 0; i < propertyNames.Length(); i++) {
                Napi::String key = propertyNames.Get(i).As<Napi::String>();
                std::string keyStr = key.Utf8Value();
                if (keyStr.length() > 0) {
                    (*object->mutable_fields())[keyStr] = napiToProtobufValue(env, obj.Get(key));
                }
            }
        }
    } else {
        // For undefined/null values, use empty string
        pbValue.set_string_value("");
    }
    
    return pbValue;
}

Napi::Value protobufValueToNapi(Napi::Env& env, const skyline::Value& value) {
    switch (value.value_case()) {
        case skyline::Value::kStringValue:
            return Napi::String::New(env, value.string_value());
        case skyline::Value::kIntValue:
            return Napi::Number::New(env, static_cast<double>(value.int_value()));
        case skyline::Value::kDoubleValue:
            return Napi::Number::New(env, value.double_value());
        case skyline::Value::kBoolValue:
            return Napi::Boolean::New(env, value.bool_value());
        case skyline::Value::kBytesValue: {
            const std::string& bytes_str = value.bytes_value();
            return Napi::Buffer<uint8_t>::Copy(env, 
                reinterpret_cast<const uint8_t*>(bytes_str.data()), 
                bytes_str.length());
        }
        case skyline::Value::kArrayValue: {
            const auto& array_value = value.array_value();
            Napi::Array arr = Napi::Array::New(env, array_value.values_size());
            for (int i = 0; i < array_value.values_size(); i++) {
                arr[i] = protobufValueToNapi(env, array_value.values(i));
            }
            return arr;
        }        case skyline::Value::kObjectValue: {
            const auto& object_value = value.object_value();
              // Check if this is a special callback function object
            auto callbackIdIt = object_value.fields().find("callbackId");
            if (callbackIdIt != object_value.fields().end() && 
                callbackIdIt->second.value_case() == skyline::Value::kStringValue) {
                std::string callbackId = callbackIdIt->second.string_value();
                
                // First try to find the callback in local storage (for client side)
                Convert::CallbackData* callbackData = Convert::find_callback(callbackId);
                if (callbackData && callbackData->funcRef) {
                    return callbackData->funcRef->Value();
                }
                
                // If callback not found locally, it means we're on the server side
                // or the callback is cross-process. In this case, just return the 
                // callback object as a regular JavaScript object, not a function.
                // The server-side JavaScript code should handle this appropriately.
                Napi::Object callbackObj = Napi::Object::New(env);
                for (const auto& [key, val] : object_value.fields()) {
                    callbackObj.Set(key, protobufValueToNapi(env, val));
                }
                return callbackObj;
            }
              // Check for instance objects
            auto instanceIdIt = object_value.fields().find("instanceId");
            auto instanceTypeIt = object_value.fields().find("instanceType");
            if (instanceIdIt != object_value.fields().end() && 
                instanceTypeIt != object_value.fields().end() &&
                instanceIdIt->second.value_case() == skyline::Value::kStringValue &&
                instanceTypeIt->second.value_case() == skyline::Value::kStringValue) {
                
#ifdef _SKYLINE_CLIENT_
                return Convert::createInstanceFromProtobuf(env, 
                    instanceIdIt->second.string_value(),
                    instanceTypeIt->second.string_value());
#else
                // For server builds, just create a basic object with instanceId and instanceType
                Napi::Object obj = Napi::Object::New(env);
                obj.Set("instanceId", Napi::String::New(env, instanceIdIt->second.string_value()));
                obj.Set("instanceType", Napi::String::New(env, instanceTypeIt->second.string_value()));
                return obj;
#endif
            }
            
            // Regular object
            Napi::Object obj = Napi::Object::New(env);
            for (const auto& [key, val] : object_value.fields()) {
                obj.Set(key, protobufValueToNapi(env, val));
            }
            return obj;
        }
        default:
            return env.Undefined();
    }
}

std::string serializeMessage(const skyline::Message& message) {
    std::string serialized;
    message.SerializeToString(&serialized);
    return serialized;
}

bool deserializeMessage(const std::string& data, skyline::Message& message) {
    return message.ParseFromString(data);
}

skyline::Message createConstructorMessage(const std::string& id, const std::string& clazz, const std::vector<skyline::Value>& params) {
    skyline::Message message;
    message.set_id(id);
    message.set_type(skyline::MessageType::CONSTRUCTOR);
    
    skyline::ConstructorData* data = message.mutable_constructor_data();
    data->set_clazz(clazz);
    for (const auto& param : params) {
        *data->add_params() = param;
    }
    
    return message;
}

skyline::Message createStaticMessage(const std::string& id, const std::string& clazz, const std::string& action, const std::vector<skyline::Value>& params) {
    skyline::Message message;
    message.set_id(id);
    message.set_type(skyline::MessageType::STATIC);
    
    skyline::StaticData* data = message.mutable_static_data();
    data->set_clazz(clazz);
    data->set_action(action);
    for (const auto& param : params) {
        *data->add_params() = param;
    }
    
    return message;
}

skyline::Message createDynamicMessage(const std::string& id, const std::string& instanceId, const std::string& action, const std::vector<skyline::Value>& params) {
    skyline::Message message;
    message.set_id(id);
    message.set_type(skyline::MessageType::DYNAMIC);
    
    skyline::DynamicData* data = message.mutable_dynamic_data();
    data->set_instance_id(instanceId);
    data->set_action(action);
    for (const auto& param : params) {
        *data->add_params() = param;
    }
    
    return message;
}

skyline::Message createDynamicPropertyMessage(const std::string& id, const std::string& instanceId, const std::string& action, skyline::PropertyAction propertyAction, const std::vector<skyline::Value>& params) {
    skyline::Message message;
    message.set_id(id);
    message.set_type(skyline::MessageType::DYNAMIC_PROPERTY);
    
    skyline::DynamicPropertyData* data = message.mutable_dynamic_property_data();
    data->set_instance_id(instanceId);
    data->set_action(action);
    data->set_property_action(propertyAction);
    for (const auto& param : params) {
        *data->add_params() = param;
    }
    
    return message;
}

skyline::Message createCallbackMessage(const std::string& id, const std::string& callbackId, const std::vector<skyline::Value>& args, bool block) {
    skyline::Message message;
    message.set_id(id);
    message.set_type(skyline::MessageType::EMIT_CALLBACK);
    
    skyline::CallbackData* data = message.mutable_callback_data();
    data->set_callback_id(callbackId);
    data->set_block(block);
    for (const auto& arg : args) {
        *data->add_args() = arg;
    }
    
    return message;
}

skyline::Message createCallbackReplyMessage(const std::string& id, const skyline::Value& result) {
    skyline::Message message;
    message.set_id(id);
    message.set_type(skyline::MessageType::CALLBACK_REPLY);
    
    auto* data = message.mutable_response_data();
    *data->mutable_return_value() = result;
    
    return message;
}

skyline::Message createResponseMessage(const std::string& id, const skyline::Value& returnValue, const std::string& instanceId, const std::string& error) {
    skyline::Message message;
    message.set_id(id);
    message.set_type(skyline::MessageType::RESPONSE);
    
    skyline::ResponseData* data = message.mutable_response_data();
    if (!error.empty()) {
        data->set_error(error);
    } else if (!instanceId.empty()) {
        data->set_instance_id(instanceId);
    } else {
        *data->mutable_return_value() = returnValue;
    }
    
    return message;
}

}
