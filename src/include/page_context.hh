#ifndef __PAGE_CONTEXT_HH__
#define __PAGE_CONTEXT_HH__

#include "base_client.hh"
#include "napi.h"
namespace Skyline {
  class PageContext: public Napi::ObjectWrap<PageContext>, public BaseClient {
  public:
    PageContext(const Napi::CallbackInfo &info);
    static void Init(Napi::Env env, Napi::Object exports);
  private:
  void appendCompiledStyleSheets(const Napi::CallbackInfo &info);
  void appendStyleSheet(const Napi::CallbackInfo &info);
  /**
   * 两个参数
   * * @param {string} path "wxlibfile://WAStyleIndex.fpiib"
   * * @param {number} id 0
   * * @return {number} undefined
   */
  void appendStyleSheetIndex(const Napi::CallbackInfo &info);
  void appendStyleSheets(const Napi::CallbackInfo &info);
  void attach(const Napi::CallbackInfo &info);
  void attachCustomRoute(const Napi::CallbackInfo &info);
  void clearStylesheets(const Napi::CallbackInfo &info);
  void createElement(const Napi::CallbackInfo &info);
  void createFragment(const Napi::CallbackInfo &info);
  /**
   * 0个参数
   * * @return {number} number
   */
  Napi::Value createStyleSheetIndexGroup(const Napi::CallbackInfo &info);
  void createTextNode(const Napi::CallbackInfo &info);
  void finishStyleSheetsCompilation(const Napi::CallbackInfo &info);
  void getComputedStyle(const Napi::CallbackInfo &info);
  void getHostNode(const Napi::CallbackInfo &info);
  void getNodeFromPoint(const Napi::CallbackInfo &info);
  void getRootNode(const Napi::CallbackInfo &info);
  void getWindowId(const Napi::CallbackInfo &info);
  void isTab(const Napi::CallbackInfo &info);
  void loadFontFace(const Napi::CallbackInfo &info);
  void preCompileStyleSheets(const Napi::CallbackInfo &info);
  void recalcStyle(const Napi::CallbackInfo &info);
  void release(const Napi::CallbackInfo &info);
  void setAsTab(const Napi::CallbackInfo &info);
  void setNavigateBackInterception(const Napi::CallbackInfo &info);
  void startRender(const Napi::CallbackInfo &info);
  void updateRouteConfig(const Napi::CallbackInfo &info);
  };
}
#endif