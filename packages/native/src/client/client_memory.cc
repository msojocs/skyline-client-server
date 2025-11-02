#include "client_memory.hh"
#include "../common/logger.hh"
#include <winbase.h>

namespace SkylineClient {

using Logger::logger;
void ClientMemory::Init(Napi::Env env) {
    logger->info("Shared memory for server to client initialized");
    msgToServer = std::make_shared<SkylineMemory::SharedMemoryCommunication>(std::string("skyline_client2server"), false);
    msgFromServer = std::make_shared<SkylineMemory::SharedMemoryCommunication>(std::string("skyline_server2client"), false);
    semClientToServer = CreateSemaphore(NULL, 0, LONG_MAX, "skyline_sem_client2server");
    semServerToClient = CreateSemaphore(NULL, 0, LONG_MAX, "skyline_sem_server2client");
    logger->info("Shared memory for client to server initialized");
}
void ClientMemory::sendMessage(const std::string &message) {
    msgToServer->sendMessage(message);
    ReleaseSemaphore(semClientToServer, 1, NULL);
}
std::string ClientMemory::receiveMessage() {
    WaitForSingleObject(semServerToClient, INFINITE);
    auto msg = msgFromServer->receiveMessage();
    return msg;
}

ClientMemory::~ClientMemory() {
    if (semClientToServer) CloseHandle(semClientToServer);
    if (semServerToClient) CloseHandle(semServerToClient);
}
}