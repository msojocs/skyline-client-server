#ifndef __PAGE_CONTEXT_HH__
#define __PAGE_CONTEXT_HH__

#include "base_client.hh"
#include "napi.h"
namespace Skyline {
class PageContext : public Napi::ObjectWrap<PageContext>, public BaseClient {
public:
  PageContext(const Napi::CallbackInfo &info);
  static void Init(Napi::Env env, Napi::Object exports);

private:
  Napi::Value getFrameworkType(const Napi::CallbackInfo &info);
  void setFrameworkType(const Napi::CallbackInfo &info, const Napi::Value &value);
  Napi::Value appendCompiledStyleSheets(const Napi::CallbackInfo &info);
  Napi::Value appendStyleSheet(const Napi::CallbackInfo &info);
  /**
   * 两个参数
   * * @param {string} path "wxlibfile://WAStyleIndex.fpiib"
   * * @param {number} id 0
   * * @return {number} undefined
   */
  Napi::Value appendStyleSheetIndex(const Napi::CallbackInfo &info);
  Napi::Value appendStyleSheets(const Napi::CallbackInfo &info);
  Napi::Value attach(const Napi::CallbackInfo &info);
  Napi::Value attachCustomRoute(const Napi::CallbackInfo &info);
  Napi::Value clearStylesheets(const Napi::CallbackInfo &info);
  Napi::Value createElement(const Napi::CallbackInfo &info);
  Napi::Value createFragment(const Napi::CallbackInfo &info);
  /**
   * 0个参数
   * * @return {number} number
   */
  Napi::Value createStyleSheetIndexGroup(const Napi::CallbackInfo &info);
  Napi::Value createTextNode(const Napi::CallbackInfo &info);
  Napi::Value finishStyleSheetsCompilation(const Napi::CallbackInfo &info);
  Napi::Value getComputedStyle(const Napi::CallbackInfo &info);
  Napi::Value getHostNode(const Napi::CallbackInfo &info);
  Napi::Value getNodeFromPoint(const Napi::CallbackInfo &info);
  Napi::Value getRootNode(const Napi::CallbackInfo &info);
  Napi::Value getWindowId(const Napi::CallbackInfo &info);
  Napi::Value isTab(const Napi::CallbackInfo &info);
  Napi::Value loadFontFace(const Napi::CallbackInfo &info);
  Napi::Value preCompileStyleSheets(const Napi::CallbackInfo &info);
  Napi::Value recalcStyle(const Napi::CallbackInfo &info);
  Napi::Value release(const Napi::CallbackInfo &info);
  Napi::Value setAsTab(const Napi::CallbackInfo &info);
  Napi::Value setNavigateBackInterception(const Napi::CallbackInfo &info);
  Napi::Value startRender(const Napi::CallbackInfo &info);
  Napi::Value updateRouteConfig(const Napi::CallbackInfo &info);
};
} // namespace Skyline
#endif