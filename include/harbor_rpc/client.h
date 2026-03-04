/**
 * @file client.h
 * @brief harborRPC 客户端定义文件
 * 
 * 本文件定义了RPC客户端的核心接口，提供两个主要模板函数：
 * - call(): 同步调用，阻塞等待返回值
 * - send(): 异步发送，不等待返回值
 */

#ifndef __HARBOR_RPC_CLIENT_H__
#define __HARBOR_RPC_CLIENT_H__

#include "harbor_rpc/defines.h"
#include "harbor_rpc/harbor.h"

namespace harbor_rpc { namespace client {

	/**
	 * @brief 发起同步RPC调用v2版本（带返回值输出参数）
	 * 
	 * 该函数会阻塞当前线程，直到远端服务处理完成或超时。
	 * 参数会被自动打包为std::tuple，通过harbor路由到目标服务。
	 * 返回值通过输出参数返回。
	 * 
	 * @tparam Ret 返回值类型
	 * @tparam Args 可变参数类型列表
	 * @param[out] result 返回值输出参数，通过引用传递
	 * @param module 目标模块/服务名称
	 * @param cmd 命令名称
	 * @param args 函数参数
	 * @return int 状态码：
	 *         - errorcode::OK 表示成功，result有效
	 *         - 其他值表示错误
	 * 
	 * @example
	 * @code
	 * std::string result;
	 * int ret = harbor_rpc::client::call_v2<std::string>(result, "my_service", "get_name", user_id);
	 * if (ret == harbor_rpc::errorcode::OK) {
	 *     std::cout << "Result: " << result << std::endl;
	 * }
	 * @endcode
	 */
	template<typename Ret, typename... Args>
	int call_v2(Ret& result, const std::string& module, const std::string& cmd, Args&&... args) {
		// 检查系统是否正在关闭
		if (harbor_rpc::run == 0)
			return errorcode::TERMINATE;
		
		// 创建消息对象
		std::shared_ptr<harbor_rpc::MSG> msg = std::make_shared<harbor_rpc::MSG>();
		std::unique_lock<std::mutex> lock(msg->mtx);
		
		// 填充消息内容
		msg->module = module;
		msg->cmd = cmd;
		msg->params = std::make_any<decltype(std::make_tuple(args...))>(std::make_tuple(args...));
		msg->ret = errorcode::TIMEOUT;

		// 发送消息到目标服务
		int ret = harbor_rpc::core::harbor::instance().send_to(module, msg);
		if (ret != 0)
			return ret;

		// 阻塞等待返回值，超时时间5000毫秒
		msg->cv.wait_for(lock, std::chrono::milliseconds(5000), [msg] {
			return msg->ret != errorcode::TIMEOUT;
		});
		
		// 如果调用成功，提取返回值
		if (msg->ret == errorcode::OK && msg->has_result()) {
			try {
				result = msg->get_result<Ret>();
			} catch (const std::bad_any_cast&) {
				return errorcode::ERROR_PARAM_LIST;
			}
		}
		
		return msg->ret;
	}

	/**
	 * @brief 发起同步RPC调用v1版本（无返回值/返回int）
	 * 
	 * 该函数会阻塞当前线程，直到远端服务处理完成或超时。
	 * 适用于返回int类型或无返回值的RPC调用。
	 * 
	 * @tparam Args 可变参数类型列表
	 * @param module 目标模块/服务名称
	 * @param cmd 命令名称
	 * @param args 函数参数
	 * @return int 返回值：
	 *         - 成功时返回server端处理函数的返回值（如果是int类型）
	 *         - 失败时返回错误码
	 * 
	 * @note 对于成员函数，第一个参数需要传入对象指针
	 * @note 默认超时时间为5000毫秒
	 */
	template<typename... Args>
	int call_v1(const std::string& module, const std::string& cmd, Args&&... args) {
		// 检查系统是否正在关闭
		if (harbor_rpc::run == 0)
			return errorcode::TERMINATE;
		
		// 创建消息对象
		std::shared_ptr<harbor_rpc::MSG> msg = std::make_shared<harbor_rpc::MSG>();
		std::unique_lock<std::mutex> lock(msg->mtx);
		
		// 填充消息内容
		msg->module = module;
		msg->cmd = cmd;
		msg->params = std::make_any<decltype(std::make_tuple(args...))>(std::make_tuple(args...));
		msg->ret = errorcode::TIMEOUT;

		// 发送消息到目标服务
		int ret = harbor_rpc::core::harbor::instance().send_to(module, msg);
		if (ret != 0)
			return ret;

		// 阻塞等待返回值，超时时间5000毫秒
		msg->cv.wait_for(lock, std::chrono::milliseconds(5000), [msg] {
			return msg->ret != errorcode::TIMEOUT;
		});
		
		return msg->ret;
	}
	
	/**
	 * @brief call别名，默认指向call_v1
	 * 
	 * 通过修改此别名可以切换call的版本：
	 * - using call = call_v1;  // 使用旧版本
	 * - using call = call_v2;  // 使用新版本（需要指定返回类型模板参数）
	 */
	template<typename... Args>
	int call(const std::string& module, const std::string& cmd, Args&&... args) {
		return call_v1(module, cmd, std::forward<Args>(args)...);
	}

	/**
	 * @brief 发送异步RPC消息
	 * 
	 * 该函数将消息发送到目标服务后立即返回，不等待处理结果。
	 * 适用于不需要返回值的场景，如通知、事件推送等。
	 * 
	 * @tparam Args 可变参数类型列表
	 * @param module 目标模块/服务名称
	 * @param cmd 命令名称
	 * @param args 函数参数
	 * @return int 返回值：
	 *         - 0 表示发送成功
	 *         - 非零表示发送失败（错误码）
	 */
	template<typename... Args>
	int send(const std::string& module, const std::string& cmd, Args&&... args) {
		// 检查系统是否正在关闭
		if (harbor_rpc::run == 0)
			return errorcode::TERMINATE;
		
		// 创建消息对象
		std::shared_ptr<harbor_rpc::MSG> msg = std::make_shared<harbor_rpc::MSG>();
		std::unique_lock<std::mutex> lock(msg->mtx);
		
		// 填充消息内容
		msg->module = module;
		msg->cmd = cmd;
		msg->params = std::make_any<decltype(std::make_tuple(args...))>(std::make_tuple(args...));
		msg->ret = errorcode::TIMEOUT;

		// 发送消息到目标服务
		int ret = harbor_rpc::core::harbor::instance().send_to(module, msg);
		if (ret != 0)
			return ret;

		return 0;
	}

}}; // namespace harbor_rpc::client

#endif//__HARBOR_RPC_CLIENT_H__