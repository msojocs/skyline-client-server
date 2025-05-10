#include "websocket.hh"
#include "napi.h"
#include <memory>
#include "../common/logger.hh"
#include <nlohmann/json.hpp>
#include <future>
#include <queue>
#include <vector>
#include <thread>
#include "../common/snowflake.hh"
#include "../common/convert.hh"

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

namespace WebSocketServer {

static SOCKET serverSocket = INVALID_SOCKET;
static std::vector<SOCKET> clientSockets;
static std::mutex clientSocketsMutex;
static std::thread acceptThread;
static std::thread receiveThread;
static bool shouldRun = true;

// 添加一个用于存储每个客户端消息缓冲区的映射
static std::map<SOCKET, std::string> clientBuffers;
static std::mutex clientBuffersMutex;

static Napi::ThreadSafeFunction websocketMessageHandleTsfn;
static std::shared_ptr<Napi::FunctionReference> websocketMessageHandleRef = nullptr;
static std::map<std::string, std::shared_ptr<std::promise<std::string>>> wsRequest;
static std::queue<std::string> blockQueue;
static bool isBlock = false;

using snowflake_t = snowflake<1534832906275L>;
snowflake_t communicationUuid;

// Function to initialize socket system on Windows
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

// Function to clean up socket system
void cleanupSocketSystem() {
#ifdef _WIN32
    WSACleanup();
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

// Send message to all clients
void sendMsg(const Napi::CallbackInfo &info) {
    if (info.Length() < 1) {
        throw Napi::TypeError::New(info.Env(), "sendMsg: Wrong number of arguments");
    }
    if (!info[0].IsString()) {
        throw Napi::TypeError::New(info.Env(), "First argument must be a string");
    }
    
    auto message = info[0].As<Napi::String>().Utf8Value();
    
    std::lock_guard<std::mutex> lock(clientSocketsMutex);
    
    if (clientSockets.empty()) {
        throw Napi::Error::New(info.Env(), "No clients connected");
    }
    
    logger->info("Sending to client: {}", message);
    
    // Prepare message with length prefix
    uint32_t messageLength = static_cast<uint32_t>(message.length());
    
    // Create length prefix (4 bytes, big-endian)
    char lengthPrefix[4];
    lengthPrefix[0] = (messageLength >> 24) & 0xFF;
    lengthPrefix[1] = (messageLength >> 16) & 0xFF;
    lengthPrefix[2] = (messageLength >> 8) & 0xFF;
    lengthPrefix[3] = messageLength & 0xFF;
    
    // Create the complete message with length prefix
    std::string completeMessage(lengthPrefix, 4);
    completeMessage.append(message);
    logger->info("Client count: {}", clientSockets.size());
    for (auto it = clientSockets.begin(); it != clientSockets.end();) {
        SOCKET clientSocket = *it;
        
        int result = send(clientSocket, completeMessage.c_str(), completeMessage.length(), 0);
        
        if (result == SOCKET_ERROR) {
            // Error occurred, remove client
            logger->error("Error sending message to client, removing client");
            closeSocket(clientSocket);
            it = clientSockets.erase(it);
        } else {
            ++it;
        }
    }
    logger->info("Send to client end.");
}

// Thread function to accept new client connections
void acceptClientConnections(const std::string &host, int port) {
    while (shouldRun) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        
        if (clientSocket == INVALID_SOCKET) {
            if (shouldRun) {
                logger->error("Accept failed");
            }
            break;
        }
        
        char clientIP[INET_ADDRSTRLEN];
#ifdef _WIN32
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
#else
        strcpy(clientIP, inet_ntoa(clientAddr.sin_addr));
#endif
        
        logger->info("New client connected from {}", clientIP);
        
        std::lock_guard<std::mutex> lock(clientSocketsMutex);
        clientSockets.push_back(clientSocket);
    }
}

// Function to process messages in a separate thread
void processMessageInThread(std::string completeMessage, SOCKET clientSocket) {
      // 客户端发起的消息，传递给JavaScript回调
      auto callback = [completeMessage, clientSocket](Napi::Env env, Napi::Function jsCallback) {
          auto ws = Napi::Object::New(env);
          ws.Set("send", Napi::Function::New(env, sendMsg));
          
          try {
              jsCallback.Call({ws, Napi::String::New(env, completeMessage)});
          } catch (const std::exception &e) {
              logger->error("Error in callback: {}", e.what());
          } catch (...) {
              logger->error("Unknown error occurred in callback");
          }
      };
      
      websocketMessageHandleTsfn.BlockingCall(callback);
  
}

// 处理接收到的消息并查找完整的长度前缀消息
void processBuffer(SOCKET clientSocket, const std::string& message) {
    // 获取或创建该客户端的消息缓冲区
    std::string& buffer = [&]() -> std::string& {
        std::lock_guard<std::mutex> lock(clientBuffersMutex);
        return clientBuffers[clientSocket];
    }();
    
    // 追加新接收的数据到缓冲区
    buffer.append(message);
    
    // 处理长度前缀消息
    while (buffer.size() >= 4) {
        // 从前4个字节解析消息长度
        uint32_t messageLength = 0;
        messageLength |= static_cast<uint32_t>(static_cast<uint8_t>(buffer[0])) << 24;
        messageLength |= static_cast<uint32_t>(static_cast<uint8_t>(buffer[1])) << 16;
        messageLength |= static_cast<uint32_t>(static_cast<uint8_t>(buffer[2])) << 8;
        messageLength |= static_cast<uint32_t>(static_cast<uint8_t>(buffer[3]));
        
        // 检查缓冲区是否包含完整消息
        if (buffer.size() < messageLength + 4) {
            // 消息还不完整，等待更多数据
            logger->info("Message incomplete, waiting for more data. Have {} bytes, need {} bytes", 
                        buffer.size(), messageLength + 4);
            return;
        }
        
        // 提取完整的消息内容（不包含长度前缀）
        std::string completeMessage = buffer.substr(4, messageLength);
        
        logger->info("Found complete message: {}", completeMessage);
        
        // 处理完整消息
        try {
            nlohmann::json json = nlohmann::json::parse(completeMessage);
            if (json.contains("id")) {
                auto id = json["id"];
                logger->info("Received message: {}", json["id"].get<std::string>());
                
                if (wsRequest.find(id) != wsRequest.end()) {
                    logger->info("Found ID: {}", id.get<std::string>());
                    // 这是对服务器发起请求的响应
                    wsRequest[id]->set_value(completeMessage);
                    wsRequest.erase(id);
                    buffer.erase(0, messageLength + 4);
                    return;
                } else {
                    logger->info("ID not found: {}", id.get<std::string>());
                }
            }
            if (isBlock) {
                // 如果当前处于阻塞状态，将消息放入队列
                logger->info("Blocked, pushing to queue...");
                blockQueue.push(completeMessage);
            } else {
                // 创建新线程处理消息，而不是在接收线程中处理
                std::thread messageThread([completeMessage, clientSocket]() {
                    processMessageInThread(completeMessage, clientSocket);
                });
                messageThread.detach();  // 分离线程，让它自己运行和清理
                logger->info("Wait next message");
            }
            
        } catch (const std::exception &e) {
            logger->error("Error parsing JSON: {}", e.what());
        }
        
        // 从缓冲区中移除已处理的消息（包括长度前缀）
        buffer.erase(0, messageLength + 4);
    }
    
}

// Thread function to handle messages from clients
void handleClientMessages() {
    char buffer[8192];
    
    while (shouldRun) {
        std::vector<SOCKET> currentSockets;
        
        {
            // logger->info("Get clientSocketsMutex[loop] lock...");
            std::lock_guard<std::mutex> lock(clientSocketsMutex);
            // logger->info("Get clientSocketsMutex[loop] lock ok.");
            currentSockets = clientSockets;
        }
        
        for (auto it = currentSockets.begin(); it != currentSockets.end();) {
            SOCKET clientSocket = *it;
            
            // Check if socket has data to read
            // fd_set readSet;
            // FD_ZERO(&readSet);
            // FD_SET(clientSocket, &readSet);
            
            // struct timeval timeout;
            // timeout.tv_sec = 0;
            // timeout.tv_usec = 1000; // 1ms timeout
            
            // int selectResult = select(clientSocket + 1, &readSet, NULL, NULL, &timeout);
            
            // logger->info("Select result: {}", selectResult);
            // if (selectResult > 0) {
                memset(buffer, 0, sizeof(buffer));
                int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                
                if (bytesReceived > 0) {
                    // Process received message
                    buffer[bytesReceived] = '\0';
                    std::string message(buffer, bytesReceived);
                    logger->info("Received raw data from client: {} bytes", bytesReceived);
                    
                    // 使用分包处理逻辑
                    processBuffer(clientSocket, message);
                    
                    ++it;
                } else if (bytesReceived == 0) {
                    // Client disconnected
                    logger->info("Client disconnected");
                    
                    // 清理客户端缓冲区
                    {
                        // logger->info("Get clientBuffersMutex lock...");
                        std::lock_guard<std::mutex> lock(clientBuffersMutex);
                        // logger->info("Get clientBuffersMutex lock ok.");
                        clientBuffers.erase(clientSocket);
                    }
                    
                    // Handle disconnection as a message
                    std::string disconnectMsg = "{\"action\": \"disconnected\"}";
                    
                    auto callback = [disconnectMsg, clientSocket](Napi::Env env, Napi::Function jsCallback) {
                        auto ws = Napi::Object::New(env);
                        ws.Set("send", Napi::Function::New(env, sendMsg));
                        ws.Set("clientSocket", Napi::Number::New(env, static_cast<double>(clientSocket)));
                        
                        try {
                            jsCallback.Call({ws, Napi::String::New(env, disconnectMsg)});
                        } catch (const std::exception &e) {
                            logger->error("Error in callback: {}", e.what());
                        } catch (...) {
                            logger->error("Unknown error occurred in callback");
                        }
                    };
                    
                    websocketMessageHandleTsfn.BlockingCall(callback);
                    
                    // Remove client from list
                    {
                        // logger->info("Get clientSocketsMutex[disconnected] lock...");
                        std::lock_guard<std::mutex> lock(clientSocketsMutex);
                        // logger->info("Get clientSocketsMutex[disconnected] lock ok.");
                        auto sockIt = std::find(clientSockets.begin(), clientSockets.end(), clientSocket);
                        if (sockIt != clientSockets.end()) {
                            closeSocket(clientSocket);
                            clientSockets.erase(sockIt);
                        }
                    }
                    
                    it = currentSockets.erase(it);
                } else {
                    // Error occurred
                    logger->error("Error receiving data from client");
                    
                    // 清理客户端缓冲区
                    {
                      // logger->info("Get clientBuffersMutex[clean] lock...");
                        std::lock_guard<std::mutex> lock(clientBuffersMutex);
                        // logger->info("Get clientBuffersMutex[clean] lock ok.");
                        clientBuffers.erase(clientSocket);
                    }
                    
                    // Remove client from list
                    {
                      // logger->info("Get clientSocketsMutex[clean] lock...");
                        std::lock_guard<std::mutex> lock(clientSocketsMutex);
                        // logger->info("Get clientSocketsMutex[clean] lock ok.");
                        auto sockIt = std::find(clientSockets.begin(), clientSockets.end(), clientSocket);
                        if (sockIt != clientSockets.end()) {
                            closeSocket(clientSocket);
                            clientSockets.erase(sockIt);
                        }
                    }
                    
                    it = currentSockets.erase(it);
                }
            // } else {
            //     // No data or error
            //     ++it;
            // }
        }
        
        // Small sleep to avoid high CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int startInner(std::string &host, int port) {
    // Initialize socket system
    if (!initSocketSystem()) {
        logger->error("Failed to initialize socket system");
        return 1;
    }
    
    // Create server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        logger->error("Failed to create socket");
        cleanupSocketSystem();
        return 1;
    }
    
    // Set socket to reuse address
    int optval = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval));
    
    // Bind socket
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    // Convert IP address from string to network format
    if (host == "0.0.0.0" || host.empty()) {
        serverAddr.sin_addr.s_addr = INADDR_ANY;
    } else {
#ifdef _WIN32
        inet_pton(AF_INET, host.c_str(), &(serverAddr.sin_addr));
#else
        serverAddr.sin_addr.s_addr = inet_addr(host.c_str());
#endif
    }
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        logger->error("Failed to bind socket");
        closeSocket(serverSocket);
        cleanupSocketSystem();
        return 1;
    }
    
    // Listen for connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        logger->error("Failed to listen on socket");
        closeSocket(serverSocket);
        cleanupSocketSystem();
        return 1;
    }
    
    logger->info("Socket server listening on {}:{}", host, port);
    
    // Start threads for accepting connections and handling client messages
    shouldRun = true;
    acceptThread = std::thread(acceptClientConnections, host, port);
    receiveThread = std::thread(handleClientMessages);
    
    return 0;
}

Napi::Number start(const Napi::CallbackInfo &info) {
    if (info.Length() < 2) {
        throw Napi::TypeError::New(info.Env(), "start: Wrong number of arguments");
    }
    if (!info[0].IsString()) {
        throw Napi::TypeError::New(info.Env(), "First argument must be a string");
    }
    if (!info[1].IsNumber()) {
        throw Napi::TypeError::New(info.Env(), "Second argument must be a number");
    }
    
    auto host = info[0].As<Napi::String>().Utf8Value();
    auto port = info[1].As<Napi::Number>().Int32Value();
    
    communicationUuid.init(3, 1);
    
    return Napi::Number::New(info.Env(), startInner(host, port));
}

void stop(const Napi::CallbackInfo &info) {
    shouldRun = false;
    
    // Release the thread-safe function
    if (websocketMessageHandleTsfn) {
        websocketMessageHandleTsfn.Release();
    }
    
    // Close all client sockets
    {
        std::lock_guard<std::mutex> lock(clientSocketsMutex);
        for (auto clientSocket : clientSockets) {
            closeSocket(clientSocket);
        }
        clientSockets.clear();
    }
    
    // Close server socket
    closeSocket(serverSocket);
    
    // Join threads
    if (acceptThread.joinable()) {
        acceptThread.join();
    }
    
    if (receiveThread.joinable()) {
        receiveThread.join();
    }
    
    // Clean up socket system
    cleanupSocketSystem();
    
    logger->info("Socket server stopped");
}

void setMessageCallback(const Napi::CallbackInfo &info) {
    if (info.Length() < 1) {
        throw Napi::TypeError::New(info.Env(), "setMessageCallback: Wrong number of arguments");
    }
    if (!info[0].IsFunction()) {
        throw Napi::TypeError::New(info.Env(), "First argument must be a function");
    }

    // Create a ThreadSafeFunction
    websocketMessageHandleTsfn = Napi::ThreadSafeFunction::New(
        info.Env(),
        info[0].As<Napi::Function>(), // JavaScript function from caller
        "Socket Callback",            // Name
        0,                            // Unlimited queue
        1                             // Only one thread will use this initially
    );
    
    websocketMessageHandleRef = std::make_shared<Napi::FunctionReference>(
        Napi::Persistent(info[0].As<Napi::Function>())
    );

    logger->info("Set message callback");
}

/**
 * 给客户端发送消息
 */
Napi::Value sendMessageSync(const Napi::CallbackInfo &info) {
    if (info.Length() < 1) {
        throw Napi::TypeError::New(info.Env(), "sendMessageSync: Wrong number of arguments");
    }
    if (!info[0].IsString()) {
        throw Napi::TypeError::New(info.Env(), "First argument must be a string");
    }
    
    auto env = info.Env();
    isBlock = true;
    
    logger->info("sendMessageSync: {}", info[0].As<Napi::String>().Utf8Value());
    
    // Parse JSON string
    auto message = info[0].As<Napi::String>().Utf8Value();
    nlohmann::json json = nlohmann::json::parse(message);
    json["id"] = std::to_string(communicationUuid.nextid());
    
    // Check if any clients are connected
    {
      std::lock_guard<std::mutex> lock(clientSocketsMutex);
      
      if (clientSockets.empty()) {
          throw Napi::Error::New(info.Env(), "No clients connected");
      }
      
      // Send to first client (we could modify this to target specific clients if needed)
      logger->info("Sending to client: {}", json.dump());
      
      std::string jsonStr = json.dump();
      
      // Create length-prefixed message
      uint32_t messageLength = static_cast<uint32_t>(jsonStr.length());
      
      // Create length prefix (4 bytes, big-endian)
      char lengthPrefix[4];
      lengthPrefix[0] = (messageLength >> 24) & 0xFF;
      lengthPrefix[1] = (messageLength >> 16) & 0xFF;
      lengthPrefix[2] = (messageLength >> 8) & 0xFF;
      lengthPrefix[3] = messageLength & 0xFF;
      
      // Create the complete message with length prefix
      std::string completeMessage(lengthPrefix, 4);
      completeMessage.append(jsonStr);
      
      int result = send(clientSockets[0], completeMessage.c_str(), completeMessage.length(), 0);
      
      if (result == SOCKET_ERROR) {
          isBlock = false;
          throw Napi::Error::New(info.Env(), "Failed to send message to client");
      }
    }
      
    auto promiseObj = std::make_shared<std::promise<std::string>>();
    std::future<std::string> futureObj = promiseObj->get_future();
    wsRequest.emplace(json["id"], promiseObj);
    
    // 3-second timeout
    auto start = std::chrono::high_resolution_clock::now();
    logger->info("Waiting for response and check blockQueue...");
    while (true) {
        if (!blockQueue.empty()) {
            auto msg = blockQueue.front();
            blockQueue.pop();
            
            try {
                // Process queued message
                auto ws = Napi::Object::New(env);
                ws.Set("send", Napi::Function::New(env, sendMsg));
                websocketMessageHandleRef->Value().Call({ws, Napi::String::New(env, msg)});
            } catch (const std::exception &e) {
                logger->error("Error processing queued message: {}", e.what());
                throw Napi::Error::New(info.Env(), e.what());
            } catch (...) {
                logger->error("Unknown error occurred processing queued message");
                throw Napi::Error::New(info.Env(), "Unknown error occurred");
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        if (elapsed.count() > 3) {
            logger->error("Request to client timeout, request data:\n" + json.dump());
            wsRequest.erase(json["id"]);
            isBlock = false;
            throw Napi::Error::New(info.Env(), "Request to client timeout, request data:\n" + json.dump());
        }
        
        if (futureObj.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            logger->info("Future object is ready");
            break;
        }
        
        // Small sleep to avoid high CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    logger->info("Check blockQueue end.");
    isBlock = false;
    std::string result_str = futureObj.get();
    auto resp = nlohmann::json::parse(result_str);
    auto v = Convert::convertJson2Value(env, resp["result"]);
    
    return v;
}

} // namespace WebSocketServer
