/**
 * @file util.h
 * @brief harborRPC 工具类定义文件
 * 
 * 本文件提供RPC框架的辅助工具类，包括：
 * - singleton_server: RPC服务单件包装器
 */

#ifndef __HARBOR_RPC_UTIL_H__
#define __HARBOR_RPC_UTIL_H__

#include <string>
#include <stdexcept>
#include "harbor_rpc/server.h"
#include "harbor_rpc/harbor.h"

namespace harbor_rpc {

/**
 * @class singleton_server
 * @brief RPC服务单件包装器
 * 
 * 将任意RPC服务类包装为单件模式，并在首次获取实例时自动注册到harbor。
 * 通过public继承方式，保留原服务的所有接口。
 * 
 * @tparam Server RPC服务类类型（需继承自server_base或rpc_server）
 * 
 * @note 服务名称通过regist_as_service()方法传入，首次调用instance()时注册到harbor。
 *       原服务的init()方法仍可正常调用。
 * 
 * @example
 * @code
 * // 定义普通RPC服务类
 * class MyRPC : public harbor_rpc::core::rpc_server<MyRPC> { ... };
 * 
 * // 使用单件包装器创建单件版本
 * using MyService = harbor_rpc::singleton_server<MyRPC>;
 * 
 * // 注册为服务并获取单件
 * MyService::regist_as_service("my_service");
 * MyService::instance().run();
 * 
 * // 或者一步完成注册和启动
 * MyService::regist_as_service("my_service").run();
 * 
 * // 调用原服务的init()（如果需要）
 * MyService::instance().init();
 * 
 * // 调用RPC
 * int ret = harbor_rpc::client::call("my_service", "cmd1", args...);
 * 
 * // 停止服务
 * MyService::instance().stop();
 * @endcode
 */
template<typename Server>
class singleton_server : public Server {
public:
    /**
     * @brief 注册为服务并获取单件实例
     * @param service_name 服务名称
     * @return singleton_server& 单件引用
     */
    static singleton_server& regist_as_service(const std::string& service_name) {
        static singleton_server* inst = nullptr;
        if (inst == nullptr) {
            inst = new singleton_server();
            _service_name = service_name;
            core::harbor::instance().regist_module(_service_name, inst);
        }
        return *inst;
    }
    
    /**
     * @brief 获取单件实例（需先调用regist_as_service）
     * @return singleton_server& 单件引用
     * @throw std::runtime_error 如果未初始化
     */
    static singleton_server& instance() {
        static singleton_server* inst = nullptr;
        if (inst == nullptr) {
            throw std::runtime_error("singleton_server not initialized, call regist_as_service() first");
        }
        return *inst;
    }
    
    /**
     * @brief 获取服务名称
     * @return const std::string& 服务名称
     */
    static const std::string& service_name() { return _service_name; }

private:
    /** @brief 私有构造函数 */
    singleton_server() : Server() {}
    
    /** @brief 禁止拷贝 */
    singleton_server(const singleton_server&) = delete;
    singleton_server& operator=(const singleton_server&) = delete;
    
    /** @brief 服务名称 */
    static std::string _service_name;
};

/** @brief 静态成员定义 */
template<typename Server>
std::string singleton_server<Server>::_service_name;

}; // namespace harbor_rpc

#endif//__HARBOR_RPC_UTIL_H__