#ifndef CLIENT_MEMORY_HH
#define CLIENT_MEMORY_HH
#include "client.hh"
#include "../memory/skyline_memory.hh"
#include <windows.h>
namespace SkylineClient {
class ClientMemory : public Client {
    public:
    void Init(Napi::Env env);
    ~ClientMemory();
    void sendMessage(const std::string &message);
    std::string receiveMessage();

    private:
    std::shared_ptr<SkylineMemory::SharedMemoryCommunication> msgFromServer;
    std::shared_ptr<SkylineMemory::SharedMemoryCommunication> msgToServer;
    HANDLE semClientToServer;
    HANDLE semServerToClient;
};
}
#endif // CLIENT_MEMORY_HH