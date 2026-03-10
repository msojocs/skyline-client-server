#ifndef CLIENT_MEMORY_HH
#define CLIENT_MEMORY_HH
#include "client.hh"
#include "../memory/skyline_memory.hh"
#include <boost/asio.hpp>
#include <memory>

namespace SkylineClient {
class ClientMemory : public Client {
    public:
    void Init(Napi::Env env);
    ~ClientMemory();
    void sendMessage(std::string &&message, std::int64_t messageId = 0);
    std::string receiveMessage(std::int64_t *messageId = nullptr);

    private:
    std::shared_ptr<SkylineMemory::SharedMemoryCommunication> msgFromServer;
    std::shared_ptr<SkylineMemory::SharedMemoryCommunication> msgToServer;
    boost::asio::io_context ioContext;
    std::unique_ptr<boost::asio::ip::tcp::socket> signalSocket;
};
}
#endif // CLIENT_MEMORY_HH
