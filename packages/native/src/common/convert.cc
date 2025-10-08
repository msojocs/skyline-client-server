#include "convert.hh"
#include "../client/skyline_global/async_style_sheets.hh"
#include "../client/skyline_global/fragment_binding.hh"
#include "../client/skyline_global/mutable_value.hh"
#include "../client/skyline_global/shadow_node/grid_view.hh"
#include "../client/skyline_global/shadow_node/hero.hh"
#include "../client/skyline_global/shadow_node/image.hh"
#include "../client/skyline_global/shadow_node/input.hh"
#include "../client/skyline_global/shadow_node/list_view.hh"
#include "../client/skyline_global/shadow_node/scroll_view.hh"
#include "../client/skyline_global/shadow_node/sticky_header.hh"
#include "../client/skyline_global/shadow_node/sticky_section.hh"
#include "../client/skyline_global/shadow_node/swiper.hh"
#include "../client/skyline_global/shadow_node/swiper_item.hh"
#include "../client/skyline_global/shadow_node/text.hh"
#include "../client/skyline_global/shadow_node/view.hh"
#include "../client/client_action.hh"
#include "../common/snowflake.hh"
#include "napi.h"
#include "protobuf_converter.hh"
#include <memory>
#include <string>


namespace Convert {

static std::map<std::string, CallbackData> callback;
static std::map<std::string, std::shared_ptr<Napi::ObjectReference>>
    instanceCache;
using snowflake_t = snowflake<1534832906275L>;
static snowflake_t callbackUuid;
static snowflake_t sharedMemoryUuid;

static std::map<std::string, Napi::FunctionReference *> clazzMap;

// Core callback functions available for both client and server
CallbackData *find_callback(const std::string &callbackId) {
    auto it = callback.find(callbackId);
    if (it != callback.end()) {
        return &it->second;
    }
    return nullptr;
}

snowflake_t& getCallbackUuid() {
    // Ensure the UUID generator is initialized
    static bool initialized = false;
    if (!initialized) {
        callbackUuid.init(2, 1);
        initialized = true;
    }
    return callbackUuid;
}

void storeCallback(const std::string& callbackId, const CallbackData& data) {
    callback[callbackId] = data;
}

#ifdef _SKYLINE_CLIENT_
Napi::Value createInstanceFromProtobuf(Napi::Env& env, const std::string& instanceId, const std::string& instanceType) {
    if (instanceType == "function") {
        // 返回值是个函数，如makeShareable
        return Napi::Function::New(
            env, [instanceId](const Napi::CallbackInfo &info) {
                auto env = info.Env();
                std::vector<skyline::Value> args;
                for (int i = 0; i < info.Length(); i++) {
                    args.push_back(ProtobufConverter::napiToProtobufValue(
                        env, info[i]));
                }
                auto result = ClientAction::callStaticSync(
                    "functionData", instanceId, args);

                return ProtobufConverter::protobufValueToNapi(env, result);
            });
    }

    auto it = clazzMap.find(instanceType);
    if (it != clazzMap.end()) {
        try {
            Napi::FunctionReference *func = it->second;
            // 创建实例
            // 先到cache找
            if (instanceCache.find(instanceId) != instanceCache.end()) {
                return instanceCache[instanceId]->Value();
            }
            // cache找不到
            auto result = func->New({Napi::String::New(env, instanceId)});
            auto ref = Napi::Persistent(result);
            instanceCache.emplace(
                instanceId, std::make_shared<Napi::ObjectReference>(
                                std::move(ref)));
            return result;
        } catch (const std::exception &e) {
            Napi::Error::New(env,
                             std::string("Error creating instance: ") +
                                 e.what())
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }
    }
    return env.Undefined();
}

void RegisteInstanceType(Napi::Env &env) {
    // callbackUuid is now initialized in getCallbackUuid() on first access
    sharedMemoryUuid.init(4, 1);
    // 注册实例类型和对应的构造函数
    clazzMap["AsyncStylesheets"] = Skyline::AsyncStyleSheets::GetClazz(env);
    clazzMap["TextShadowNode"] = Skyline::TextShadowNode::GetClazz(env);
    clazzMap["InputShadowNode"] = Skyline::InputShadowNode::GetClazz(env);
    clazzMap["ImageShadowNode"] = Skyline::ImageShadowNode::GetClazz(env);
    clazzMap["SwiperShadowNode"] = Skyline::SwiperShadowNode::GetClazz(env);
    // SwiperItemShadoNode就是少了一个w
    clazzMap["SwiperItemShadoNode"] =
        Skyline::SwiperItemShadowNode::GetClazz(env);
    clazzMap["ViewShadowNode"] = Skyline::ViewShadowNode::GetClazz(env);
    clazzMap["GridViewShadowNode"] = Skyline::GridViewShadowNode::GetClazz(env);
    clazzMap["ScrollViewShadowNode"] =
        Skyline::ScrollViewShadowNode::GetClazz(env);
    clazzMap["ListViewShadowNode"] = Skyline::ListViewShadowNode::GetClazz(env);
    clazzMap["StickySectionShadowNode"] =
        Skyline::StickySectionShadowNode::GetClazz(env);
    clazzMap["StickyHeaderShadowNode"] =
        Skyline::StickyHeaderShadowNode::GetClazz(env);
    clazzMap["FragmentBinding"] = Skyline::FragmentBinding::GetClazz(env);
    clazzMap["MutableValue"] = Skyline::MutableValue::GetClazz(env);
    clazzMap["HeroShadowNode"] = Skyline::HeroShadowNode::GetClazz(env);
}
#endif

// Protobuf conversion functions
skyline::MessageType stringToMessageType(const std::string &typeStr) {
    if (typeStr == "constructor")
        return skyline::CONSTRUCTOR;
    if (typeStr == "static")
        return skyline::STATIC;
    if (typeStr == "dynamic")
        return skyline::DYNAMIC;
    if (typeStr == "dynamicProperty")
        return skyline::DYNAMIC_PROPERTY;
    if (typeStr == "emitCallback")
        return skyline::EMIT_CALLBACK;
    if (typeStr == "callbackReply")
        return skyline::CALLBACK_REPLY;
    if (typeStr == "response")
        return skyline::RESPONSE;
    return skyline::CONSTRUCTOR; // default fallback
}

std::string messageTypeToString(skyline::MessageType type) {
    switch (type) {
    case skyline::CONSTRUCTOR:
        return "constructor";
    case skyline::STATIC:
        return "static";
    case skyline::DYNAMIC:
        return "dynamic";
    case skyline::DYNAMIC_PROPERTY:
        return "dynamicProperty";
    case skyline::EMIT_CALLBACK:
        return "emitCallback";
    case skyline::CALLBACK_REPLY:
        return "callbackReply";
    case skyline::RESPONSE:
        return "response";
    default:
        return "constructor";
    }
}

Napi::Object convertProtobufToJs(Napi::Env &env,
                                 const skyline::Message &message) {
    Napi::Object obj = Napi::Object::New(env);

    obj.Set("id", Napi::String::New(env, message.id()));
    obj.Set("type",
            Napi::String::New(env, messageTypeToString(message.type())));
    // Convert the specific data based on message type
    switch (message.type()) {
    case skyline::CONSTRUCTOR:
        if (message.has_constructor_data()) {
            Napi::Object constructorObj = Napi::Object::New(env);
            constructorObj.Set(
                "clazz",
                Napi::String::New(env, message.constructor_data().clazz()));
            // Convert parameters array
            Napi::Array params = Napi::Array::New(env);
            for (int i = 0; i < message.constructor_data().params_size(); i++) {
                // Convert protobuf Value to Napi::Value using ProtobufConverter
                params[i] = ProtobufConverter::protobufValueToNapi(
                    env, message.constructor_data().params(i));
            }
            constructorObj.Set("params", params);
            obj.Set("data", constructorObj);
        }
        break;
    case skyline::STATIC:
        if (message.has_static_data()) {
            Napi::Object staticObj = Napi::Object::New(env);
            staticObj.Set(
                "clazz", Napi::String::New(env, message.static_data().clazz()));
            staticObj.Set("action", Napi::String::New(
                                        env, message.static_data().action()));
            Napi::Array params = Napi::Array::New(env);
            for (int i = 0; i < message.static_data().params_size(); i++) {
                params[i] = ProtobufConverter::protobufValueToNapi(
                    env, message.static_data().params(i));
            }
            staticObj.Set("params", params);
            obj.Set("data", staticObj);
        }
        break;
    case skyline::DYNAMIC:
        if (message.has_dynamic_data()) {
            Napi::Object dynamicObj = Napi::Object::New(env);
            dynamicObj.Set(
                "instanceId",
                Napi::String::New(env, message.dynamic_data().instance_id()));
            dynamicObj.Set("action", Napi::String::New(
                                         env, message.dynamic_data().action()));
            Napi::Array params = Napi::Array::New(env);
            for (int i = 0; i < message.dynamic_data().params_size(); i++) {
                params[i] = ProtobufConverter::protobufValueToNapi(
                    env, message.dynamic_data().params(i));
            }
            dynamicObj.Set("params", params);
            obj.Set("data", dynamicObj);        }
        break;
    case skyline::DYNAMIC_PROPERTY:
        if (message.has_dynamic_property_data()) {
            Napi::Object dynamicPropertyObj = Napi::Object::New(env);
            dynamicPropertyObj.Set(
                "instanceId",
                Napi::String::New(env, message.dynamic_property_data().instance_id()));
            dynamicPropertyObj.Set("action", Napi::String::New(
                                         env, message.dynamic_property_data().action()));
            
            // Convert property action enum to string
            std::string propertyActionStr = 
                (message.dynamic_property_data().property_action() == skyline::GET) ? "get" : "set";
            dynamicPropertyObj.Set("propertyAction", Napi::String::New(env, propertyActionStr));
            
            Napi::Array params = Napi::Array::New(env);
            for (int i = 0; i < message.dynamic_property_data().params_size(); i++) {
                params[i] = ProtobufConverter::protobufValueToNapi(
                    env, message.dynamic_property_data().params(i));
            }
            dynamicPropertyObj.Set("params", params);
            obj.Set("data", dynamicPropertyObj);        }
        break;
    case skyline::EMIT_CALLBACK:
        if (message.has_callback_data()) {
            Napi::Object callbackObj = Napi::Object::New(env);
            callbackObj.Set(
                "callbackId",
                Napi::String::New(env, message.callback_data().callback_id()));
            callbackObj.Set("block", 
                           Napi::Boolean::New(env, message.callback_data().block()));
            
            Napi::Array args = Napi::Array::New(env);
            for (int i = 0; i < message.callback_data().args_size(); i++) {
                args[i] = ProtobufConverter::protobufValueToNapi(
                    env, message.callback_data().args(i));
            }
            callbackObj.Set("args", args);
            obj.Set("data", callbackObj);
        }
        break;
    case skyline::CALLBACK_REPLY:
    case skyline::RESPONSE:
        if (message.has_response_data()) {
            const auto &responseData = message.response_data();
            Napi::Object responseObj = Napi::Object::New(env);            // Handle error field
            if (!responseData.error().empty()) {
                responseObj.Set("error",
                                Napi::String::New(env, responseData.error()));
            }

            // Handle result fields
            bool hasReturnValue = responseData.has_return_value();
            bool hasInstanceId = !responseData.instance_id().empty();

            if (hasReturnValue || hasInstanceId) {
                Napi::Object resultObj = Napi::Object::New(env);

                if (hasReturnValue) {
                    resultObj.Set("returnValue",
                                  ProtobufConverter::protobufValueToNapi(
                                      env, responseData.return_value()));
                }

                if (hasInstanceId) {
                    resultObj.Set(
                        "instanceId",
                        Napi::String::New(env, responseData.instance_id()));
                }

                responseObj.Set("result", resultObj);
            }

            obj.Set("data", responseObj);
        }
        break;
    default:
        // Handle other message types as needed
        break;
    }

    return obj;
}

skyline::Message convertJsToProtobuf(Napi::Env &env, const Napi::Object &obj) {
    skyline::Message message;

    // Set basic fields
    if (obj.Has("id")) {
        message.set_id(obj.Get("id").As<Napi::String>().Utf8Value());
    }

    if (obj.Has("type")) {
        std::string typeStr = obj.Get("type").As<Napi::String>().Utf8Value();
        message.set_type(stringToMessageType(typeStr));
    }
    // Handle data field based on message type
    if (obj.Has("data") && obj.Get("data").IsObject()) {
        Napi::Object data = obj.Get("data").As<Napi::Object>();

        switch (message.type()) {
        case skyline::CONSTRUCTOR: {
            skyline::ConstructorData *constructorData =
                message.mutable_constructor_data();
            if (data.Has("clazz")) {
                constructorData->set_clazz(
                    data.Get("clazz").As<Napi::String>().Utf8Value());
            }
            if (data.Has("params") && data.Get("params").IsArray()) {
                Napi::Array params = data.Get("params").As<Napi::Array>();
                for (uint32_t i = 0; i < params.Length(); i++) {
                    skyline::Value *param = constructorData->add_params();
                    *param =
                        ProtobufConverter::napiToProtobufValue(env, params[i]);
                }
            }
            break;
        }
        case skyline::STATIC: {
            skyline::StaticData *staticData = message.mutable_static_data();
            if (data.Has("clazz")) {
                staticData->set_clazz(
                    data.Get("clazz").As<Napi::String>().Utf8Value());
            }
            if (data.Has("action")) {
                staticData->set_action(
                    data.Get("action").As<Napi::String>().Utf8Value());
            }
            if (data.Has("params") && data.Get("params").IsArray()) {
                Napi::Array params = data.Get("params").As<Napi::Array>();
                for (uint32_t i = 0; i < params.Length(); i++) {
                    skyline::Value *param = staticData->add_params();
                    *param =
                        ProtobufConverter::napiToProtobufValue(env, params[i]);
                }
            }
            break;
        }
        case skyline::DYNAMIC: {
            skyline::DynamicData *dynamicData = message.mutable_dynamic_data();
            if (data.Has("instanceId")) {
                dynamicData->set_instance_id(
                    data.Get("instanceId").As<Napi::String>().Utf8Value());
            }
            if (data.Has("action")) {
                dynamicData->set_action(
                    data.Get("action").As<Napi::String>().Utf8Value());
            }
            if (data.Has("params") && data.Get("params").IsArray()) {
                Napi::Array params = data.Get("params").As<Napi::Array>();
                for (uint32_t i = 0; i < params.Length(); i++) {
                    skyline::Value *param = dynamicData->add_params();
                    *param =
                        ProtobufConverter::napiToProtobufValue(env, params[i]);
                }
            }
            break;
        }
        case skyline::DYNAMIC_PROPERTY: {
            skyline::DynamicPropertyData *dynamicPropertyData =
                message.mutable_dynamic_property_data();
            if (data.Has("instanceId")) {
                dynamicPropertyData->set_instance_id(
                    data.Get("instanceId").As<Napi::String>().Utf8Value());
            }
            if (data.Has("action")) {
                dynamicPropertyData->set_action(
                    data.Get("action").As<Napi::String>().Utf8Value());
            }
            if (data.Has("propertyAction")) {
                std::string propertyActionStr =
                    data.Get("propertyAction").As<Napi::String>().Utf8Value();
                if (propertyActionStr == "get") {
                    dynamicPropertyData->set_property_action(skyline::GET);
                } else if (propertyActionStr == "set") {
                    dynamicPropertyData->set_property_action(skyline::SET);
                }
            }
            if (data.Has("params") && data.Get("params").IsArray()) {
                Napi::Array params = data.Get("params").As<Napi::Array>();
                for (uint32_t i = 0; i < params.Length(); i++) {
                    skyline::Value *param = dynamicPropertyData->add_params();
                    *param =
                        ProtobufConverter::napiToProtobufValue(env, params[i]);
                }
            }
            break;
        }
        case skyline::EMIT_CALLBACK: {
            skyline::CallbackData *callbackData =
                message.mutable_callback_data();
            if (data.Has("callbackId")) {
                callbackData->set_callback_id(
                    data.Get("callbackId").As<Napi::String>().Utf8Value());
            }
            if (data.Has("block")) {
                callbackData->set_block(
                    data.Get("block").As<Napi::Boolean>().Value());
            }
            if (data.Has("args") && data.Get("args").IsArray()) {
                Napi::Array args = data.Get("args").As<Napi::Array>();
                for (uint32_t i = 0; i < args.Length(); i++) {
                    skyline::Value *arg = callbackData->add_args();
                    *arg = ProtobufConverter::napiToProtobufValue(env, args[i]);
                }
            }
            break;
        }
        case skyline::CALLBACK_REPLY:
        case skyline::RESPONSE: {
            skyline::ResponseData *responseData =
                message.mutable_response_data();

            // Handle error field first
            if (data.Has("error")) {
                responseData->set_error(
                    data.Get("error").As<Napi::String>().Utf8Value());
            }

            // Handle result field - can now have both returnValue and
            // instanceId
            if (data.Has("result") && data.Get("result").IsObject()) {
                Napi::Object result = data.Get("result").As<Napi::Object>();                // Set returnValue if present
                if (result.Has("returnValue")) {
                    skyline::Value *returnValue =
                        responseData->mutable_return_value();
                    *returnValue = ProtobufConverter::napiToProtobufValue(
                        env, result.Get("returnValue"));
                }

                // Set instanceId if present (can coexist with returnValue now)
                if (result.Has("instanceId")) {
                    responseData->set_instance_id(result.Get("instanceId")
                                                      .As<Napi::String>()
                                                      .Utf8Value());
                }
            }            // Handle direct fields for backward compatibility
            if (data.Has("returnValue")) {
                skyline::Value *returnValue =
                    responseData->mutable_return_value();
                *returnValue = ProtobufConverter::napiToProtobufValue(
                    env, data.Get("returnValue"));
            }

            if (data.Has("instanceId")) {
                responseData->set_instance_id(
                    data.Get("instanceId").As<Napi::String>().Utf8Value());
            }

            break;
        }
        default:
            // Handle other message types as needed
            break;
        }
    }

    return message;
}

} // namespace Convert
