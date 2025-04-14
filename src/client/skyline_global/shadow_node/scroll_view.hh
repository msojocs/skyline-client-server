#ifndef __SCROLL_VIEW_SHADOW_NODE_HH__
#define __SCROLL_VIEW_SHADOW_NODE_HH__

#include "napi.h"
#include "node.hh"

namespace Skyline {

class ScrollViewShadowNode : public Napi::ObjectWrap<ScrollViewShadowNode>,
                             public ShadowNode {
public:
  static Napi::FunctionReference *GetClazz(Napi::Env env);
  ScrollViewShadowNode(const Napi::CallbackInfo &info);

private:
  /**
   * 相比view多出来的
   * getScrollPosition: ƒ getScrollPosition()
   * holdPosition: ƒ holdPosition()
   * onRefresherAbortEvent: ƒ onRefresherAbortEvent()
    onRefresherPullingEvent: ƒ onRefresherPullingEvent()
    onRefresherRefreshEvent: ƒ onRefresherRefreshEvent()
    onRefresherRestoreEvent: ƒ onRefresherRestoreEvent()
    onRefresherStatusChangeEvent: ƒ onRefresherStatusChangeEvent()
    onRefresherWillRefreshEvent: ƒ onRefresherWillRefreshEvent()
    onScrollEndEvent: ƒ onScrollEndEvent()
    onScrollEvent: ƒ onScrollEvent()
    onScrollStartEvent: ƒ onScrollStartEvent()
    onScrollToLowerEvent: ƒ onScrollToLowerEvent()
    onScrollToUpperEvent: ƒ onScrollToUpperEvent()
    requestRefresherRefresh: ƒ requestRefresherRefresh()
    requestRefresherTwoLevel: ƒ requestRefresherTwoLevel()
    scrollIntoView: ƒ scrollIntoView()
    scrollTo: ƒ scrollTo()
    setRefresherHeader: ƒ setRefresherHeader()
   */
  // getScrollPosition
  Napi::Value getScrollPosition(const Napi::CallbackInfo &info);
  // holdPosition
  Napi::Value holdPosition(const Napi::CallbackInfo &info);
  // onRefresherAbortEvent
  Napi::Value onRefresherAbortEvent(const Napi::CallbackInfo &info);
  // onRefresherPullingEvent
  Napi::Value onRefresherPullingEvent(const Napi::CallbackInfo &info);
  // onRefresherRefreshEvent
  Napi::Value onRefresherRefreshEvent(const Napi::CallbackInfo &info);
  // onRefresherRestoreEvent
  Napi::Value onRefresherRestoreEvent(const Napi::CallbackInfo &info);
  // onRefresherStatusChangeEvent
  Napi::Value onRefresherStatusChangeEvent(const Napi::CallbackInfo &info);
  // onRefresherWillRefreshEvent
  Napi::Value onRefresherWillRefreshEvent(const Napi::CallbackInfo &info);
  Napi::Value onScrollEndEvent(const Napi::CallbackInfo &info);
  Napi::Value onScrollEvent(const Napi::CallbackInfo &info);
  Napi::Value onScrollStartEvent(const Napi::CallbackInfo &info);
  Napi::Value onScrollToLowerEvent(const Napi::CallbackInfo &info);
  Napi::Value onScrollToUpperEvent(const Napi::CallbackInfo &info);
  Napi::Value requestRefresherRefresh(const Napi::CallbackInfo &info);
  Napi::Value requestRefresherTwoLevel(const Napi::CallbackInfo &info);
  Napi::Value scrollIntoView(const Napi::CallbackInfo &info);
  Napi::Value scrollTo(const Napi::CallbackInfo &info);
  Napi::Value setRefresherHeader(const Napi::CallbackInfo &info);
};

} // namespace Skyline

#endif