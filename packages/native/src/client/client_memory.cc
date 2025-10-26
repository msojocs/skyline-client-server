#include "client_memory.hh"
#include "../common/logger.hh"

namespace SkylineClient {

using Logger::logger;
void ClientMemory::Init(Napi::Env env) {
    logger->info("Shared memory for server to client initialized");
    msgToServer = std::make_shared<SkylineMemory::SharedMemoryCommunication>(std::string("skyline_client2server"), false);
    msgFromServer = std::make_shared<SkylineMemory::SharedMemoryCommunication>(std::string("skyline_server2client"), false);
    logger->info("Shared memory for client to server initialized");
}
void ClientMemory::sendMessage(const std::string &message) {
    msgToServer->sendMessage(message);
}
std::string ClientMemory::receiveMessage() {
    auto msg = msgFromServer->receiveMessage();
    if (msg.empty()) {
        return "";
    }
    return msg;
}

ClientMemory::~ClientMemory() {}
}