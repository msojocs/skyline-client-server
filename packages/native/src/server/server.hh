#ifndef __SERVER_HH__
#define __SERVER_HH__
#include <napi.h>

namespace SkylineServer {
    class Server {
    public:
        virtual void Init(const Napi::CallbackInfo &info) = 0;
        virtual void sendMessage(const std::string &message) = 0;
        virtual std::string receiveMessage() = 0;
    };
}
#endif // __SERVER_HH__