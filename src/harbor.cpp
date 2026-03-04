/**
 * @file harbor.cpp
 * @brief harborRPC 消息路由中心实现文件
 * 
 * 本文件实现了harbor类的方法，包括：
 * - 模块注册
 * - 消息路由
 * - 全局运行控制
 */

#include "harbor_rpc/harbor.h"

#include <memory>
#include <cassert>

#include "harbor_rpc/defines.h"
#include "harbor_rpc/server.h"

namespace harbor_rpc {

/**
 * @brief 全局运行标志
 * 
 * 当该值为1时，系统正常运行
 * 当该值为0时，系统需要停止，所有RPC调用将返回TERMINATE错误
 */
int run = 1;

namespace core {

/**
 * @brief 注册RPC服务模块
 * 
 * 将服务端实例注册到Harbor的模块映射表中。
 * 注册成功后，客户端可以通过模块名称调用该服务。
 * 
 * @param module 模块名称（需要保证唯一性）
 * @param s 服务端实例指针
 * @return true 注册成功
 * 
 * @note 重复注册相同名称会触发断言错误
 */
bool harbor::regist_module(const std::string& module, server_base* s) {
	// 断言检查：确保模块名称未被注册
	assert(_modules.find(module) == _modules.end());
	
	// 创建模块信息
	Module m;
	m.handle = s;
	
	// 注册到映射表
	_modules[module] = m;

	return true;
}

/**
 * @brief 向指定模块发送消息
 * 
 * 将消息路由到目标服务端的消息队列中。
 * 首先检查系统是否正在关闭，然后查找目标模块，
 * 最后将消息投递到目标服务端。
 * 
 * @param module 目标模块名称
 * @param msg 消息智能指针
 * @return int 返回值：
 *         - 0: 发送成功
 *         - -1: 模块不存在
 *         - errorcode::TERMINATE: 系统正在关闭
 */
int harbor::send_to(const std::string& module, std::shared_ptr<harbor_rpc::MSG> msg) {
	// 检查系统是否正在关闭
	if (harbor_rpc::run != 1) {
		msg->ret = errorcode::TERMINATE;
		msg->cv.notify_all();
		return msg->ret;
	}
	
	// 查找目标模块
	if (_modules.find(module) == _modules.end())
		return -1;
	
	// 获取服务端句柄并投递消息
	server_base* handle = _modules[module].handle;
	handle->post_msg(msg);
	
	return 0;
}

}}; // namespace harbor_rpc::core