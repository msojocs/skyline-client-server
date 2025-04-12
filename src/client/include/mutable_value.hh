#ifndef __MUTABLE_HH__
#define __MUTABLE_HH__

#include "base_client.hh"
#include "napi.h"

namespace Skyline {
  class MutableValue: public Napi::ObjectWrap<MutableValue>, public BaseClient {
  public:
  MutableValue(const Napi::CallbackInfo& info);
    static Napi::FunctionReference * GetClazz(Napi::Env env);
    Napi::Value getInstanceId(const Napi::CallbackInfo& info) override;
    private:
    /**
     * value: (...)
_animation: (...)
_value: (...)
_windowId: (...)
get value: ƒ ()
set value: ƒ ()
get _animation: ƒ ()
set _animation: ƒ ()
get _value: ƒ ()
set _value: ƒ ()
get _windowId: ƒ ()
set _windowId: ƒ ()
installGetter: ƒ installGetter()
     */
    Napi::Value getValue(const Napi::CallbackInfo& info);
    void setValue(const Napi::CallbackInfo& info, const Napi::Value &value);
    Napi::Value getAnimation(const Napi::CallbackInfo& info);
    void setAnimation(const Napi::CallbackInfo& info);
    Napi::Value getWindowId(const Napi::CallbackInfo& info);
    void setWindowId(const Napi::CallbackInfo& info);
    Napi::Value installGetter(const Napi::CallbackInfo& info);
  };
}
#endif // __MUTABLE_HH__