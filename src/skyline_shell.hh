#ifndef __SKYLINE_SHELL_HH__
#define __SKYLINE_SHELL_HH__
#include "napi.h"
namespace SkylineShell {

class SkylineShell : public Napi::ObjectWrap<SkylineShell> {
public:
  static void Init(Napi::Env env, Napi::Object exports);

  SkylineShell(const Napi::CallbackInfo &info);

private:
  void setNotifyBootstrapDoneCallback(const Napi::CallbackInfo &info);
  void setNotifyWindowReadyCallback(const Napi::CallbackInfo &info);
  void setNotifyRouteDoneCallback(const Napi::CallbackInfo &info);
  void setNavigateBackCallback(const Napi::CallbackInfo &info);
  void setNavigateBackDoneCallback(const Napi::CallbackInfo &info);
  void setLoadResourceCallback(const Napi::CallbackInfo &info);
  void setLoadResourceAsyncCallback(const Napi::CallbackInfo &info);
  void setHttpRequestCallback(const Napi::CallbackInfo &info);
  void setSendLogCallback(const Napi::CallbackInfo &info);
  void setSafeAreaEdgeInsets(const Napi::CallbackInfo &info);
  void createWindow(const Napi::CallbackInfo &info);
  void destroyWindow(const Napi::CallbackInfo &info);
};
} // namespace SkylineShell
#endif