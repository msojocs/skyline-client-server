#include "node.hh"
#include "napi.h"
#include <vector>
#include <string_view>

namespace HTML {
ShadowNode::ShadowNode(const Napi::CallbackInfo &info) {
  if (info.Length() < 1) {
    throw Napi::TypeError::New(info.Env(),
                               "Constructor: Wrong number of arguments");
  }

  m_instanceId = info[0].As<Napi::Number>().Int64Value();
}

Napi::Value ShadowNode::reload(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}
// isConnected
Napi::Value ShadowNode::isConnected(const Napi::CallbackInfo &info) {
  return getProperty(info, __func__);
}
Napi::Value ShadowNode::setAttribute(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

Napi::Value ShadowNode::removeAttribute(const Napi::CallbackInfo &info) {
  return sendToServerSync(info, __func__);
}

} // namespace HTML