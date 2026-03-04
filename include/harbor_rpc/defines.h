/**
 * @file defines.h
 * @brief harborRPC 核心定义文件
 * 
 * 本文件定义了harborRPC框架的核心数据结构和错误码。
 * harborRPC是一个用于线程间通信的轻量级RPC框架，支持同步调用(call)和异步发送(send)两种模式。
 */

#ifndef __HARBOR_RPC_DEFINES_H__
#define __HARBOR_RPC_DEFINES_H__

#include <string>
#include <any>
#include <condition_variable>

namespace harbor_rpc {

	/**
	 * @struct MSG
	 * @brief RPC消息结构体，封装了RPC调用的完整上下文
	 * 
	 * 该结构体是RPC通信的核心数据结构，承载了从调用方到服务方的所有信息，
	 * 包括路由信息、参数、同步原语和返回值。
	 * 
	 * @note 生命周期说明：
	 *   - 调用方创建MSG对象（通常通过shared_ptr管理）
	 *   - 对于send调用：消息发送后由server端处理并销毁
	 *   - 对于call调用：消息发送后，调用方阻塞等待返回值，server端处理完成后唤醒调用方
	 *   - 返回值返回后，MSG对象由shared_ptr自动销毁
	 */
	struct MSG {
		/** @brief 目标模块/服务名称，用于路由到正确的server */
		std::string module;
		
		/** @brief 命令名称，用于在server端查找对应的处理函数 */
		std::string cmd;
		
		/** 
		 * @brief 条件变量，用于call模式下的同步等待
		 * 
		 * 当调用方使用call()发起同步调用时，会在该条件变量上等待，
		 * 直到server端处理完成并notify_all唤醒。
		 */
		std::condition_variable cv;
		
		/** 
		 * @brief 互斥锁，与cv配合使用
		 * 
		 * 保护返回值的并发访问，确保线程安全
		 */
		std::mutex mtx;
		
		/** 
		 * @brief 参数容器，存储打包后的函数参数
		 * 
		 * 参数被打包为std::tuple，然后存储为std::any，
		 * 在server端解包后传递给注册的处理函数
		 */
		std::any params;
		
		/** 
		 * @brief 返回值（状态码）
		 * 
		 * 存储RPC调用的状态码，初始值为TIMEOUT
		 * server端处理完成后会更新该值
		 * 用于表示调用是否成功，不用于存储实际返回值
		 */
		int ret;
		
		/** 
		 * @brief 返回值（任意类型）
		 * 
		 * 存储RPC调用的实际返回值，支持任意类型
		 * 通过std::any实现类型擦除
		 * 调用方需要通过get_result<T>()获取具体类型的返回值
		 */
		std::any result;
		
		/**
		 * @brief 获取返回值
		 * 
		 * 从result中提取具体类型的返回值
		 * 如果类型不匹配或result为空，会抛出std::bad_any_cast异常
		 * 
		 * @tparam T 返回值类型
		 * @return T 返回值
		 */
		template<typename T>
		T get_result() const {
			return std::any_cast<T>(result);
		}
		
		/**
		 * @brief 检查返回值是否有效
		 * @return true result中有有效的返回值
		 * @return false result为空或无效
		 */
		bool has_result() const {
			return result.has_value();
		}
	};

	/**
	 * @namespace errorcode
	 * @brief RPC框架错误码定义
	 * 
	 * 定义了RPC框架内部使用的错误码，用于区分不同类型的错误
	 */
	namespace errorcode {
		/**
		 * @enum RET
		 * @brief RPC返回值枚举
		 */
		enum RET {
			OK = 0,                    /**< 调用成功 */
			VERSION_ERROR = -100,      /**< 版本不匹配 */
			TERMINATE = -9995,         /**< 系统正在关闭 */
			ERROR_PARAM_LIST = -9996,  /**< 参数列表错误，参数类型或数量不匹配 */
			UNKNOW_CMD = -9997,        /**< 未知命令，server端未注册该cmd */
			UNKNOW = -9998,            /**< 未知错误 */
			TIMEOUT = -9999,           /**< 调用超时 */
		};
	}

	/**
	 * @brief 全局运行标志
	 * 
	 * 当该值为0时，表示系统需要停止，所有RPC调用将返回TERMINATE错误
	 * 通过stop_all()函数设置该值为0
	 */
	extern int run;
	
	/**
	 * @brief 停止所有RPC服务
	 * 
	 * 设置全局运行标志为0，通知所有RPC组件停止运行
	 * 通常在程序退出前调用
	 */
	inline void stop_all() {
		run = 0;
	}

}; // namespace harbor_rpc

#endif//__HARBOR_RPC_DEFINES_H__
