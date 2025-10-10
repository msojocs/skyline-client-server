#ifndef __SERVER_HH__
#define __SERVER_HH__
#include <napi.h>

namespace SkylineServer {
    class Server {
    public:
        virtual void Init(Napi::Env env) = 0;
        virtual void sendMessage(const std::string &message) = 0;
        virtual std::string receiveMessage(const std::string &name) = 0;
    };
}
#endif // __SERVER_HH__