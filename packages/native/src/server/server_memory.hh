#ifndef __SERVER_MEMORY_HH__
#define __SERVER_MEMORY_HH__
#include "server.hh"
#include "../memory/skyline_memory.hh"

namespace SkylineServer {
    class ServerMemory : public Server {
    public:
        void Init(Napi::Env env);
        ~ServerMemory();
        void sendMessage(const std::string &message);
        std::string receiveMessage();
    private:
        std::shared_ptr<SkylineMemory::SharedMemoryCommunication> msgFromClient;
        std::shared_ptr<SkylineMemory::SharedMemoryCommunication> msgToClient;
    };
}
#endif // __SERVER_MEMORY_HH__