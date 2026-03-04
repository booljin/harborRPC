# harborRPC

<!--
================================================================================
META DATA - 元数据
================================================================================
文档更新日期: 2026-03-04

【重要通知 - 致未来维护此文档的 AI】

1. 每次修改此文档时，请务必更新上方的"文档更新日期"，保持日期格式为 YYYY-MM-DD。
2. "致谢"章节是项目的重要信息，请务必保留，不得删除或修改。
================================================================================
-->

## 致谢

本项目的核心架构设计灵感来源于 [skynet](https://github.com/cloudwu/skynet) 框架。

特别感谢 **云风（吴云洋）** —— 中国游戏开发领域的资深技术专家、skynet 框架作者、前网易游戏核心成员。他开创性的 Actor 模型架构设计，以及 server-harbor-client 消息路由体系，为游戏服务端开发提供了优雅而高效的解决方案。

本项目的以下设计借鉴自 skynet：

- **Harbor 消息路由中心**：实现服务模块间的解耦通信
- **send/call 双模式接口**：异步发送与同步调用分离
- **服务模块化架构**：通过消息队列实现模块间通信

作者曾从事多年基于 skynet 的游戏服务端开发，如今转型 C++ 工业项目开发，将这一优秀的设计理念带入新的领域，以此致敬。

---

## 设计主旨

harborRPC 是一个轻量级、易用的进程内 RPC（远程过程调用）框架，专为 C++17 设计。

### 核心设计理念

1. **简单易用** - 通过模板元编程和宏，简化 RPC 服务的定义和调用
2. **类型安全** - 编译期参数类型检查，避免运行时错误
3. **灵活扩展** - 支持任意返回类型，不限于 `int`
4. **低侵入性** - 服务端代码与业务逻辑分离，便于维护
5. **高性能** - 基于消息队列的异步处理，支持并发调用

## 基本设计方案

### 架构概览

```
┌─────────────────────────────────────────────────────────┐
│                      Client API                         │
│  call() / call_v2() / send()                            │
└─────────────────────┬───────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────┐
│                    Harbor (路由中心)                     │
│  - 管理服务注册表                                        │
│  - 路由消息到目标服务                                    │
└─────────────────────┬───────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────┐
│                 Server (服务端基类)                      │
│  - 消息队列                                              │
│  - 工作线程                                              │
│  - 命令分发                                              │
└─────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 文件 | 说明 |
|------|------|------|
| `MSG` | defines.h | 消息结构，包含参数、返回值、同步原语 |
| `harbor` | harbor.h/cpp | 消息路由中心，管理服务注册 |
| `server_base` | server.h | 服务基类，处理消息循环 |
| `rpc_server<T>` | server.h | CRTP 模板基类，推荐使用 |
| `singleton_server<T>` | util.h | 单件包装器 |
| `client` | client.h | 客户端 API，提供 call/send 方法 |

### 调用流程

```
客户端                          Harbor                        服务端
  │                              │                              │
  │ call(module, cmd, args)      │                              │
  │─────────────────────────────>│                              │
  │                              │ 查找服务                      │
  │                              │─────────────────────────────>│
  │                              │                              │ 处理命令
  │                              │                              │ 执行回调
  │                              │<─────────────────────────────│
  │ 返回结果                      │                              │
  │<─────────────────────────────│                              │
  │                              │                              │
```

## 项目结构

```
harborRPC/
├── README.md                   # 本说明文档
├── LICENSE                     # MIT 授权文件
├── CMakeLists.txt              # CMake 构建配置
├── include/
│   ├── harbor_rpc.h            # 总入口头文件
│   └── harbor_rpc/
│       ├── defines.h           # 基础定义（MSG、errorcode 等）
│       ├── client.h            # 客户端 API
│       ├── server.h            # 服务端核心类
│       ├── harbor.h            # 消息路由中心
│       └── util.h              # 工具类（singleton_server）
├── src/
│   ├── harbor.cpp              # harbor 实现
│   └── server.cpp              # server_base 实现
├── test/
│   ├── test.cpp                # 测试主程序
│   ├── example_rpc.h           # 示例 RPC 服务定义
│   └── example_rpc.cpp         # 示例 RPC 服务实现
└── todolist/
    ├── 要求.md                  # 项目需求
    ├── plan.md                  # 实施计划
    └── todolist.md              # 任务清单
```

## 使用说明

### 1. 定义 RPC 服务

#### 方式一：使用 rpc_server 模板基类（推荐）

```cpp
#include "harbor_rpc.h"

class MyService : public harbor_rpc::core::rpc_server<MyService> {
public:
    MyService() { init(); regist_cmds(); }
    
protected:
    void regist_cmds() override {
        // 注册 int 返回值的命令
        regist("add", &MyService::add);
        regist("multiply", [this](int a, int b) { return a * b; });
        
        // 注册任意返回类型的命令
        regist_ex<std::string>("get_name", &MyService::get_name);
        regist_ex<std::vector<int>>("get_list", &MyService::get_list);
    }
    
private:
    static int add(int a, int b) { return a + b; }
    static std::string get_name(int id) { return "User_" + std::to_string(id); }
    static std::vector<int> get_list(int count) { /* ... */ }
};
```

#### 方式二：使用 singleton_server 包装器（单件模式）

```cpp
// 定义服务类
class MyRPC : public harbor_rpc::core::rpc_server<MyRPC> { /* ... */ };

// 包装为单件
using MyService = harbor_rpc::singleton_server<MyRPC>;

// 注册并启动
MyService::regist_as_service("my_service").run();

// 停止服务
MyService::instance().stop();
```

#### 方式三：使用宏定义（已过时）

```cpp
DECLARE_RPC_SERVER_BEGIN(MyService, my_service)
public:
    static int cmd_handler(int a, int b);
DECLARE_RPC_SERVER_END()
```

### 2. 启动服务

```cpp
// 多实例模式
MyService service1, service2;
harbor_rpc::core::harbor::instance().regist_module("service_alpha", &service1);
harbor_rpc::core::harbor::instance().regist_module("service_beta", &service2);
service1.start();
service2.start();

// 单件模式
MyService::regist_as_service("my_service").run();
```

### 3. 调用 RPC

```cpp
// 同步调用（返回 int）
int result = harbor_rpc::client::call("my_service", "add", 10, 20);

// 同步调用（任意返回类型）
std::string name;
int ret = harbor_rpc::client::call_v2<std::string>(name, "my_service", "get_name", 100);

std::vector<int> list;
int ret = harbor_rpc::client::call_v2<std::vector<int>>(list, "my_service", "get_list", 5);

// 异步发送（不等待返回）
harbor_rpc::client::send("my_service", "notify", message);
```

### 4. 错误处理

```cpp
int result = harbor_rpc::client::call("my_service", "cmd", args...);
if (result == harbor_rpc::errorcode::UNKNOW_CMD) {
    // 未知命令
} else if (result == harbor_rpc::errorcode::TIMEOUT) {
    // 超时
}
```

### 错误码定义

| 错误码 | 值 | 说明 |
|--------|-----|------|
| `OK` | 0 | 成功 |
| `UNKNOW_CMD` | -9997 | 未知命令 |
| `TIMEOUT` | -9999 | 调用超时 |
| `MODULE_NOT_FOUND` | -1 | 模块未找到 |

### 5. 性能统计

```cpp
// 获取统计数据
auto stats = service.get_statistics();
std::cout << "总调用: " << stats.total_calls << std::endl;
std::cout << "成功率: " << (double)stats.successful_calls / stats.total_calls << std::endl;
std::cout << "平均耗时: " << stats.avg_time_ms() << " ms" << std::endl;

// 重置统计
service.reset_statistics();
```

## 编译要求

- C++17 或更高版本
- CMake 3.24+

```bash
mkdir build && cd build
cmake ..
cmake --build . --config release
```

## 许可证

MIT License