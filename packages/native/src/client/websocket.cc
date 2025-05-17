#include <chrono>
#include <map>
#include <queue>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <future>
#include <memory>
#include <string>
#include <napi.h>
#include <thread>
#include <mutex>
#include "../common/snowflake.hh"
#include "../common/convert.hh"
#include "../common/logger.hh"

// Socket headers
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

using Logger::logger;

namespace WebSocket {
    // Socket-related variables
    static SOCKET socketFd = INVALID_SOCKET;
    static bool isConnected = false;
    static std::thread receiveThread;
    static bool shouldRun = true;
    
    // 添加互斥锁以保护共享资源
    static std::mutex socketMutex;
    static std::mutex callbackQueueMutex;
    static std::mutex wsRequestMutex;
    static std::mutex connectionMutex;
    
    static std::map<std::string, std::shared_ptr<std::promise<std::string>>> wsRequest;

    using snowflake_t = snowflake<1534832906275L>;
    snowflake_t serverCommunicationUuid;
    std::string serverUrl = "127.0.0.1"; // Changed to just IP without protocol
    int serverPort = 3001;               // Added port variable
    std::queue<nlohmann::json> callbackQueue;
    bool blocked = false;

    // Function to initialize Socket system on Windows
    bool initSocketSystem() {
#ifdef _WIN32
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            logger->error("WSAStartup failed with error: {}", result);
            return false;
        }
        return true;
#else
        return true;
#endif
    }

    // Function to safely close socket
    void closeSocket(SOCKET sock) {
        if (sock != INVALID_SOCKET) {
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
        }
    }

    // Function to clean up socket system
    void cleanupSocketSystem() {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    // Add this function before the receiveThreadFunction to process messages in threads
    void processMessageInThread(std::string completeMessage) {
        try {
            nlohmann::json json = nlohmann::json::parse(completeMessage);
            
            if (json.contains("id") && !json.contains("type")) {
                // Response to a request
                auto id = json["id"];
                logger->info("Received response for message: {}", id.get<std::string>());
                
                {
                    auto it = wsRequest.find(id);
                    if (it != wsRequest.end()) {
                        it->second->set_value(completeMessage);
                        // std::lock_guard<std::mutex> lock(wsRequestMutex);
                        wsRequest.erase(it);
                    } else {
                        logger->error("ID not found: {}", id.get<std::string>());
                    }
                }
            } else if (json.contains("type")) {
                bool isCurrentlyBlocked = false;
                {
                    // std::lock_guard<std::mutex> lock(callbackQueueMutex);
                    isCurrentlyBlocked = blocked;
                    
                    if (isCurrentlyBlocked) {
                        // Queue message if we're blocked
                        logger->info("Blocked, pushing to queue...");
                        callbackQueue.push(json);
                    }
                }
                
                if (!isCurrentlyBlocked) {
                    // Notification message
                    std::string type = json["type"].get<std::string>();
                    if (type == "emitCallback") {
                        // Callback trigger
                        std::string callbackId = json["callbackId"].get<std::string>();
                        auto ptr = Convert::find_callback(callbackId);
                        
                        if (ptr != nullptr) {
                            logger->info("CallbackId found: {}", callbackId);
                            
                            auto block = json["data"]["block"];
                            if (block.is_boolean() && block.get<bool>() == false) {
                                ptr->tsfn.NonBlockingCall([json](Napi::Env env, Napi::Function jsCallback) {
                                    auto args = json["data"]["args"];
                                    std::string callbackId = json["callbackId"].get<std::string>();
                                    Napi::HandleScope scope(env);
                                    std::vector<Napi::Value> argsVec;
                                    
                                    for (auto& arg : args) {
                                        argsVec.push_back(Convert::convertJson2Value(env, arg));
                                    }
                                    
                                    logger->info("Calling callback function...");
                                    auto resultValue = jsCallback.Call(argsVec);
                                    logger->info("Callback function call complete");
                                    
                                    // Send callback result
                                    auto resultJson = Convert::convertValue2Json(env, resultValue);
                                    if (json.contains("id")) {
                                        logger->info("Replying to callback...");
                                        nlohmann::json callbackResult = {
                                            {"id", json["id"].get<std::string>()},
                                            {"type", "callbackReply"},
                                            {"result", resultJson}
                                        };
                                        
                                        std::string callbackContent = callbackResult.dump();
                                        uint32_t callbackLength = static_cast<uint32_t>(callbackContent.length());
                                        
                                        // Create length prefix
                                        char lengthPrefix[4];
                                        lengthPrefix[0] = (callbackLength >> 24) & 0xFF;
                                        lengthPrefix[1] = (callbackLength >> 16) & 0xFF;
                                        lengthPrefix[2] = (callbackLength >> 8) & 0xFF;
                                        lengthPrefix[3] = callbackLength & 0xFF;
                                        
                                        // Create full message
                                        std::string message(lengthPrefix, 4);
                                        message.append(callbackContent);
                                        
                                        // std::lock_guard<std::mutex> lock(socketMutex);
                                        if (socketFd != INVALID_SOCKET) {
                                            int sendResult = send(socketFd, message.c_str(), message.length(), 0);
                                            if (sendResult == SOCKET_ERROR) {
#ifdef _WIN32
                                                logger->error("Error sending callback response: {}", WSAGetLastError());
#else
                                                logger->error("Error sending callback response: {}", errno);
#endif
                                            } else {
                                                logger->info("Callback response sent successfully");
                                            }
                                        } else {
                                            logger->error("Cannot send callback response: socket is invalid");
                                        }
                                    }
                                });
                            } else {
                                ptr->tsfn.BlockingCall([json](Napi::Env env, Napi::Function jsCallback) {
                                    auto args = json["data"]["args"];
                                    std::string callbackId = json["callbackId"].get<std::string>();
                                    Napi::HandleScope scope(env);
                                    std::vector<Napi::Value> argsVec;
                                    
                                    for (auto& arg : args) {
                                        argsVec.push_back(Convert::convertJson2Value(env, arg));
                                    }
                                    
                                    logger->info("Calling callback function...");
                                    auto resultValue = jsCallback.Call(argsVec);
                                    logger->info("Callback function call complete");
                                    
                                    // Send callback result
                                    auto resultJson = Convert::convertValue2Json(env, resultValue);
                                    if (json.contains("id")) {
                                        logger->info("Replying to callback...");
                                        nlohmann::json callbackResult = {
                                            {"id", json["id"].get<std::string>()},
                                            {"type", "callbackReply"},
                                            {"result", resultJson}
                                        };
                                        
                                        std::string callbackContent = callbackResult.dump();
                                        uint32_t callbackLength = static_cast<uint32_t>(callbackContent.length());
                                        
                                        // Create length prefix
                                        char lengthPrefix[4];
                                        lengthPrefix[0] = (callbackLength >> 24) & 0xFF;
                                        lengthPrefix[1] = (callbackLength >> 16) & 0xFF;
                                        lengthPrefix[2] = (callbackLength >> 8) & 0xFF;
                                        lengthPrefix[3] = callbackLength & 0xFF;
                                        
                                        // Create full message
                                        std::string message(lengthPrefix, 4);
                                        message.append(callbackContent);
                                        
                                        // std::lock_guard<std::mutex> lock(socketMutex);
                                        if (socketFd != INVALID_SOCKET) {
                                            int sendResult = send(socketFd, message.c_str(), message.length(), 0);
                                            if (sendResult == SOCKET_ERROR) {
#ifdef _WIN32
                                                logger->error("Error sending callback response: {}", WSAGetLastError());
#else
                                                logger->error("Error sending callback response: {}", errno);
#endif
                                            } else {
                                                logger->info("Callback response sent successfully");
                                            }
                                        } else {
                                            logger->error("Cannot send callback response: socket is invalid");
                                        }
                                    }
                                });
                            }
                        } else {
                            logger->error("CallbackId not found: {}", callbackId);
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            logger->error("Failed to parse message: {}", e.what());
        } catch (...) {
            logger->error("Unknown error occurred while processing message");
        }
    }

    // Receive thread function
    void receiveThreadFunction() {
        char buffer[8192];
        std::string message;
        
        while (shouldRun && isConnected) {
            SOCKET localSocketFd;
            {
                // std::lock_guard<std::mutex> lock(socketMutex);
                localSocketFd = socketFd;
            }
            
            if (localSocketFd == INVALID_SOCKET) {
                logger->error("Socket is invalid in receive thread");
                
                {
                    // std::lock_guard<std::mutex> lock(connectionMutex);
                    isConnected = false;
                }
                
                break;
            }
            
            // Length-prefixed message processing
            // First, try to read the message header (4-byte length)
            if (message.size() < 4) {
                memset(buffer, 0, sizeof(buffer));
                int bytesReceived = recv(localSocketFd, buffer, sizeof(buffer) - 1, 0);
                
                if (bytesReceived > 0) {
                    buffer[bytesReceived] = '\0';
                    message.append(buffer, bytesReceived);
                } else if (bytesReceived == 0) {
                    // Connection closed
                    logger->info("Socket connection closed by server");
                    
                    {
                        // std::lock_guard<std::mutex> lock(connectionMutex);
                        isConnected = false;
                    }
                    
                    break;
                } else {
                    // Error occurred
#ifdef _WIN32
                    logger->error("Socket receive error: {}", WSAGetLastError());
#else
                    logger->error("Socket receive error: {}", errno);
#endif
                    
                    {
                        // std::lock_guard<std::mutex> lock(connectionMutex);
                        isConnected = false;
                    }
                    
                    break;
                }
            }
            
            // Keep processing messages as long as we have complete ones
            while (message.size() >= 4) {
                // Extract message length from the first 4 bytes
                uint32_t messageLength = 0;
                messageLength |= static_cast<uint32_t>(static_cast<uint8_t>(message[0])) << 24;
                messageLength |= static_cast<uint32_t>(static_cast<uint8_t>(message[1])) << 16;
                messageLength |= static_cast<uint32_t>(static_cast<uint8_t>(message[2])) << 8;
                messageLength |= static_cast<uint32_t>(static_cast<uint8_t>(message[3]));
                
                // Check if we have received the complete message
                if (message.size() < messageLength + 4) {
                    // Need more data, read again
                    memset(buffer, 0, sizeof(buffer));
                    int bytesReceived = recv(localSocketFd, buffer, sizeof(buffer) - 1, 0);
                    
                    if (bytesReceived > 0) {
                        buffer[bytesReceived] = '\0';
                        message.append(buffer, bytesReceived);
                    } else if (bytesReceived == 0) {
                        // Connection closed
                        logger->info("Socket connection closed by server");
                        
                        {
                            // std::lock_guard<std::mutex> lock(connectionMutex);
                            isConnected = false;
                        }
                        
                        break;
                    } else {
                        // Error occurred
#ifdef _WIN32
                        logger->error("Socket receive error: {}", WSAGetLastError());
#else
                        logger->error("Socket receive error: {}", errno);
#endif
                        
                        {
                            // std::lock_guard<std::mutex> lock(connectionMutex);
                            isConnected = false;
                        }
                        
                        break;
                    }
                    continue;
                }
                
                // We have a complete message
                std::string completeMessage = message.substr(4, messageLength);
                logger->info("Received message: {}", completeMessage);
                
                // Remove the processed message (including length prefix) from the buffer
                message.erase(0, messageLength + 4);
                
                // Create a new thread to process the message
                std::thread messageThread([completeMessage]() {
                    processMessageInThread(completeMessage);
                });
                messageThread.detach();  // Detach the thread so it can run independently
                logger->info("Wait next message");
            }
            
            // Small sleep to avoid high CPU usage
            // std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    
    // 安全清理所有资源
    void cleanup() {
        logger->info("Cleaning up WebSocket resources");
        
        // 停止接收线程
        shouldRun = false;
        
        // 关闭连接
        {
            // std::lock_guard<std::mutex> lock(connectionMutex);
            isConnected = false;
        }
        
        // 关闭套接字
        {
            // std::lock_guard<std::mutex> lock(socketMutex);
            if (socketFd != INVALID_SOCKET) {
                closeSocket(socketFd);
                socketFd = INVALID_SOCKET;
            }
        }
        
        // 等待接收线程结束
        if (receiveThread.joinable()) {
            receiveThread.join();
        }
        
        // 清理请求
        {
            // std::lock_guard<std::mutex> lock(wsRequestMutex);
            for (auto& request : wsRequest) {
                try {
                    request.second->set_exception(std::make_exception_ptr(std::runtime_error("Connection closed")));
                } catch (const std::exception&) {
                    // 忽略异常
                }
            }
            wsRequest.clear();
        }
        
        // 清理回调队列
        {
            // std::lock_guard<std::mutex> lock(callbackQueueMutex);
            while (!callbackQueue.empty()) {
                callbackQueue.pop();
            }
            blocked = false;
        }
        
        // 清理套接字系统
        cleanupSocketSystem();
    }

    void initWebSocket(Napi::Env &env) {
        // 清理任何现有资源
        cleanup();
        
        // Initialize socket system
        if (!initSocketSystem()) {
            throw Napi::Error::New(env, "Failed to initialize socket system");
        }

        serverCommunicationUuid.init(1, 1);
        
        // Create socket
        {
            // std::lock_guard<std::mutex> lock(socketMutex);
            socketFd = socket(AF_INET, SOCK_STREAM, 0);
            if (socketFd == INVALID_SOCKET) {
#ifdef _WIN32
                int error = WSAGetLastError();
                cleanupSocketSystem();
                throw Napi::Error::New(env, "Socket creation failed with error: " + std::to_string(error));
#else
                cleanupSocketSystem();
                throw Napi::Error::New(env, "Socket creation failed with error: " + std::to_string(errno));
#endif
            }
        }
        
        // Connect to server
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        
        // Convert IP address from string to network format
#ifdef _WIN32
        inet_pton(AF_INET, serverUrl.c_str(), &(serverAddr.sin_addr));
#else
        serverAddr.sin_addr.s_addr = inet_addr(serverUrl.c_str());
#endif
        
        logger->info("Connecting to {}:{}", serverUrl, serverPort);
        
        // Connect to server
        {
            // std::lock_guard<std::mutex> lock(socketMutex);
            if (connect(socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
#ifdef _WIN32
                int error = WSAGetLastError();
                closesocket(socketFd);
                socketFd = INVALID_SOCKET;
                cleanupSocketSystem();
                throw Napi::Error::New(env, "Connection failed with error: " + std::to_string(error));
#else
                closesocket(socketFd);
                socketFd = INVALID_SOCKET;
                cleanupSocketSystem();
                throw Napi::Error::New(env, "Connection failed with error: " + std::to_string(errno));
#endif
            }
        }
        
        {
            // std::lock_guard<std::mutex> lock(connectionMutex);
            isConnected = true;
        }
        
        logger->info("Connected to server");
        
        // Start receive thread
        shouldRun = true;
        receiveThread = std::thread(receiveThreadFunction);
        
        // Wait for connection to be established
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        bool isSocketConnected = false;
        {
            // std::lock_guard<std::mutex> lock(connectionMutex);
            isSocketConnected = isConnected;
        }
        
        if (!isSocketConnected) {
            cleanup();
            throw Napi::Error::New(env, "Socket connection failed");
        }
    }
    nlohmann::json sendMessageSync(nlohmann::json& data) {
        std::string id = std::to_string(serverCommunicationUuid.nextid());
        data["id"] = id;
        std::string msg = data.dump();
        logger->info("send to server {}", msg);
        // Sleep(1000);
        bool isSocketConnected = false;
        {
            // std::lock_guard<std::mutex> lock(connectionMutex);
            isSocketConnected = isConnected;
        }
        
        if (!isSocketConnected) {
            throw std::runtime_error("Socket is not connected");
        }
        
        {
            // std::lock_guard<std::mutex> lock(callbackQueueMutex);
            blocked = true;
        }
        
        std::string messageContent = data.dump();
        uint32_t messageLength = static_cast<uint32_t>(messageContent.length());
        
        // Create the length prefix (4 bytes, big-endian)
        char lengthPrefix[4];
        lengthPrefix[0] = (messageLength >> 24) & 0xFF;
        lengthPrefix[1] = (messageLength >> 16) & 0xFF;
        lengthPrefix[2] = (messageLength >> 8) & 0xFF;
        lengthPrefix[3] = messageLength & 0xFF;
        
        // Prepare the full message with length prefix
        std::string message(lengthPrefix, 4);
        message.append(messageContent);
        
        
        {
            int bytesSent;
            // std::lock_guard<std::mutex> lock(socketMutex);
            if (socketFd == INVALID_SOCKET) {
                // std::lock_guard<std::mutex> lock(callbackQueueMutex);
                blocked = false;
                throw std::runtime_error("Socket is invalid");
            }
            bytesSent = send(socketFd, message.c_str(), message.length(), 0);
            logger->info("Send bytes {}, total length {}", bytesSent, message.length());
            if (bytesSent < 0) {
                // std::lock_guard<std::mutex> lock(callbackQueueMutex);
                blocked = false;
    #ifdef _WIN32
                throw std::runtime_error("Failed to send data to server: " + std::to_string(WSAGetLastError()));
    #else
                throw std::runtime_error("Failed to send data to server: " + std::to_string(errno));
    #endif
            }
        }
        
        auto promiseObj = std::make_shared<std::promise<std::string>>();
        std::future<std::string> futureObj = promiseObj->get_future();
        
        {
            // std::lock_guard<std::mutex> lock(wsRequestMutex);
            logger->info("Adding request to wsRequest map with ID: {}", id);
            wsRequest.emplace(id, promiseObj);
            if (wsRequest.find(id) == wsRequest.end()) {
                logger->error("Failed to add request to wsRequest map");
            }
        }
        
        auto start = std::chrono::steady_clock::now();
        logger->info("Waiting for response and checking blockQueue...");
        while (true) {
            // if (callbackQueue.empty()) {
            //     logger->info("Check sleep...");
            //     // Small sleep to avoid high CPU usage
            //     std::this_thread::sleep_for(std::chrono::microseconds(1));
            //     logger->info("Check sleep end.");
            // }
            // else
            if (!callbackQueue.empty())
            {
                try {
                    nlohmann::json queuedJson = callbackQueue.front();
                    callbackQueue.pop();
                    logger->info("Processing queued callback...");
                    
                    // Make a local copy of the JSON data to prevent any potential issues with the original being modified
                    nlohmann::json queuedJsonCopy = queuedJson;
                    
                    // Extract necessary information before callback execution
                    std::string callbackId;
                    std::string responseId;
                    
                    try {
                        callbackId = queuedJsonCopy["callbackId"].get<std::string>();
                        responseId = queuedJsonCopy.contains("id") ? queuedJsonCopy["id"].get<std::string>() : "";
                    } catch (const std::exception& e) {
                        logger->error("Error parsing callback data: {}", e.what());
                        continue; // Skip this callback and move to the next item in the queue
                    }
                    
                    // Find the callback before doing any work
                    auto callbackPtr = Convert::find_callback(callbackId);
                    if (!callbackPtr) {
                        logger->error("CallbackId not found: {}", callbackId);
                        continue; // Skip this callback and move to the next item in the queue
                    }
                    
                    logger->info("CallbackId found: {}", callbackId);
                    
                    // Create a strong reference to the function reference to prevent it from being destroyed
                    std::shared_ptr<Napi::FunctionReference> funcRef = callbackPtr->funcRef;
                    if (!funcRef) {
                        logger->error("Function reference is null for callback: {}", callbackId);
                        continue;
                    }
                    
                    // Get the environment and create a scope
                    try {
                        // Direct initialization without default constructor
                        Napi::Env callbackEnv = funcRef->Env();
                        
                        // Create a new handle scope to manage JS object lifetimes
                        Napi::HandleScope scope(callbackEnv);
                        
                        // Create arguments for the callback
                        std::vector<Napi::Value> argsVec;
                        try {
                            auto& args = queuedJsonCopy["data"]["args"];
                            for (auto& arg : args) {
                                argsVec.push_back(Convert::convertJson2Value(callbackEnv, arg));
                            }
                        } catch (const std::exception& e) {
                            logger->error("Error converting arguments: {}", e.what());
                            continue;
                        }
                        
                        // Call the JavaScript function with proper error handling
                        Napi::Value resultValue;
                        try {
                            logger->info("Calling callback function...");
                            resultValue = funcRef->Value().Call(argsVec);
                            logger->info("Callback function call complete");
                        } catch (const Napi::Error& e) {
                            logger->error("JavaScript error during callback execution: {}", e.Message());
                            continue;
                        } catch (const std::exception& e) {
                            logger->error("C++ error during callback execution: {}", e.what());
                            continue;
                        } catch (...) {
                            logger->error("Unknown error during callback execution");
                            continue;
                        }
                        
                        // Only send response if there's an ID to reply to
                        if (!responseId.empty()) {
                            try {
                                // Convert the result to JSON
                                auto resultJson = Convert::convertValue2Json(callbackEnv, resultValue);
                                
                                // Prepare the response
                                logger->info("Replying to callback...");
                                nlohmann::json callbackResult = {
                                    {"id", responseId},
                                    {"type", "callbackReply"},
                                    {"result", resultJson}
                                };
                                
                                // Convert to string
                                std::string callbackContent = callbackResult.dump();
                                uint32_t callbackLength = static_cast<uint32_t>(callbackContent.length());
                                
                                // Create length prefix
                                char lengthPrefix[4];
                                lengthPrefix[0] = (callbackLength >> 24) & 0xFF;
                                lengthPrefix[1] = (callbackLength >> 16) & 0xFF;
                                lengthPrefix[2] = (callbackLength >> 8) & 0xFF;
                                lengthPrefix[3] = callbackLength & 0xFF;
                                
                                // Create full message
                                std::string message(lengthPrefix, 4);
                                message.append(callbackContent);
                                
                                // Create a copy of the message to be sent
                                std::string messageCopy = message;
                                
                                // Send the response within its own scope to ensure the lock is released
                                {
                                    // std::lock_guard<std::mutex> lock(socketMutex);
                                    if (socketFd != INVALID_SOCKET) {
                                        int sendResult = send(socketFd, messageCopy.c_str(), messageCopy.length(), 0);
                                        if (sendResult == SOCKET_ERROR) {
    #ifdef _WIN32
                                            logger->error("Error sending queued callback response: {}", WSAGetLastError());
    #else
                                            logger->error("Error sending queued callback response: {}", errno);
    #endif
                                        } else {
                                            logger->info("Queued callback response sent successfully");
                                        }
                                    } else {
                                        logger->error("Cannot send queued callback response: socket is invalid");
                                    }
                                }
                            } catch (const std::exception& e) {
                                logger->error("Error preparing or sending response: {}", e.what());
                            } catch (...) {
                                logger->error("Unknown error preparing or sending response");
                            }
                        }
                        
                        // Add a small delay after all processing to ensure resources are fully released
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        
                    } catch (const std::exception& e) {
                        logger->error("Error processing queued callback: {}", e.what());
                    } catch (...) {
                        logger->error("Unknown error processing queued callback");
                    }
                } catch (const std::exception& e) {
                    logger->error("Error processing queued callback: {}", e.what());
                } catch (...) {
                    logger->error("Unknown error processing queued callback");
                }
            }
            
            if (futureObj.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                logger->info("Future object is ready");
                break;
            }
            auto delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
            if (delta_ms > 5000) {
                {
                    // std::lock_guard<std::mutex> lock(wsRequestMutex);
                    wsRequest.erase(id);
                }
                
                {
                    // std::lock_guard<std::mutex> lock(callbackQueueMutex);
                    blocked = false;
                }
                logger->error("Operation timed out after 5 seconds, request data:\n" + data.dump());
                throw std::runtime_error("Operation timed out after 5 seconds, request data:\n" + data.dump());
            }
        }
        
        std::string result = futureObj.get();
        
        blocked = false;
        
        logger->info("Received result: {}", result);
        
        if (result.empty()) {
            throw std::runtime_error("Server response is empty");
        }
        
        auto resp = nlohmann::json::parse(result);
        if (resp.contains("error")) {
            throw std::runtime_error("Server response error: " + resp["error"].get<std::string>());
        }
        
        return resp["result"];
    }

    void sendMessageAsync(nlohmann::json& data) {
        std::string id = std::to_string(serverCommunicationUuid.nextid());
        data["id"] = id;
        logger->info("Sending async message to server: {}", data.dump());
        
        bool isSocketConnected = false;
        {
            // std::lock_guard<std::mutex> lock(connectionMutex);
            isSocketConnected = isConnected;
        }
        
        if (!isSocketConnected) {
            throw std::runtime_error("Socket is not connected");
        }
        
        // 创建消息的副本，以确保在线程中安全使用
        std::string messageContent = data.dump();
        uint32_t messageLength = static_cast<uint32_t>(messageContent.length());
        
        // Create the length prefix (4 bytes, big-endian)
        char lengthPrefix[4];
        lengthPrefix[0] = (messageLength >> 24) & 0xFF;
        lengthPrefix[1] = (messageLength >> 16) & 0xFF;
        lengthPrefix[2] = (messageLength >> 8) & 0xFF;
        lengthPrefix[3] = messageLength & 0xFF;
        
        // Prepare the full message with length prefix
        std::string message(lengthPrefix, 4);
        message.append(messageContent);
        
        std::thread t([message]() {
            // std::lock_guard<std::mutex> lock(socketMutex);
            if (socketFd != INVALID_SOCKET) {
                int ret = send(socketFd, message.c_str(), message.length(), 0);
                if (ret == SOCKET_ERROR) {
                    logger->error("Error sending async message");
                }
            } else {
                logger->error("Socket invalid during async send");
            }
        });
        t.detach();
    }

    // The remaining functions stay the same, just using the socket implementation instead of WebSockets
    void callDynamicAsync(const std::string& instanceId, const std::string& action, nlohmann::json& args) {
        nlohmann::json json {
            {"type", "dynamic"},
            {"action", action},
            {"data", {
                {"instanceId", instanceId},
                {"params", args}
            }}
        };
        
        sendMessageAsync(json);
    }

    nlohmann::json callConstructorSync(const std::string& clazz, nlohmann::json& args) {
        nlohmann::json json {
            {"type", "constructor"},
            {"clazz", clazz},
            {"data", {
                {"params", args}
            }}
        };
        
        return sendMessageSync(json);
    }

    nlohmann::json callDynamicSync(const std::string& instanceId, const std::string& action, nlohmann::json& args) {
        nlohmann::json json {
            {"type", "dynamic"},
            {"action", action},
            {"data", {
                {"instanceId", instanceId},
                {"params", args}
            }}
        };
        
        return sendMessageSync(json);
    }

    nlohmann::json callDynamicPropertySetSync(const std::string& instanceId, const std::string& action, nlohmann::json& args) {
        nlohmann::json json {
            {"type", "dynamicProperty"},
            {"action", action},
            {"data", {
                {"instanceId", instanceId},
                {"params", args},
                {"propertyAction", "set"},
            }}
        };
        
        return sendMessageSync(json);
    }

    nlohmann::json callDynamicPropertyGetSync(const std::string& instanceId, const std::string& action) {
        nlohmann::json json {
            {"type", "dynamicProperty"},
            {"action", action},
            {"data", {
                {"instanceId", instanceId},
                {"propertyAction", "get"},
            }}
        };
        
        return sendMessageSync(json);
    }

    nlohmann::json callStaticSync(const std::string& clazz, const std::string& action, nlohmann::json& args) {
        nlohmann::json json {
            {"type", "static"},
            {"clazz", clazz},
            {"action", action},
            {"data", {
                {"params", args}
            }}
        };
        
        return sendMessageSync(json);
    }

    nlohmann::json callCustomHandleSync(const std::string& action, nlohmann::json& args) {
        return callStaticSync("customHandle", action, args);
    }
}