#ifndef __ASYNC_STYLE_SHEETS_HH__
#define __ASYNC_STYLE_SHEETS_HH__
#include <napi.h>
#include "base_client.hh"

namespace Skyline {
  class AsyncStyleSheets: public Napi::ObjectWrap<AsyncStyleSheets>, public BaseClient {
  public:
    AsyncStyleSheets(const Napi::CallbackInfo& info);
    static Napi::FunctionReference* GetClazz(Napi::Env env);
  private:
    void setScopeId(const Napi::CallbackInfo& info);
  };
}
#endif // __ASYNC_STYLE_SHEETS_HH__