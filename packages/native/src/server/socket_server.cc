#include "socket_server.hh"
#include "../common/convert.hh"
#include "../common/logger.hh"
#include "../common/snowflake.hh"
#include "napi.h"
#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>


using boost::asio::ip::tcp;
using Logger::logger;

namespace SocketServer {
static boost::asio::io_context io_context;
static std::shared_ptr<tcp::acceptor> acceptor;
static std::shared_ptr<tcp::socket>
    client; // Single client for 1-to-1 connection
static std::atomic<bool> client_connected{false};
static Napi::ThreadSafeFunction messageHandleTsfn;
static std::shared_ptr<Napi::FunctionReference> messageHandleRef;
static std::map<std::string, std::shared_ptr<std::promise<skyline::Message>>>
    socketRequest;
static std::queue<skyline::Message> blockQueue;
static std::mutex blockQueueMutex; // 保护 blockQueue 的互斥锁
using snowflake_t = snowflake<1534832906275L>;
static snowflake_t communicationUuid;

// Forward declarations
void startHeartbeat();
std::string readLengthPrefixedMessage(std::shared_ptr<tcp::socket> socket_ptr);

void processMessage(std::shared_ptr<tcp::socket> client, const skyline::Message &message) {
    try {
        logger->info("received protobuf message");

        // Extract the message ID
        std::string id = message.id();

        // Check if there's a promise associated with this ID
        auto it = socketRequest.find(id);
        if (it != socketRequest.end()) {
            logger->info("found id: {}", id);
            // server发出的消息的回复
            it->second->set_value(message);
            socketRequest.erase(it);
            return;
        }
        logger->info("push protobuf message to queue... {}", id);
        {
            std::lock_guard<std::mutex> lock(blockQueueMutex);
            blockQueue.push(message);
        }

        // Check if Protobuf callback is set
        if (!messageHandleTsfn) {
            logger->error("Protobuf callback not set! Please call setMessageCallback first.");
            return;
        }

        // Call the JavaScript callback through the thread-safe function
        auto callback = [message](Napi::Env env, Napi::Function jsCallback) {
            try {
                // Remove the corresponding message from the queue
                {
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
void handleClient(std::shared_ptr<tcp::socket> socket_ptr) {
    try {
        client = socket_ptr;
        client_connected = true;

        while (true) {
            // Read a length-prefixed message
            std::string message = readLengthPrefixedMessage(socket_ptr);

            if (!message.empty()) {
                skyline::Message msg;
                if (msg.ParseFromString(message)) {
                    processMessage(socket_ptr, msg);
                } else {
                    logger->error("Failed to parse protobuf message");
                }
            }
        }
    } catch (boost::system::system_error &e) {
        logger->error("Client disconnected: {}", e.what());
        client.reset();
        client_connected = false;
    }
}
int startInner(const Napi::Env &env, std::string &host, int port) {
    try {
        acceptor = std::make_shared<tcp::acceptor>(
            io_context, tcp::endpoint(tcp::v4(), port));

        // Start accepting connections (only one client in 1-to-1 scenario)
        std::thread([&]() {
            while (true) {
                auto new_client = std::make_shared<tcp::socket>(io_context);
                acceptor->accept(*new_client);

                // Check if we already have a client connected
                if (client_connected) {
                    logger->warn("New client attempting to connect, but "
                                    "one is already connected. Rejecting.");
                    new_client->close();
                    continue;
                }                // Handle client in a separate thread
                std::thread([new_client]() {
                    handleClient(new_client);
                }).detach();
            }
        }).detach();

        // Start the io_context in a separate thread
        std::thread([&]() { io_context.run(); }).detach();

        logger->info("Socket server listening on {}:{}", host, port);
        return 0;
    } catch (std::exception &e) {
        logger->error("Failed to start server: {}", e.what());
        throw Napi::Error::New(env, e.what());
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
    }    auto env = info.Env();
    auto host = info[0].As<Napi::String>().Utf8Value();
    auto port = info[1].As<Napi::Number>().Int32Value();
    communicationUuid.init(3, 1);
    startInner(env, host, port);
    
    // Start heartbeat monitoring
    startHeartbeat();
    
    return env.Undefined();
}
void stop(const Napi::CallbackInfo &info) {
    if (acceptor) {
        if (messageHandleTsfn) {
            messageHandleTsfn.Release();
        }

        if (client && client->is_open()) {
            client->close();
        }
        client.reset();
        client_connected = false;

        acceptor->close();
        io_context.stop();
        logger->info("Socket server stopped");
    }
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
        Napi::Persistent(info[0].As<Napi::Function>()));    logger->info("Set protobuf message callback");
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
    logger->info("send protobuf message to client, id: {}", message.id());

    logger->debug("Send msg to client: {}", message.DebugString());
    if (client && client->is_open()) {
        // Serialize the message and send it
        std::string serializedMessage = message.SerializeAsString();
        std::uint32_t messageLength = static_cast<std::uint32_t>(serializedMessage.size());
        boost::asio::write(*client, boost::asio::buffer(&messageLength, sizeof(messageLength)));
        boost::asio::write(*client, boost::asio::buffer(serializedMessage));
    } else {
        logger->warn("No client connected or client is not open");
    }
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

    auto id = std::to_string(communicationUuid.nextid());
    message.set_id(id);
    if (!client || !client->is_open()) {
        throw Napi::Error::New(info.Env(), "No client connected");
    }

    // 先存储，再发送
    auto promiseObj = std::make_shared<std::promise<skyline::Message>>();
    std::future<skyline::Message> futureObj = promiseObj->get_future();

    {
        // 使用作用域限制插入操作的临界区范围
        socketRequest.emplace(id, promiseObj);
    }

    logger->info("send protobuf sync message to client, id: {}", id);
    logger->debug("id: {}, content: {}", id, message.DebugString());

    // 发送消息给单一客户端
    if (client && client->is_open()) {
        // Serialize the message and send it
        std::string serializedMessage = message.SerializeAsString();
        std::uint32_t messageLength = static_cast<std::uint32_t>(serializedMessage.size());
        boost::asio::write(*client, boost::asio::buffer(&messageLength, sizeof(messageLength)));
        boost::asio::write(*client, boost::asio::buffer(serializedMessage));
    } else {
        socketRequest.erase(id);
        throw Napi::Error::New(info.Env(),
                                "Client disconnected while sending");
    }

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
        }        auto status = futureObj.wait_for(std::chrono::milliseconds(0));
        if (status == std::future_status::ready) {
            break;
        }

        // 检查队列中是否有待处理的消息
        skyline::Message queuedMsg;
        bool hasMessage = false;
        {
            std::lock_guard<std::mutex> lock(blockQueueMutex);
            if (!blockQueue.empty()) {
                queuedMsg = blockQueue.front();
                blockQueue.pop();
                hasMessage = true;
            }
        }
        
        if (hasMessage) {
            try {
                // debug消息
                // Client发来的消息
                logger->debug("start to handle blocked protobuf message: {}", queuedMsg.id());
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
    logger->debug("id: {}, data: {}", id, result.DebugString());

    // Convert the Protobuf message to a JavaScript value
    auto resultObj = Convert::convertProtobufToJs(env, result);
    
    return resultObj.Get("data").As<Napi::Object>()
    .Get("result").As<Napi::Object>()
    .Get("returnValue");
}

std::string readLengthPrefixedMessage(std::shared_ptr<tcp::socket> socket_ptr) {
    boost::asio::streambuf buffer;
    std::istream stream(&buffer);
    std::string message;

    // Read the length prefix (4 bytes)
    char lengthPrefix[4];
    boost::asio::read(*socket_ptr, boost::asio::buffer(lengthPrefix, 4));
    logger->info("Read length prefix from client");

    // Convert the length prefix to an integer
    std::uint32_t messageLength = 0;
    std::memcpy(&messageLength, lengthPrefix, sizeof(messageLength));
    // Note: Using native byte order since client sends in native format
    logger->info("Message length: {}", messageLength);

    // Read the message body
    message.resize(messageLength);
    boost::asio::read(*socket_ptr, boost::asio::buffer(message));
    logger->info("Read message body, actual size: {}", message.size());    return message;
}

// Add heartbeat functionality for connection monitoring using Protobuf
void startHeartbeat() {
    std::thread([]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(30)); // 30 second heartbeat
            
            if (client && client->is_open() && client_connected) {                try {
                    // Create a Protobuf heartbeat message using STATIC type
                    skyline::Message heartbeat;
                    heartbeat.set_type(skyline::MessageType::STATIC);
                    heartbeat.set_id(std::to_string(communicationUuid.nextid()));
                    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    
                    // Use static data for heartbeat
                    auto* staticData = heartbeat.mutable_static_data();
                    staticData->set_clazz("heartbeat");
                    staticData->set_action("ping");
                    
                    // Create a value for timestamp
                    auto* timestampValue = staticData->add_params();
                    timestampValue->set_double_value(static_cast<double>(timestamp));

                    // Send the protobuf heartbeat
                    std::string serializedMessage = heartbeat.SerializeAsString();
                    std::uint32_t messageLength = static_cast<std::uint32_t>(serializedMessage.size());
                    boost::asio::write(*client, boost::asio::buffer(&messageLength, sizeof(messageLength)));
                    boost::asio::write(*client, boost::asio::buffer(serializedMessage));
                } catch (const std::exception& e) {
                    logger->warn("Heartbeat failed, client may be disconnected: {}", e.what());
                    client.reset();
                    client_connected = false;
                }
            } else {
                // No client connected, sleep longer
                std::this_thread::sleep_for(std::chrono::seconds(30));
            }
        }
    }).detach();
}

} // namespace SocketServer
