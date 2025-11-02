#include "server_memory.hh"
#include "../common/logger.hh"

using Logger::logger;

namespace SkylineServer {
    void ServerMemory::Init(const Napi::CallbackInfo &info) {
        try {
            msgToClient = std::make_shared<SkylineMemory::SharedMemoryCommunication>("skyline_server2client", true);
            msgFromClient = std::make_shared<SkylineMemory::SharedMemoryCommunication>("skyline_client2server", true);
            semClientToServer = CreateSemaphore(NULL, 0, LONG_MAX, "skyline_sem_client2server");
            semServerToClient = CreateSemaphore(NULL, 0, LONG_MAX, "skyline_sem_server2client");
            logger->info("Shared memory server initialized");
        } catch (const std::exception &e) {
            logger->error("Failed to initialize shared memory server: {}", e.what());
        }
    }
    ServerMemory::~ServerMemory() {
        try {
            msgFromClient.reset();
            msgToClient.reset();
            if (semClientToServer) CloseHandle(semClientToServer);
            if (semServerToClient) CloseHandle(semServerToClient);
            logger->info("Shared memory server stopped");
        } catch (const std::exception &e) {
            logger->error("Error stopping shared memory server: {}", e.what());
        }
    }
    void ServerMemory::sendMessage(const std::string &message) {
        try {
            if (msgToClient) {
                msgToClient->sendMessage(message);
                ReleaseSemaphore(semServerToClient, 1, NULL);
                logger->info("Sent message of length {}", message.size());
            } else {
                logger->error("Shared memory for sending messages is not initialized.");
            }
        } catch (const std::exception &e) {
            logger->error("Error sending message: {}", e.what());
        }
    }
    std::string ServerMemory::receiveMessage() {
        try {
            if (msgFromClient) {
                WaitForSingleObject(semClientToServer, INFINITE);
                auto msg = msgFromClient->receiveMessage();
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