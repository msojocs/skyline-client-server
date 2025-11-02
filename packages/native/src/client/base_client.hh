#ifndef __BASE_CLIENT_HH__
#define __BASE_CLIENT_HH__
#include <napi.h>

namespace Skyline {

class BaseClient {
public:
  Napi::Value getInstanceId(const Napi::CallbackInfo &info);
protected:
  int64_t m_instanceId;
  /**
   * 与服务器通信
   * 
   * 同步方法
   */
  Napi::Value sendToServerSync(const Napi::CallbackInfo &info, const std::string &methodName);
  /**
   * 与服务器通信
   * 
   * 异步方法,不关心回复
   */
  Napi::Value sendToServerAsync(const Napi::CallbackInfo &info, const std::string &methodName);
  /**
   * 与服务器通信
   * 
   * 同步方法,用于构造函数
   */
  int64_t sendConstructorToServerSync(const Napi::CallbackInfo &info, const std::string &className);

  /**
   * 获取property
   */
  void setProperty(const Napi::CallbackInfo &info, const std::string &propertyName);
  /**
   * 设置property
   */
  Napi::Value getProperty(const Napi::CallbackInfo &info, const std::string &propertyName);
};

} // namespace Skyline

#endif