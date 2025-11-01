#include "../common/convert.hh"
#include "../common/logger.hh"
#include "napi.h"
#include <basetsd.h>
#include <cstdint>
#include <exception>
#include <memory>
#include <thread>
#include <chrono>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "messages.pb.h"
// #define USE_MEMORY
#ifdef USE_MEMORY
#include "server_memory.hh"
#else
#include "server_socket.hh"
#endif

using Logger::logger;

namespace Process {
static std::shared_ptr<SkylineServer::Server> server;
static Napi::ThreadSafeFunction messageHandleTsfn;
static std::shared_ptr<Napi::FunctionReference> messageHandleRef;
static std::map<int64_t, std::shared_ptr<std::promise<skyline::Message>>> socketRequest;
static std::queue<skyline::Message> blockQueue;
static std::mutex blockQueueMutex; // 保护 blockQueue 的互斥锁
static std::condition_variable blockQueueCV; // 条件变量，用于高效等待消息
static int64_t requestId = 1;

void processMessage(const skyline::Message &message) {
    try {
        // Extract the message ID
        auto id = message.id();
        logger->info("Received protobuf message: {}", id);

        // Check if there's a promise associated with this ID
        auto it = socketRequest.find(id);
        if (it != socketRequest.end()) {
            logger->info("Found id: {}", id);
            // server发出的消息的回复
            it->second->set_value(message);
            socketRequest.erase(it);
            blockQueueCV.notify_all(); // 通知等待者有新消息到达
            return;
        }
        logger->info("Push protobuf message to queue... {}", id);
        {
            std::lock_guard<std::mutex> lock(blockQueueMutex);
            blockQueue.push(message);
            blockQueueCV.notify_all(); // 通知等待者有新消息到达
        }

        // Check if Protobuf callback is set
        if (!messageHandleTsfn) {
            logger->error("Protobuf callback not set! Please call setMessageCallback first.");
            return;
        }

        // Call the JavaScript callback through the thread-safe function
        auto callback = [message](Napi::Env env, Napi::Function jsCallback) {
            try {
                logger->debug("Handle callback start. {}", message.id());
                // Remove the corresponding message from the queue
                {
                    // TODO: 此处有性能浪费
                    std::lock_guard<std::mutex> lock(blockQueueMutex);
                    if (blockQueue.empty()) {
                        return;
                    }
                    auto queueMsg = blockQueue.front();
                    if (queueMsg.id() == message.id()) {
                        blockQueue.pop(); // Remove the message from the queue
                    } else {
                        logger->warn("Queue message mismatch: queue has {}, processing {}", 
                                    queueMsg.id(), message.id());
                        // Still process the message, but don't pop from queue
                        return ;
                    }
                    
                }
                
                logger->debug("Handle protobuf message in js start. {}", message.id());
                jsCallback.Call({Convert::convertProtobufToJs(env, message)});
                logger->debug("Handle protobuf message in js end. {}", message.id());
            } catch (const std::exception &e) {
                logger->error("Error in protobuf callback: {}", e.what());
            } catch (...) {
                logger->error("Unknown error occurred in protobuf callback");
            }
        };

        logger->info("blocking call start: {}", id);
        messageHandleTsfn.BlockingCall(callback);
        logger->info("blocking call end: {}", id);
    } catch (const std::exception &e) {
        logger->error("Error processing protobuf message: {}", e.what());
    } catch (...) {
        logger->error("Unknown error occurred while processing protobuf message");
    }
}
int startInner(const Napi::CallbackInfo &info) {
    try {
        #ifdef USE_MEMORY
        server = std::make_shared<SkylineServer::ServerMemory>();
        #else
        server = std::make_shared<SkylineServer::ServerSocket>();
        #endif
        server->Init(info);
        // Start accepting connections (only one client in 1-to-1 scenario)
        std::thread([&]() {
            while (true) {
                try{
                    // Handle client in a separate thread
                    // logger->info("start to getMessage!");
                    auto msg = server->receiveMessage();
                    if (msg.empty()) {
                        continue;
                    }
                    skyline::Message pbMessage;
                    if (!pbMessage.ParseFromString(msg)) {
                        logger->error("Failed to parse Protobuf message from shared memory");
                        continue;
                    }
                    processMessage(pbMessage);
                }catch (const std::exception &e) {
                    logger->error("Error in message processing thread: {}", e.what());
                    break;
                } catch (...) {
                    logger->error("Unknown error occurred in message processing thread");
                    break;
                }
            }
        }).detach();

        return 0;
    } catch (std::exception &e) {
        logger->error("Failed to start server: {}", e.what());
        throw Napi::Error::New(info.Env(), e.what());
    }
}

Napi::Value start(const Napi::CallbackInfo &info) {
    if (info.Length() < 2) {
        throw Napi::TypeError::New(info.Env(),
                                   "start: Wrong number of arguments");
    }
    if (!info[0].IsString()) {
        throw Napi::TypeError::New(info.Env(),
                                   "First argument must be a string");
    }
    if (!info[1].IsNumber()) {
        throw Napi::TypeError::New(info.Env(),
                                   "Second argument must be a number");
    } 
    auto env = info.Env();
    auto host = info[0].As<Napi::String>().Utf8Value();
    auto port = info[1].As<Napi::Number>().Int32Value();
    startInner(info);
    
    return env.Undefined();
}
void stop(const Napi::CallbackInfo &info) {
    logger->info("Socket server stopped");
}

void setMessageCallback(const Napi::CallbackInfo &info) {
    if (info.Length() < 1) {
        throw Napi::TypeError::New(
            info.Env(), "setMessageCallback: Wrong number of arguments");
    }
    if (!info[0].IsFunction()) {
        throw Napi::TypeError::New(info.Env(),
                                   "First argument must be a function");
    }    // Create a ThreadSafeFunction for Protobuf
    messageHandleTsfn = Napi::ThreadSafeFunction::New(
        info.Env(), info[0].As<Napi::Function>(), "Socket Protobuf Callback", 0, 1);
    messageHandleRef = std::make_shared<Napi::FunctionReference>(
        Napi::Persistent(info[0].As<Napi::Function>()));
    logger->info("Set protobuf message callback");
}

Napi::Value sendMessageSingle(const Napi::CallbackInfo &info) {
    if (info.Length() < 1) {
        throw Napi::TypeError::New(
            info.Env(), "sendMessageSinglePb: Wrong number of arguments");
    }

    if (!info[0].IsObject()) {
        throw Napi::TypeError::New(info.Env(),
                                   "First argument must be a protobuf object");
    }
    auto env = info.Env();
    // Convert the JavaScript object to a Protobuf message
    skyline::Message message = Convert::convertJsToProtobuf(env, info[0].As<Napi::Object>());
    logger->info("Send protobuf message to client, id: {}", message.id());

    // Serialize the message and send it
    std::string serializedMessage = message.SerializeAsString();
    server->sendMessage(serializedMessage);
    return info.Env().Undefined();
}
/**
 * 给客户端发送消息
 */
Napi::Value sendMessageSync(const Napi::CallbackInfo &info) {
    if (info.Length() < 1) {
        throw Napi::TypeError::New(
            info.Env(), "sendMessageSync: Wrong number of arguments");
    }

    if (!info[0].IsObject()) {
        throw Napi::TypeError::New(info.Env(),
                                   "First argument must be a protobuf object");
    }

    auto env = info.Env();

    // Convert the JavaScript object to a Protobuf message
    skyline::Message message = Convert::convertJsToProtobuf(env, info[0].As<Napi::Object>());

    if (requestId >= INT64_MAX) {
        requestId = 1;
    }
    auto id = requestId++;
    message.set_id(id);

    // 先存储，再发送
    auto promiseObj = std::make_shared<std::promise<skyline::Message>>();
    std::future<skyline::Message> futureObj = promiseObj->get_future();

    {
        // 使用作用域限制插入操作的临界区范围
        socketRequest.emplace(id, promiseObj);
    }

    logger->info("send protobuf sync message to client, id: {}", id);
    // logger->debug("id: {}", id, message.DebugString());

    // 发送消息给单一客户端
    // Serialize the message and send it
    std::string serializedMessage = message.SerializeAsString();
    server->sendMessage(serializedMessage);

    // 使用更高效的等待方式
    auto start = std::chrono::steady_clock::now();
    std::chrono::duration<double> timeoutDuration(3.0); // 3秒超时

    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - start;
        if (elapsed > timeoutDuration) {
            socketRequest.erase(id);
            throw Napi::Error::New(
                info.Env(),
                "Request to client timeout, request data:\n" + message.DebugString());
        }
        auto status = futureObj.wait_for(std::chrono::milliseconds(0));
        if (status == std::future_status::ready) {
            break;
        }

        // 检查队列中是否有待处理的消息
        skyline::Message queuedMsg;
        {
            std::unique_lock<std::mutex> lock(blockQueueMutex);
            // 等待消息到达或超时（最多等10ms，避免单次等待过长）
            blockQueueCV.wait_for(lock, std::chrono::milliseconds(1), 
                                  [&]() { return !blockQueue.empty(); });
            
            if (!blockQueue.empty()) {
                queuedMsg = blockQueue.front();
                blockQueue.pop();
            } else {
                continue;
            }
        }
        
        if (queuedMsg.IsInitialized()) {
            try {
                // debug消息
                // Client发来的消息
                logger->debug("Start to handle blocked protobuf message: {}", queuedMsg.id());
                // Call the JavaScript callback through the thread-safe function
                messageHandleRef->Value().Call({Convert::convertProtobufToJs(env, queuedMsg)});
            } catch (const std::exception &e) {
                logger->error("Error parsing protobuf message: {}", e.what());
                throw Napi::Error::New(info.Env(), e.what());
            } catch (...) {
                logger->error("Unknown error occurred");
                throw Napi::Error::New(info.Env(), "Unknown error occurred");
            }
        }
    }

    logger->info("futureObj wait end, id: {}", id);
    skyline::Message result = futureObj.get();
    logger->debug("Result get id: {}", id);

    // Convert the Protobuf message to a JavaScript value
    auto resultObj = Convert::convertProtobufToJs(env, result);
    
    return resultObj.Get("data").As<Napi::Object>()
    .Get("result").As<Napi::Object>()
    .Get("returnValue");
}

} // namespace SocketServer
