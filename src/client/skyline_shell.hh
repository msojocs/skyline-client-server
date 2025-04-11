#ifndef __SKYLINE_SHELL_HH__
#define __SKYLINE_SHELL_HH__
#include <nlohmann/json.hpp>
#include "napi.h"
#include "include/base_client.hh"
namespace SkylineShell {

class SkylineShell : public Napi::ObjectWrap<SkylineShell>, public Skyline::BaseClient {
public:
  static void Init(Napi::Env env, Napi::Object exports);
  static void DispatchCallback(std::string &action, nlohmann::json &data);

  SkylineShell(const Napi::CallbackInfo &info);
  Napi::Value getInstanceId(const Napi::CallbackInfo &info);

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
  void notifyAppLaunch(const Napi::CallbackInfo &info);
  /**
   * @brief 通知平台亮度变化
   */
  void onPlatformBrightnessChanged(const Napi::CallbackInfo &info);
  void dispatchTouchStartEvent(const Napi::CallbackInfo &info);
  void dispatchTouchEndEvent(const Napi::CallbackInfo &info);
  void dispatchTouchMoveEvent(const Napi::CallbackInfo &info);
  void dispatchTouchCancelEvent(const Napi::CallbackInfo &info);
  void dispatchKeyboardEvent(const Napi::CallbackInfo &info);
};
} // namespace SkylineShell
#endif