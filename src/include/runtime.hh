#ifndef __RUNTIME_HH__
#define __RUNTIME_HH__
#include <napi.h>


namespace Skyline {
  namespace Runtime {
    void Init(Napi::Env env, Napi::Object exports);
  }
}
#endif // __RUNTIME_HH__