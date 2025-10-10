#ifndef CLIENT_MEMORY_HH
#define CLIENT_MEMORY_HH
#include "client.hh"
#include "../memory/skyline_memory.hh"

class ClientMemory : public Client {
    public:
    void Init(Napi::Env env);
    ~ClientMemory();
    void sendMessage(const std::string &message);
    std::string receiveMessage(const std::string &name);

    private:
    std::shared_ptr<SkylineMemory::SharedMemoryCommunication> msgFromServer;
    std::shared_ptr<SkylineMemory::SharedMemoryCommunication> msgToServer;
};
#endif // CLIENT_MEMORY_HH