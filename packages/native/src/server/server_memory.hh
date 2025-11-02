#ifndef __SERVER_MEMORY_HH__
#define __SERVER_MEMORY_HH__
#include "server.hh"
#include "../memory/skyline_memory.hh"
#include <windows.h>

namespace SkylineServer {
    class ServerMemory : public Server {
    public:
        void Init(const Napi::CallbackInfo &info);
        ~ServerMemory();
        void sendMessage(const std::string &message);
        std::string receiveMessage();
    private:
        std::shared_ptr<SkylineMemory::SharedMemoryCommunication> msgFromClient;
        std::shared_ptr<SkylineMemory::SharedMemoryCommunication> msgToClient;
        HANDLE semClientToServer;
        HANDLE semServerToClient;
    };
}
#endif // __SERVER_MEMORY_HH__