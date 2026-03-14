#ifndef __EVENT__HH__
#define __EVENT__HH__
#include "node.hh"
#include <napi.h>

namespace HTML {
class Event : public Napi::ObjectWrap<Event>,
                       public ShadowNode {
public:
  Event(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);

private:
  Napi::Value getDialog(const Napi::CallbackInfo &info);
  Napi::Value getMessageText(const Napi::CallbackInfo &info);
  Napi::Value getMessageType(const Napi::CallbackInfo &info);
  Napi::Value getType(const Napi::CallbackInfo &info);
  Napi::Value preventDefault(const Napi::CallbackInfo &info);
  void setReturnValue(const Napi::CallbackInfo &info, const Napi::Value &value);
};
} // namespace HTML
#endif // __EVENT__HH__
