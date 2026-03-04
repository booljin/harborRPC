/**
 * @file example_rpc.h
 * @brief Example RPC服务示例
 * 
 * 本文件展示了如何使用harborRPC框架定义RPC服务。
 * 包含传统宏定义方式和新的任意类型返回值支持。
 */

#ifndef _EXAMPLE_RPC_H__
#define _EXAMPLE_RPC_H__

#include <string>
#include <vector>
#include "harbor_rpc.h"
#include <iostream>

/**
 * @brief 使用宏定义的RPC服务示例（传统方式，已过时）
 * @deprecated 建议使用新的继承方式
 */
DECLARE_RPC_SERVER_BEGIN(ExampleRPC, example_rpc)
public:
    /** @brief 静态命令处理函数示例 */
    static int s_do_cmd1(std::string p1, int p2);
    
    /** @brief 成员函数处理示例 */
    int s_do_cmd2(int a, int b);
    
    /** @brief 返回string类型的函数 */
    static std::string get_name(int id);
    
    /** @brief 返回vector<int>类型的函数 */
    static std::vector<int> get_list(int count);

DECLARE_RPC_SERVER_END()

//=============================================================================
// ExampleRPCv2: 使用新的rpc_server<T>模板基类方式（推荐）
//=============================================================================

/**
 * @class ExampleRPCv2
 * @brief 使用新模板基类的RPC服务示例（推荐方式）
 * 
 * 继承自rpc_server<ExampleRPCv2>，使用CRTP模式。
 * 相比宏定义方式，这种方式更加清晰、类型安全。
 */
class ExampleRPCv2 : public harbor_rpc::core::rpc_server<ExampleRPCv2> {
public:
    /** @brief 静态命令处理函数示例 */
    static int s_do_cmd1(std::string p1, int p2);
    
    /** @brief 成员函数处理示例 */
    int s_do_cmd2(int a, int b);
    
    /** @brief 返回string类型的函数 */
    static std::string get_name(int id);
    
    /** @brief 返回vector<int>类型的函数 */
    static std::vector<int> get_list(int count);
    
protected:
    /** @brief 注册命令处理函数 */
    void regist_cmds() override;
    
private:
    int _call_count = 0;  ///< 调用计数器，用于演示成员变量
};

/**
 * @brief 使用singleton_server包装器的单件版本
 * 
 * 通过singleton_server包装ExampleRPCv2，实现单件模式。
 * 可以通过regist_as_service()注册并获取单件实例。
 */
using ExampleRPCv2Singleton = harbor_rpc::singleton_server<ExampleRPCv2>;

#endif//_EXAMPLE_RPC_H__