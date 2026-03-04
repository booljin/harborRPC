/**
 * @file harbor.h
 * @brief harborRPC 核心调度器定义文件
 * 
 * 本文件定义了RPC框架的核心调度器harbor，所有RPC模块都需要在这里注册，
 * 消息也在这里路由。RPC客户端和服务端不会直接互相认识，而是通过harbor进行中转。
 * 
 * 工作流程：
 * 1. server会将自己注册到harbor（见regist_module接口）
 * 2. client在发起调用时，需要声明目标模块（见send_to接口）
 * 3. harbor将消息路由到正确的server端
 */

#ifndef __HARBOR_RPC_HARBOR_H__
#define __HARBOR_RPC_HARBOR_H__

#include "harbor_rpc/defines.h"
#include <string>
#include <map>

namespace harbor_rpc { namespace core{

class server_base;

// @brief RPC管理器
// @details 所有RPC模块都要在这里注册，消息也在这里路由
class harbor{
private:
	harbor(){};
	harbor(const harbor&) = delete;
	harbor& operator=(const harbor&) = delete;
public:
	static harbor& instance(){
		static harbor instance_;
		return instance_;
	}
private:
	// @brief RPC模块结构
	struct Module{
		server_base* handle;
	};


public:
	// @brief 注册一个RPC模块
	// @param module 模块名称，该名称应该符合一些约定，避免和其他模块冲突，也能让调用者方便的知道
	bool regist_module(const std::string& module, server_base*);

	// @brief 向指定模块发送消息
	// @param module 模块名称
	int send_to(const std::string& module, std::shared_ptr<harbor_rpc::MSG> msg);

private:
	std::map<std::string, Module> _modules;
	bool _run{ true };
};

}};

#endif//__HARBOR_RPC_HARBOR_H__