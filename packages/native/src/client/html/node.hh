#ifndef __HTML_NODE__HH__
#define __HTML_NODE__HH__
#include "../base_client.hh"
#include <napi.h>
#include <unordered_set>
#include <vector>

namespace HTML {
// NOTE: 修改此处配置需要手动清除cmake的编译缓存
template<typename T>
std::vector<Napi::ClassPropertyDescriptor<T>> GetCommonMethods() {
  std::vector<Napi::ClassPropertyDescriptor<T>> methods;
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("reload", &T::reload));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("setAttribute", &T::setAttribute, static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  methods.push_back(Napi::InstanceWrap<T>::InstanceMethod("removeAttribute", &T::removeAttribute));


  // Add instance accessors
  methods.push_back(Napi::InstanceWrap<T>::InstanceAccessor(
    "isConnected", &T::isConnected, nullptr,
    static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  methods.push_back(Napi::InstanceWrap<T>::InstanceAccessor(
    "instanceId", &T::getInstanceId, nullptr,
    static_cast<napi_property_attributes>(napi_writable | napi_configurable)));
  return methods;
}

class ShadowNode : public BaseClient {
public:
  ShadowNode(const Napi::CallbackInfo &info);
  static Napi::FunctionReference *GetClazz(Napi::Env env);

  Napi::Value reload(const Napi::CallbackInfo &info);
  Napi::Value removeAttribute(const Napi::CallbackInfo &info);
  Napi::Value setAttribute(const Napi::CallbackInfo &info);
  Napi::Value isConnected(const Napi::CallbackInfo &info);

  private:
  int styleScope;
  std::string id;
  std::unordered_set<std::string> classList;
};
} // namespace HTML
#endif // __SHADOW_NODE__HH__