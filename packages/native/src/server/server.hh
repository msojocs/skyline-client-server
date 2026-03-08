#ifndef __SERVER_HH__
#define __SERVER_HH__
#include <cstdint>
#include <napi.h>

namespace SkylineServer {
    class Server {
    public:
        virtual void Init(const Napi::CallbackInfo &info) = 0;
        virtual void sendMessage(std::string&& message, std::int64_t messageId = 0) = 0;
        virtual std::string receiveMessage(std::int64_t *messageId = nullptr) = 0;
    };
}
#endif // __SERVER_HH__
