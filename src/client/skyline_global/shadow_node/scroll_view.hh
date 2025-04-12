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
  Napi::Value setRefresherHeader(const Napi::CallbackInfo &info);
};

} // namespace Skyline

#endif