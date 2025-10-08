#include "client_action.hh"
#include "../common/convert.hh"
#include "../common/logger.hh"
#include "../common/snowflake.hh"
#include "../common/protobuf_converter.hh"
#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include "client_memory.hh"
#include "client.hh"

using Logger::logger;

namespace ClientAction {
static std::shared_ptr<Client> client;
static std::mutex socketRequestMutex; // Add mutex for thread synchronization
static std::unordered_map<std::string, std::shared_ptr<std::promise<skyline::Message>>>
    requestMapping;
static std::queue<std::shared_ptr<skyline::Message>> callbackQueue;

// Forward declarations
void processMessage(const skyline::Message &message);

static std::atomic<bool> blocked{false};
using snowflake_t = snowflake<1534832906275L>;
static snowflake_t serverCommunicationUuid;

void processMessage(const skyline::Message &message) {
    logger->info("received protobuf message, id: {}", message.id());

    try {
        // Check if it's a response to a request
        if (!message.id().empty()) {
            std::lock_guard<std::mutex> lock(socketRequestMutex);
            auto it = requestMapping.find(message.id());
            if (it != requestMapping.end()) {
                it->second->set_value(message);
                requestMapping.erase(it);
                return;
            }
        }

        // Handle callback messages
        if (blocked) {
            logger->info("blocked, push protobuf message to queue...");
            callbackQueue.push(std::make_shared<skyline::Message>(message));
            return;
        }

        // Handle different message types
        switch (message.type()) {
            case skyline::EMIT_CALLBACK: {
                // Handle Protobuf callback
                const auto& callbackData = message.callback_data();
                std::string callbackId = callbackData.callback_id();
                auto ptr = Convert::find_callback(callbackId);
                
                if (ptr != nullptr) {
                    logger->info("callbackId found: {}", callbackId);
                    bool isBlocking = callbackData.block();
                    
                    auto callbackFunction = [message, callbackData](Napi::Env env, Napi::Function jsCallback) {
                        Napi::HandleScope scope(env);
                        std::vector<Napi::Value> argsVec;                        // Convert Protobuf values to Napi values
                        for (const auto &arg : callbackData.args()) {
                            argsVec.push_back(ProtobufConverter::protobufValueToNapi(env, arg));
                        }
                        
                        logger->info("call callback function...");
                        auto resultValue = jsCallback.Call(argsVec);
                        logger->info("call callback function end...");                        // If callback expects a reply, send it back
                        if (!message.id().empty()) {
                            logger->info("reply callback...");
                            skyline::Value resultProtobuf = ProtobufConverter::napiToProtobufValue(env, resultValue);
                            
                            skyline::Message callbackResult;
                            callbackResult.set_id(message.id());
                            callbackResult.set_type(skyline::CALLBACK_REPLY);
                            
                            auto* responseData = callbackResult.mutable_response_data();
                            *responseData->mutable_return_value() = resultProtobuf;
                            
                            // Send reply using Protobuf
                            {
                                std::string serializedMessage = ProtobufConverter::serializeMessage(callbackResult);
                                // uint32_t messageLength = static_cast<uint32_t>(serializedMessage.size());
                                // std::string lengthPrefix(reinterpret_cast<const char*>(&messageLength), sizeof(messageLength));
                                // std::string fullMessage = lengthPrefix + serializedMessage;
                                client->sendMessage(serializedMessage);
                            }
                        }
                    };
                    
                    if (isBlocking) {
                        ptr->tsfn.BlockingCall(callbackFunction);
                    } else {
                        ptr->tsfn.NonBlockingCall(callbackFunction);
                    }
                } else {
                    logger->error("callbackId not found: {}", callbackId);
                }
                break;
            }
            default:
                logger->warn("Unhandled protobuf message type: {}, content: {}", static_cast<int>(message.type()), message.DebugString());
                break;
        }
    } catch (const std::exception &e) {
        logger->error("Error processing protobuf message: {}", e.what());
    }
}

void initSocket(Napi::Env &env) {
    try {
        serverCommunicationUuid.init(1, 1);
        
        // TODO: 初始化不同的客户端以应对不同的传输方式
        client = std::make_shared<ClientMemory>();
        client->Init(env);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait for shared memory to be ready
        // Start synchronous message reading thread
        std::thread([]() {
            try {
                while (true) {
                    std::string msg = client->receiveMessage(
                        #ifdef _WIN32
                        "Global\\skyline_server2client_notify"
                        #else
                        "skyline_server2client_notify"
                        #endif
                    );
                    if (msg.empty()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        continue;
                    }
                    skyline::Message pbMessage;
                    if (!pbMessage.ParseFromString(msg)) {
                        logger->error("Failed to parse Protobuf message from shared memory");
                        continue;
                    }
                    processMessage(pbMessage);
                }
            } catch (std::exception &e) {
                logger->error("Message reading thread error: {}", e.what());
            }
        }).detach();
    } catch (std::exception &e) {
        logger->error("Connection error: {}", e.what());
        throw Napi::Error::New(env, std::string("Socket connection failed: ") +
                                        e.what());
    }
    catch(...) {
        logger->error("Unknown error occurred during socket initialization");
        throw Napi::Error::New(env, "Unknown error occurred during socket initialization");
    }
}

skyline::Message sendMessageSync(const skyline::Message &message) {

    blocked = true;
    
    // 序列化Protobuf消息
    std::string serializedMessage = ProtobufConverter::serializeMessage(message);
    logger->info("Protobuf message serialized, size: {}, id: {}", serializedMessage.size(), message.id());
    
    // 添加长度前缀（4字节）和消息体
    // uint32_t messageLength = static_cast<uint32_t>(serializedMessage.size());
    // std::string lengthPrefix(reinterpret_cast<const char*>(&messageLength), sizeof(messageLength));
    // std::string fullMessage = lengthPrefix + serializedMessage;
    // logger->info("Full message with length prefix, total size: {}", fullMessage.size());
    logger->debug("id: {}, content: {}", message.id(), message.DebugString());

    auto promiseObj = std::make_shared<std::promise<skyline::Message>>();
    std::future<skyline::Message> futureObj = promiseObj->get_future();

    // Lock the mutex while manipulating the map
    {
        std::lock_guard<std::mutex> lock(socketRequestMutex);
        auto insertResult = requestMapping.emplace(message.id(), promiseObj);
        if (!insertResult.second) {
            blocked = false;
            throw std::runtime_error("id insert to request map failed: " + message.id());
        }
    }

    // Send message with socket protection
    client->sendMessage(serializedMessage);
    logger->debug("send end.");
    
    // Wait for response with timeout
    auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    
    while (std::chrono::steady_clock::now() < timeout) {
        // 使用短暂等待替代立即轮询，减少 CPU 消耗
        auto waitStatus = futureObj.wait_for(std::chrono::milliseconds(0));
        
        if (waitStatus == std::future_status::ready) {
            break;
        }
        // Handle callback queue during wait
        if (!callbackQueue.empty()) {
            auto pbMessage = callbackQueue.front();
            callbackQueue.pop();
            logger->info("start to handle callback.");
            
            if (pbMessage->type() == skyline::EMIT_CALLBACK) {
                const auto& callbackData = pbMessage->callback_data();
                std::string callbackId = callbackData.callback_id();
                auto ptr = Convert::find_callback(callbackId);
                
                if (ptr != nullptr) {
                    logger->info("callbackId found: {}", callbackId);
                    auto env = ptr->funcRef->Env();
                    Napi::HandleScope scope(env);
                    std::vector<Napi::Value> argsVec;                    for (const auto &arg : callbackData.args()) {
                        argsVec.push_back(ProtobufConverter::protobufValueToNapi(env, arg));
                    }
                    std::shared_ptr<Napi::FunctionReference> funcRef = ptr->funcRef;
                    auto resultValue = funcRef->Value().Call(argsVec);

                    // If callback expects a reply, send it back
                    if (!pbMessage->id().empty()) {
                        logger->info("reply callback...");
                        skyline::Value resultProtobuf = ProtobufConverter::napiToProtobufValue(env, resultValue);
                        
                        skyline::Message callbackResult;
                        callbackResult.set_id(pbMessage->id());
                        callbackResult.set_type(skyline::CALLBACK_REPLY);
                        
                        auto* responseData = callbackResult.mutable_response_data();
                        *responseData->mutable_return_value() = resultProtobuf;
                        
                        // Send reply using Protobuf
                        {
                            std::string serializedMessage = ProtobufConverter::serializeMessage(callbackResult);
                            client->sendMessage(serializedMessage);
                        }
                    }
                } else {
                    logger->error("callbackId not found: {}", callbackId);
                }            }
        }
    }

    // 检查是否超时
    if (std::chrono::steady_clock::now() >= timeout) {
        blocked = false;
        requestMapping.erase(message.id());
        throw std::runtime_error("Operation timed out after 5 seconds");
    }

    skyline::Message result = futureObj.get();
    blocked = false;
    
    logger->debug("Recevied message: {}", result.DebugString());
    // Check for errors in response
    if (result.type() == skyline::MessageType::RESPONSE) {
        const auto& responseData = result.response_data();
        if (!responseData.error().empty()) {
            throw std::runtime_error("Server response error: " + responseData.error());
        }
    }
    
    return result;
}

void sendMessageAsync(const skyline::Message &message) {

    // 序列化Protobuf消息
    std::string serializedMessage = ProtobufConverter::serializeMessage(message);
    
    client->sendMessage(serializedMessage);
}

void callDynamicAsync(const std::string &instanceId, const std::string &action,
                      const std::vector<skyline::Value> &params) {
    std::string id = std::to_string(serverCommunicationUuid.nextid());
    skyline::Message message = ProtobufConverter::createDynamicMessage(id, instanceId, action, params);
    sendMessageAsync(message);
}

skyline::Value callConstructorSync(const std::string &clazz,
                                   const std::vector<skyline::Value> &params) {
    std::string id = std::to_string(serverCommunicationUuid.nextid());
    skyline::Message message = ProtobufConverter::createConstructorMessage(id, clazz, params);
    skyline::Message response = sendMessageSync(message);
    
    if (response.type() == skyline::MessageType::RESPONSE) {
        const auto& responseData = response.response_data();
        if (!responseData.error().empty()) {
            throw std::runtime_error("Server response error: " + responseData.error());
        }
        if (!responseData.instance_id().empty()) {
            skyline::Value result;
            result.set_string_value(responseData.instance_id());
            return result;
        }
        return responseData.return_value();
    }
    
    throw std::runtime_error("Invalid response type");
}

skyline::Value callDynamicSync(const std::string &instanceId,
                               const std::string &action,
                               const std::vector<skyline::Value> &params) {
    std::string id = std::to_string(serverCommunicationUuid.nextid());
    skyline::Message message = ProtobufConverter::createDynamicMessage(id, instanceId, action, params);
    skyline::Message response = sendMessageSync(message);
    
    if (response.type() == skyline::MessageType::RESPONSE) {
        const auto& responseData = response.response_data();
        if (!responseData.error().empty()) {
            throw std::runtime_error("Server response error: " + responseData.error());
        }
        return responseData.return_value();
    }
    
    throw std::runtime_error("Invalid response type");
}

skyline::Value callDynamicPropertySetSync(const std::string &instanceId,
                                          const std::string &action,
                                          const std::vector<skyline::Value> &params) {
    std::string id = std::to_string(serverCommunicationUuid.nextid());
    skyline::Message message = ProtobufConverter::createDynamicPropertyMessage(id, instanceId, action, skyline::PropertyAction::SET, params);
    skyline::Message response = sendMessageSync(message);
    
    if (response.type() == skyline::MessageType::RESPONSE) {
        const auto& responseData = response.response_data();
        if (!responseData.error().empty()) {
            throw std::runtime_error("Server response error: " + responseData.error());
        }
        return responseData.return_value();
    }
    
    throw std::runtime_error("Invalid response type");
}

skyline::Value callDynamicPropertyGetSync(const std::string &instanceId,
                                          const std::string &action) {
    std::string id = std::to_string(serverCommunicationUuid.nextid());
    std::vector<skyline::Value> emptyParams;
    skyline::Message message = ProtobufConverter::createDynamicPropertyMessage(id, instanceId, action, skyline::PropertyAction::GET, emptyParams);
    skyline::Message response = sendMessageSync(message);
    
    if (response.type() == skyline::MessageType::RESPONSE) {
        const auto& responseData = response.response_data();
        if (!responseData.error().empty()) {
            throw std::runtime_error("Server response error: " + responseData.error());
        }
        return responseData.return_value();
    }
    
    throw std::runtime_error("Invalid response type");
}

skyline::Value callStaticSync(const std::string &clazz,
                              const std::string &action, const std::vector<skyline::Value> &params) {
    std::string id = std::to_string(serverCommunicationUuid.nextid());
    skyline::Message message = ProtobufConverter::createStaticMessage(id, clazz, action, params);
    skyline::Message response = sendMessageSync(message);
    // logger->debug("callStaticSync response: {}", response.DebugString());
    if (response.type() == skyline::MessageType::RESPONSE) {
        const auto& responseData = response.response_data();
        if (!responseData.error().empty()) {
            throw std::runtime_error("Server response error: " + responseData.error());
        }
        return responseData.return_value();
    }
    
    throw std::runtime_error("Invalid response type");
}

skyline::Value callCustomHandleSync(const std::string &action,
                                    const std::vector<skyline::Value> &params) {
    return callStaticSync("customHandle", action, params);
}

} // namespace SocketClient
