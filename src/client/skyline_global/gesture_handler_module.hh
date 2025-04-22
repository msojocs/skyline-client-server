#ifndef __GESTURE_HANDLER_MODULE_HH__
#define __GESTURE_HANDLER_MODULE_HH__
#include <napi.h>

namespace Skyline {
namespace GestureHandlerModule {
  void Init(Napi::Env env, Napi::Object exports);
}

}
#endif // __GESTURE_HANDLER_MODULE_HH__