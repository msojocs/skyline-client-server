#include "client_memory.hh"
#include "../common/logger.hh"

namespace SkylineClient {

using Logger::logger;
void ClientMemory::Init(Napi::Env env) {
    logger->info("Shared memory for server to client initialized");
    msgToServer = std::make_shared<SkylineMemory::SharedMemoryCommunication>(std::string("skyline_client2server"), true);
    msgFromServer = std::make_shared<SkylineMemory::SharedMemoryCommunication>(std::string("skyline_server2client"), false);
    #ifdef _WIN32
    msgToServer->file_notify = CreateSemaphoreA(nullptr, 0, 1, "Global\\skyline_client2server_notify");
    #elif __linux__
    msgToServer->file_notify = sem_open("skyline_client2server_notify", O_CREAT, 0644, 0);
    #endif
    logger->info("Shared memory for client to server initialized");
}
void ClientMemory::sendMessage(const std::string &message) {
    msgToServer->sendMessage(message);
}
std::string ClientMemory::receiveMessage() {
    
    #ifdef _WIN32
    std::string name="Global\\skyline_server2client_notify";
    #else
    std::string name="skyline_server2client_notify";
    #endif
        
    return msgFromServer->receiveMessage(name);
}

ClientMemory::~ClientMemory() {}
}