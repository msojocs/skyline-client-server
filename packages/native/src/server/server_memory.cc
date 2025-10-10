#include "server_memory.hh"
#include "../common/logger.hh"

using Logger::logger;

namespace SkylineServer {
    void ServerMemory::Init(Napi::Env env) {
        try {
            msgToClient = std::make_shared<SkylineMemory::SharedMemoryCommunication>("skyline_server2client", false);
            #ifdef _WIN32
            msgToClient->file_notify = CreateSemaphoreA(nullptr, 0, 1, "Global\\skyline_server2client_notify");
            #elif __linux__
            msgToClient->file_notify = sem_open("skyline_server2client_notify", O_CREAT, 0644, 0);
            #endif
            msgFromClient = std::make_shared<SkylineMemory::SharedMemoryCommunication>("skyline_client2server", false);
            logger->info("Shared memory server initialized");
        } catch (const std::exception &e) {
            logger->error("Failed to initialize shared memory server: {}", e.what());
        }
    }
    ServerMemory::~ServerMemory() {
        try {
            msgFromClient.reset();
            msgToClient.reset();
            logger->info("Shared memory server stopped");
        } catch (const std::exception &e) {
            logger->error("Error stopping shared memory server: {}", e.what());
        }
    }
    void ServerMemory::sendMessage(const std::string &message) {
        try {
            if (msgToClient) {
                msgToClient->sendMessage(message);
                logger->info("Sent message of length {}", message.size());
            } else {
                logger->error("Shared memory for sending messages is not initialized.");
            }
        } catch (const std::exception &e) {
            logger->error("Error sending message: {}", e.what());
        }
    }
    std::string ServerMemory::receiveMessage(const std::string &name) {
        try {
            if (msgFromClient) {
                auto msg = msgFromClient->receiveMessage(
                        #ifdef _WIN32
                        "Global\\skyline_client2server_notify"
                        #elif __linux__
                        "skyline_client2server_notify"
                        #endif
                    );
                logger->info("Received message of length {}", msg.size());
                return msg;
            } else {
                logger->error("Shared memory for receiving messages is not initialized.");
                return "";
            }
        } catch (const std::exception &e) {
            logger->error("Error receiving message: {}", e.what());
            return "";
        }
    }
}