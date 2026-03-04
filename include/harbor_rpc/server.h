/**
 * @file server.h
 * @brief harborRPC 服务端定义文件
 * 
 * 本文件定义了RPC服务端的核心组件，包括：
 * - function_traits: 函数类型推导模板
 * - server_base: RPC服务基类
 * - rpc_server: RPC服务模板基类（CRTP模式）
 * - DECLARE_RPC_SERVER_BEGIN/END: 服务声明宏（不推荐使用，建议使用新的模板方式）
 */

#ifndef __HARBOR_RPC_SERVER_H__
#define __HARBOR_RPC_SERVER_H__

#include <list>
#include <map>
#include <tuple>
#include <functional>
#include "harbor_rpc/defines.h"

namespace harbor_rpc {

	/**
	 * @brief 函数类型推导模板
	 * 
	 * 用于在编译期提取函数的返回类型、参数类型和参数数量等信息。
	 * 支持普通函数、函数指针和成员函数指针。
	 * 
	 * @tparam Func 函数类型
	 * 
	 * @example
	 * @code
	 * void foo(int, std::string);
	 * using traits = function_traits<decltype(foo)>;
	 * // traits::return_type -> void
	 * // traits::args_tuple_type -> std::tuple<int, std::string>
	 * // traits::arg_count -> 2
	 * // traits::arg<0>::type -> int
	 * // traits::arg<1>::type -> std::string
	 * @endcode
	 */
	template<typename Func>
	struct function_traits;

	/**
	 * @brief 普通函数的类型特化
	 * @tparam Ret 返回类型
	 * @tparam Args 参数类型列表
	 */
	template<typename Ret, typename... Args>
	struct function_traits<Ret(Args...)> {
		using return_type = Ret;                           /**< 返回类型 */
		using args_tuple_type = std::tuple<Args...>;       /**< 参数打包为tuple的类型 */
		static constexpr size_t arg_count = sizeof...(Args); /**< 参数数量 */
		
		/**
		 * @brief 获取第N个参数的类型
		 * @tparam N 参数索引（从0开始）
		 */
		template <std::size_t N>
		struct arg
		{
			static_assert(N < arg_count, "Argument index out of range");
			using type = typename std::tuple_element<N, args_tuple_type>::type;
		};
	};

	/** @brief 函数指针的类型特化 */
	template<typename Ret, typename... Args>
	struct function_traits<Ret(*)(Args...)> : function_traits<Ret(Args...)> {};

	/** @brief 成员函数指针的类型特化 */
	template<typename Class, typename Ret, typename... Args>
	struct function_traits<Ret(Class::*)(Args...)> : function_traits<Ret(Class*, Args...)> {};
	
	/** @brief const成员函数指针的类型特化 */
	template<typename Class, typename Ret, typename... Args>
	struct function_traits<Ret(Class::*)(Args...) const> : function_traits<Ret(Args...)> {};
	
	/** @brief std::function的类型特化 */
	template<typename Ret, typename... Args>
	struct function_traits<std::function<Ret(Args...)>> : function_traits<Ret(Args...)> {};
	
	/** @brief lambda表达式的类型特化（通过operator()推导） */
	template<typename T>
	struct function_traits : function_traits<decltype(&T::operator())> {};

}; // namespace harbor_rpc


namespace harbor_rpc { namespace core {

/**
 * @class server_base
 * @brief RPC服务端基类
 * 
 * 所有RPC服务都需要继承此类，并实现regist_cmds()方法来注册命令处理函数。
 * 服务端运行在独立的工作线程中，从消息队列中获取消息并处理。
 * 
 * @note 使用方式：
 *   1. 继承server_base类
 *   2. 在regist_cmds()中注册命令处理函数
 *   3. 调用run()启动服务线程
 *   4. 调用stop()停止服务
 * 
 * @example
 * @code
 * class MyService : public server_base {
 * protected:
 *     void regist_cmds() override {
 *         regist("cmd1", &MyService::handle_cmd1, this);
 *     }
 * private:
 *     int handle_cmd1(int a, int b) { return a + b; }
 * };
 * 
 * MyService service;
 * service.run();  // 启动服务
 * service.stop(); // 停止服务
 * @endcode
 */
class server_base {
public:
	/**
	 * @struct Statistics
	 * @brief 性能统计数据结构
	 */
	struct Statistics {
		size_t total_calls = 0;           /**< 总调用次数 */
		size_t successful_calls = 0;      /**< 成功调用次数 */
		size_t failed_calls = 0;          /**< 失败调用次数 */
		size_t timeout_calls = 0;         /**< 超时调用次数 */
		uint64_t total_time_ms = 0;       /**< 总处理时间(毫秒) */
		uint64_t max_time_ms = 0;         /**< 单次最大处理时间(毫秒) */
		uint64_t min_time_ms = UINT64_MAX;/**< 单次最小处理时间(毫秒) */
		
		/**
		 * @brief 获取平均处理时间
		 * @return double 平均处理时间(毫秒)
		 */
		double avg_time_ms() const {
			return total_calls > 0 ? (double)total_time_ms / total_calls : 0.0;
		}
	};

	server_base();
	~server_base();

private:
	/**
	 * @brief 处理消息队列中的消息
	 * 
	 * 从消息队列中取出消息，根据cmd查找对应的处理函数并执行。
	 * 处理完成后唤醒等待的调用方。
	 * 
	 * @param final 是否为最后一次执行（退出时处理积压消息）
	 * @return int 0表示队列为空，1表示还有消息未处理
	 */
	int do_cmds(bool final = false);
	
	/**
	 * @brief 日常任务，子类可重写
	 * 
	 * 在工作线程的每次循环中都会调用此函数，
	 * 子类可以在此执行定期任务（如心跳检测、状态清理等）。
	 */
	virtual void daily_task() {}

public:
	/**
	 * @brief 初始化接口，子类可重写
	 * 
	 * 在服务启动前调用，用于执行初始化操作
	 */
	virtual void init() {};

	/**
	 * @brief 注册命令处理函数（返回int类型）
	 * 
	 * 将函数指针注册到命令映射表中。函数的参数会被自动打包为std::tuple，
	 * 在调用时自动解包传递。返回值存储在msg->ret中。
	 * 
	 * @tparam Func 函数类型（支持普通函数、静态函数、成员函数）
	 * @param cmd 命令名称
	 * @param f 处理函数
	 * @return true 注册成功
	 * 
	 * @example
	 * @code
	 * // 注册静态函数
	 * regist("cmd1", &MyClass::static_handler);
	 * 
	 * // 注册成员函数（需要传入this）
	 * regist("cmd2", &MyClass::member_handler, this);
	 * @endcode
	 */
	template<typename Func>
	bool regist(const std::string& cmd, Func f) {
		using ArgsTuple = typename function_traits<Func>::args_tuple_type;
		auto func = [f](const std::any& params, [[maybe_unused]] std::shared_ptr<MSG> msg) {
			ArgsTuple p = std::any_cast<ArgsTuple>(params);
			return std::apply(f, p);
		};
		_cmds[cmd] = func;
		return true;
	};
	
	/**
	 * @brief 注册命令处理函数（支持任意返回类型）
	 * 
	 * 将函数指针注册到命令映射表中，支持任意返回类型。
	 * 返回值会存储在msg->result中（std::any类型）。
	 * 
	 * @tparam Ret 返回值类型
	 * @tparam Func 函数类型
	 * @param cmd 命令名称
	 * @param f 处理函数
	 * @return true 注册成功
	 * 
	 * @example
	 * @code
	 * // 注册返回string的函数
	 * regist<std::string>("get_name", &MyClass::get_name);
	 * 
	 * // 注册返回vector的函数
	 * regist<std::vector<int>>("get_list", &MyClass::get_list);
	 * @endcode
	 */
	template<typename Ret, typename Func>
	bool regist_ex(const std::string& cmd, Func f) {
		using ArgsTuple = typename function_traits<Func>::args_tuple_type;
		auto func = [f](const std::any& params, std::shared_ptr<MSG> msg) {
			ArgsTuple p = std::any_cast<ArgsTuple>(params);
			Ret result = std::apply(f, p);
			msg->result = std::make_any<Ret>(std::move(result));
			return errorcode::OK;
		};
		_cmds[cmd] = func;
		return true;
	};

	/**
	 * @brief 将消息投递到队列
	 * @param msg 消息指针
	 * @return int 0表示成功
	 */
	int post_msg(std::shared_ptr<MSG>);
	
	/**
	 * @brief 获取命令映射表
	 * @return std::map<std::string, std::function<int(const std::any&, std::shared_ptr<MSG>)>>& 命令映射表引用
	 */
	inline std::map<std::string, std::function<int(const std::any&, std::shared_ptr<MSG>)>>& cmd_map() { return _cmds; }

public:
	/**
	 * @brief 启动服务线程
	 * 
	 * 创建并启动工作线程，开始处理消息
	 */
	void run();
	
	/**
	 * @brief 停止服务线程
	 * 
	 * 设置停止标志，等待工作线程处理完积压消息后退出
	 */
	void stop();

public:
	/**
	 * @brief 设置非忙碌时的休眠时间
	 * @param ms 休眠时间（毫秒）
	 */
	inline void set_sleepms_when_not_busy(int ms) { _sleepms_when_not_busy = ms; }
	
	/**
	 * @brief 设置单次工作的时间限制
	 * @param ms 时间限制（毫秒）
	 */
	inline void set_working_time_limit(int ms) { _working_time_limit = ms; }
	
	/**
	 * @brief 获取性能统计数据
	 * @return Statistics 统计数据副本
	 */
	inline Statistics get_statistics() const {
		std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(_stats_mtx));
		return _stats;
	}
	
	/**
	 * @brief 重置性能统计数据
	 */
	inline void reset_statistics() {
		std::lock_guard<std::mutex> lock(_stats_mtx);
		_stats = Statistics();
	}

private:
	/**
	 * @brief 注册命令处理函数（纯虚函数）
	 * 
	 * 子类必须实现此函数，在其中调用regist()注册所有命令处理函数
	 */
	virtual void regist_cmds() = 0;

private:
	/** @brief 命令处理函数映射表 */
	std::map<std::string, std::function<int(const std::any&, std::shared_ptr<MSG>)>> _cmds;
	
	/** @brief 性能统计数据 */
	Statistics _stats;
	
	/** @brief 统计数据互斥锁 */
	mutable std::mutex _stats_mtx;

protected:
	/** @brief 工作线程 */
	std::thread _thread;
	
	/** @brief 运行标志，true表示继续运行 */
	bool _run;
	
	/** @brief 非忙碌时的休眠时间（毫秒） */
	int _sleepms_when_not_busy = 10;
	
	/** @brief 每次处理消息的时间片限制（毫秒） */
	int _working_time_limit = 10;

	/** @brief 消息队列互斥锁 */
	std::mutex _msg_queue_mtx;
	
	/** @brief 消息队列 */
	std::list<std::shared_ptr<harbor_rpc::MSG>> _msg_queue;
};

/**
 * @class rpc_server
 * @brief RPC服务模板基类（CRTP模式）
 * 
 * 使用CRTP（Curiously Recurring Template Pattern）模式，
 * 提供更现代、更安全的RPC服务定义方式，替代宏定义。
 * 
 * @tparam Derived 派生类类型
 * 
 * @note 一个RPC服务类可以有多个实例，每个实例在注册到harbor时指定名字。
 *       如需单件模式，请使用harbor_rpc::singleton_server包装器。
 * 
 * @example
 * @code
 * class MyService : public harbor_rpc::core::rpc_server<MyService> {
 * protected:
 *     void regist_cmds() override {
 *         regist("cmd1", &MyService::handle_cmd1);
 *         regist("cmd2", [this](int a, int b) { return this->handle_cmd2(a, b); });
 *         regist_ex<std::string>("get_name", &MyService::get_name);
 *     }
 *     
 * private:
 *     static int handle_cmd1(int a, int b) { return a + b; }
 *     int handle_cmd2(int a, int b) { return a * b; }
 *     static std::string get_name(int id) { return "User_" + std::to_string(id); }
 * };
 * 
 * // 使用方式1：多实例（手动注册）
 * MyService service1, service2;
 * harbor_rpc::core::harbor::instance().regist_module("service_alpha", &service1);
 * harbor_rpc::core::harbor::instance().regist_module("service_beta", &service2);
 * service1.start();
 * service2.start();
 * 
 * // 使用方式2：单件模式（使用singleton_server包装器）
 * using MyServiceSingleton = harbor_rpc::singleton_server<MyService>;
 * MyServiceSingleton::init("my_service").start();
 * @endcode
 */
template<typename Derived>
class rpc_server : public server_base {
public:
    rpc_server() = default;
    virtual ~rpc_server() = default;
    
    // 禁止拷贝和赋值
    rpc_server(const rpc_server&) = delete;
    rpc_server& operator=(const rpc_server&) = delete;
    
    /**
     * @brief 启动服务
     * 
     * 启动工作线程。需要在调用前手动注册到harbor。
     * 使用方式：
     * @code
     * MyService service;
     * harbor_rpc::core::harbor::instance().regist_module("my_service", &service);
     * service.start();
     * @endcode
     */
    void start() {
        run();
    }
    
    /**
     * @brief 停止服务
     */
    void shutdown() {
        stop();
    }
};

}}; // namespace harbor_rpc::core


/**
 * @defgroup RPCServerMacros RPC服务声明宏
 * @brief 用于简化RPC服务类声明的宏定义
 * 
 * @warning 这些宏已过时，建议使用新的rpc_server<T>模板基类方式
 * @deprecated 请使用rpc_server模板基类代替
 * @{
 */

/**
 * @brief 声明RPC服务类开始
 * 
 * 使用方式：
 * @code
 * DECLARE_RPC_SERVER_BEGIN(MyService, my_service)
 * public:
 *     static int cmd_handler(int a, int b);
 * DECLARE_RPC_SERVER_END()
 * @endcode
 * 
 * @param class_name 类名
 * @param module_name 模块名（用于路由）
 */
#define DECLARE_RPC_SERVER_BEGIN(class_name, module_name)\
class class_name : public harbor_rpc::core::server_base{\
friend class harbor_rpc::core::harbor;\
private:\
	class_name(){ init(); regist_cmds();}\
	class_name(const class_name&) = delete;\
	class_name& operator=(const class_name&) = delete;\
public:\
	static class_name& instance(){\
		static class_name* class_name##_instance = nullptr;\
		if(class_name##_instance == nullptr){\
			class_name##_instance = new class_name();\
			harbor_rpc::core::harbor::instance().regist_module(#module_name, class_name##_instance);\
		}\
		return *class_name##_instance;\
	}\
private:\
	void regist_cmds() override;

/**
 * @brief 声明RPC服务类结束
 */
#define DECLARE_RPC_SERVER_END() };

/** @} */ // end of RPCServerMacros group

#endif//__HARBOR_RPC_SERVER_H__