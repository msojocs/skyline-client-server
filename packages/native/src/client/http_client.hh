#pragma once

#include <napi.h>
#include <string>

class HttpClient {
public:

    /**
     * 发送HTTP请求到指定端点
     * @param endpoint 端点路径 (例如: "/destroy", "/start")
     * @param operation_name 操作名称用于日志
     * @return 响应内容
     * @throws std::runtime_error 如果请求失败
     */
    static std::string sendHttpRequest(const std::string& endpoint, const std::string& operation_name);
    
private:
    /**
     * 解析HTTP响应获取内容
     * @param response_str 完整的HTTP响应字符串
     * @return 解析后的内容
     * @throws std::runtime_error 如果解析失败
     */
    static std::string parseHttpResponse(const std::string& response_str);
    
    /**
     * 获取系统错误信息
     * @param error_code 错误代码
     * @return 错误信息字符串
     */
    static std::string getSystemErrorMessage(int error_code);
};
