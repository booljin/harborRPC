/**
 * @file example_rpc.cpp
 * @brief Example RPC服务实现
 * 
 * 本文件展示了如何实现RPC服务处理函数并注册。
 */

#include "test/example_rpc.h"
#include <iostream>

/**
 * @brief 注册命令处理函数
 * 
 * 展示了两种注册方式：
 * 1. regist() - 用于返回int类型的函数
 * 2. regist_ex<Ret>() - 用于返回任意类型的函数
 */
void ExampleRPC::regist_cmds() {
    // 传统方式注册int返回值的函数
    regist("cmd1", ExampleRPC::s_do_cmd1);
    regist("cmd2", &ExampleRPC::s_do_cmd2);
    
    // 新方式：注册返回string的函数
    regist_ex<std::string>("get_name", ExampleRPC::get_name);
    
    // 新方式：注册返回vector<int>的函数
    regist_ex<std::vector<int>>("get_list", ExampleRPC::get_list);
}

/**
 * @brief 静态命令处理函数示例
 * @param p1 字符串参数
 * @param p2 整数参数
 * @return int 返回值
 */
int ExampleRPC::s_do_cmd1(std::string p1, int p2) {
    ExampleRPC& t = ExampleRPC::instance();
    std::cout << &t << " " << p1 << " " << p2 << std::endl;
    return 789;
}

/**
 * @brief 成员函数处理示例
 * @param p1 第一个整数
 * @param p2 第二个整数
 * @return int 返回值
 */
int ExampleRPC::s_do_cmd2(int p1, int p2) {
    std::cout << this << " cmd2 " << " " << p1 << " " << p2 << std::endl;
    return p1 + p2;
}

/**
 * @brief 返回string类型的函数示例
 * @param id 用户ID
 * @return std::string 用户名称
 */
std::string ExampleRPC::get_name(int id) {
    return "User_" + std::to_string(id);
}

/**
 * @brief 返回vector<int>类型的函数示例
 * @param count 列表元素数量
 * @return std::vector<int> 整数列表
 */
std::vector<int> ExampleRPC::get_list(int count) {
    std::vector<int> result;
    for (int i = 0; i < count; ++i) {
        result.push_back(i * 10);
    }
    return result;
}


//=============================================================================
// ExampleRPCv2 实现（使用新的rpc_server<T>模板基类）
//=============================================================================

/**
 * @brief 注册命令处理函数
 */
void ExampleRPCv2::regist_cmds() {
    // 传统方式注册int返回值的函数
    regist("cmd1", ExampleRPCv2::s_do_cmd1);
    
    // 成员函数需要使用lambda绑定this
    regist("cmd2", [this](int a, int b) { return this->s_do_cmd2(a, b); });
    
    // 新方式：注册返回string的函数
    regist_ex<std::string>("get_name", ExampleRPCv2::get_name);
    
    // 新方式：注册返回vector<int>的函数
    regist_ex<std::vector<int>>("get_list", ExampleRPCv2::get_list);
}

/**
 * @brief 静态命令处理函数示例
 */
int ExampleRPCv2::s_do_cmd1(std::string p1, int p2) {
    std::cout << "[v2] s_do_cmd1: " << p1 << " " << p2 << std::endl;
    return 789;
}

/**
 * @brief 成员函数处理示例
 */
int ExampleRPCv2::s_do_cmd2(int p1, int p2) {
    _call_count++;
    std::cout << "[v2] s_do_cmd2: " << p1 << " + " << p2 << " (call #" << _call_count << ")" << std::endl;
    return p1 + p2;
}

/**
 * @brief 返回string类型的函数示例
 */
std::string ExampleRPCv2::get_name(int id) {
    return "User_v2_" + std::to_string(id);
}

/**
 * @brief 返回vector<int>类型的函数示例
 */
std::vector<int> ExampleRPCv2::get_list(int count) {
    std::vector<int> result;
    for (int i = 0; i < count; ++i) {
        result.push_back(i * 100);  // 使用100作为倍数，便于区分v1版本
    }
    return result;
}