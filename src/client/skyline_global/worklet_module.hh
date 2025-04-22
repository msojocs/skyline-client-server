#ifndef __WORKLET_MODULE_HH__
#define __WORKLET_MODULE_HH__
#include <napi.h>

namespace Skyline {
namespace WorkletModule {
  void Init(Napi::Env env, Napi::Object exports);
}

}
#endif // __WORKLET_MODULE_HH__