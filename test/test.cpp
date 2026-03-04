/**
 * @file test.cpp
 * @brief harborRPC 测试程序
 * 
 * 本文件展示了harborRPC框架的各种使用方式：
 * 1. 传统的int返回值调用
 * 2. 任意类型返回值调用
 * 3. 异步发送
 */

#include "example_rpc.h"
#include <iostream>
#include <thread>

int main() {
    // 启动RPC服务
    ExampleRPC::instance().run();
    
    // 等待服务启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "=== 测试1: 传统int返回值调用 ===" << std::endl;
    
    // 异步发送（不等待返回值）
    int ret1 = harbor_rpc::client::send("example_rpc", "cmd1", std::string("test_param"), 42);
    std::cout << "send result: " << ret1 << std::endl;
    
    // 同步调用（返回int）
    int ret2 = harbor_rpc::client::call("example_rpc", "cmd2", &ExampleRPC::instance(), 10, 20);
    std::cout << "call result: " << ret2 << std::endl;
    
    std::cout << "\n=== 测试2: 任意类型返回值调用 (call_v2) ===" << std::endl;
    
    // 调用返回string的函数
    std::string name;
    int ret3 = harbor_rpc::client::call_v2<std::string>(name, "example_rpc", "get_name", 100);
    if (ret3 == harbor_rpc::errorcode::OK) {
        std::cout << "get_name result: " << name << std::endl;
    } else {
        std::cout << "get_name failed: " << ret3 << std::endl;
    }
    
    // 调用返回vector<int>的函数
    std::vector<int> list;
    int ret4 = harbor_rpc::client::call_v2<std::vector<int>>(list, "example_rpc", "get_list", 5);
    if (ret4 == harbor_rpc::errorcode::OK) {
        std::cout << "get_list result: ";
        for (int v : list) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
    } else {
        std::cout << "get_list failed: " << ret4 << std::endl;
    }
    
    std::cout << "\n=== 测试3: 错误处理测试 ===" << std::endl;
    
    // 测试未知命令
    int ret5 = harbor_rpc::client::call("example_rpc", "unknown_cmd");
    std::cout << "unknown_cmd result: " << ret5 << " (expected: " << harbor_rpc::errorcode::UNKNOW_CMD << ")" << std::endl;
    
    std::cout << "\n=== 测试4: ExampleRPCv2 (新模板基类方式) ===" << std::endl;
    
    // 创建并启动ExampleRPCv2服务
    ExampleRPCv2 service_v2;
    harbor_rpc::core::harbor::instance().regist_module("example_rpc_v2", &service_v2);
    service_v2.start();
    
    // 等待服务启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 测试v2服务
    int ret6 = harbor_rpc::client::call("example_rpc_v2", "cmd1", std::string("v2_test"), 99);
    std::cout << "v2 cmd1 result: " << ret6 << std::endl;
    
    int ret7 = harbor_rpc::client::call("example_rpc_v2", "cmd2", 5, 7);
    std::cout << "v2 cmd2 result: " << ret7 << std::endl;
    
    // 测试v2的string返回
    std::string name_v2;
    int ret8 = harbor_rpc::client::call_v2<std::string>(name_v2, "example_rpc_v2", "get_name", 200);
    if (ret8 == harbor_rpc::errorcode::OK) {
        std::cout << "v2 get_name result: " << name_v2 << std::endl;
    }
    
    // 测试v2的vector返回
    std::vector<int> list_v2;
    int ret9 = harbor_rpc::client::call_v2<std::vector<int>>(list_v2, "example_rpc_v2", "get_list", 3);
    if (ret9 == harbor_rpc::errorcode::OK) {
        std::cout << "v2 get_list result: ";
        for (int v : list_v2) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
    }
    
    std::cout << "\n=== 测试5: singleton_server单件包装器 ===" << std::endl;
    
    // 使用singleton_server包装器，注册为服务并启动
    ExampleRPCv2Singleton::regist_as_service("example_rpc_singleton").run();
    
    // 等待服务启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 通过单件方式调用
    int ret10 = harbor_rpc::client::call("example_rpc_singleton", "cmd1", std::string("singleton_test"), 123);
    std::cout << "singleton cmd1 result: " << ret10 << std::endl;
    
    int ret11 = harbor_rpc::client::call("example_rpc_singleton", "cmd2", 10, 20);
    std::cout << "singleton cmd2 result: " << ret11 << std::endl;
    
    // 获取服务名称
    std::cout << "singleton service name: " << ExampleRPCv2Singleton::service_name() << std::endl;
    
    // 停止服务
    std::cout << "\n=== 停止服务 ===" << std::endl;
    
    // 停止 singleton 服务（注意：singleton 服务会随静态对象析构自动停止）
    // ExampleRPCv2Singleton::instance().stop();  // 不手动停止，让析构函数处理
    
    // 停止 v2 服务
    service_v2.stop();
    
    // 停止 v1 服务
    ExampleRPC::instance().stop();
    
    std::cout << "测试完成!" << std::endl;
    
    // 等待一秒让所有线程完全退出
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    return 0;
}