/**
 * @file server.cpp
 * @brief harborRPC 服务端实现文件
 * 
 * 本文件实现了server_base类的方法，包括：
 * - 线程启动与停止
 * - 消息队列处理
 * - 命令分发执行
 */

#include "harbor_rpc/server.h"

#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>

#include "harbor_rpc/defines.h"

namespace harbor_rpc {

namespace core {

/**
 * @brief 构造函数
 * 
 * 初始化运行标志为true
 */
server_base::server_base() : _run(true) {
}

/**
 * @brief 析构函数
 * 
 * 确保服务线程正确停止
 */
server_base::~server_base() {
	if (_run && _thread.joinable()) {
		stop();
	}
}

/**
 * @brief 启动服务线程
 * 
 * 创建并启动工作线程。工作线程会持续从消息队列中获取消息并处理，
 * 直到调用stop()方法。
 * 
 * 线程工作流程：
 * 1. 执行日常任务（daily_task）
 * 2. 处理消息队列中的消息
 * 3. 根据队列是否空闲决定休眠时间
 * 4. 重复步骤1-3，直到_run为false
 * 5. 退出前处理完所有积压消息
 */
void server_base::run() {
	// 在启动线程前注册命令
	regist_cmds();
	
	_thread = std::thread([&, this]() {
		while (_run) {
			// 执行日常任务（子类可重写）
			daily_task();
			
			// 处理消息队列
			int ret = do_cmds();
			
			// 根据队列状态决定休眠时间
			if (ret == 0)
				// 队列为空，休眠较长时间
				std::this_thread::sleep_for(std::chrono::milliseconds(_sleepms_when_not_busy));
			else
				// 队列还有消息，短暂休眠后继续处理
				std::this_thread::sleep_for(std::chrono::milliseconds(0));
		}
		
		// 退出前处理完所有积压消息
		do_cmds(true);
	});
}

/**
 * @brief 停止服务线程
 * 
 * 设置停止标志，等待工作线程处理完积压消息后退出。
 * 线程会通过join()等待完成。
 */
void server_base::stop() {
	_run = false;
	if (_thread.joinable())
		_thread.join();
}

/**
 * @brief 处理消息队列中的消息
 * 
 * 从消息队列中取出消息，根据cmd查找对应的处理函数并执行。
 * 支持时间片控制，避免长时间占用CPU。
 * 
 * 处理流程：
 * 1. 记录开始时间
 * 2. 从队列取出消息
 * 3. 根据cmd查找处理函数
 * 4. 执行处理函数并设置返回值
 * 5. 唤醒等待的客户端
 * 6. 检查是否超过时间片限制
 * 
 * @param final 是否为最后一次执行（退出时处理积压消息）
 *              - true: 处理完所有消息
 *              - false: 受时间片限制
 * @return int 
 *         - 0: 队列为空
 *         - 1: 还有消息未处理
 */
int server_base::do_cmds(bool final) {
	// 记录开始时间，用于时间片控制
	auto start = std::chrono::system_clock::now();
	
	// 检查队列是否为空
	if (_msg_queue.size() == 0)
		return 0;
	
	// 处理消息队列
	while (_msg_queue.size() > 0) {
		// 从队列头部取出消息
		std::unique_lock<std::mutex> lock(_msg_queue_mtx);
		std::shared_ptr<harbor_rpc::MSG> msg = _msg_queue.front();
		_msg_queue.pop_front();
		lock.unlock();
		
		// 查找命令处理函数
		auto& cmd_map_ = cmd_map();
		if (cmd_map_.find(msg->cmd) != cmd_map_.end()) {
			// 找到处理函数，执行调用
			try {
				std::lock_guard<std::mutex> msg_lock(msg->mtx);
				// 执行处理函数并获取返回值（传入msg以支持任意类型返回值）
				int ret = cmd_map_[msg->cmd](msg->params, msg);
				msg->ret = ret;
			} catch (std::bad_any_cast&) {
				// 参数类型转换失败
				msg->ret = harbor_rpc::errorcode::ERROR_PARAM_LIST;
			} catch (...) {
				// 其他未知异常
				msg->ret = harbor_rpc::errorcode::UNKNOW;
			}
			// 唤醒等待的客户端
			msg->cv.notify_all();
		} else {
			// 未找到命令处理函数
			std::lock_guard<std::mutex> msg_lock(msg->mtx);
			msg->ret = harbor_rpc::errorcode::UNKNOW_CMD;
			msg->cv.notify_all();
		}
		
		// 时间片控制：检查是否超过限制
		if (!final) {
			auto n = std::chrono::system_clock::now();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(n - start).count() >= _working_time_limit)
				break;
		}
	}
	
	// 返回队列状态
	if (_msg_queue.size() == 0)
		return 0;
	else
		return 1;
}

/**
 * @brief 将消息投递到队列
 * 
 * 线程安全地将消息添加到消息队列尾部。
 * 
 * @param msg 消息智能指针
 * @return int 始终返回0表示成功
 */
int server_base::post_msg(std::shared_ptr<MSG> msg) {
	std::lock_guard<std::mutex> lock(_msg_queue_mtx);
	_msg_queue.emplace_back(msg);
	return 0;
}

}}; // namespace harbor_rpc::core