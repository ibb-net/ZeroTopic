# 安装指南

本文档详细介绍 OpenIBBOs 的安装和配置步骤。

## 系统要求

### 硬件平台
- ARM Cortex-M 系列微控制器
- RISC-V 架构（部分支持）
- 其他支持 C99 标准的嵌入式平台

### 软件环境
- **编译器**: GCC 7.0+ / Clang 6.0+ / ARM Compiler 6.0+
- **构建系统**: CMake 3.10+ / Makefile
- **操作系统**: FreeRTOS / RT-Thread / 裸机 / Linux

## 安装方式

### 方式一：Git 子模块（推荐）

适合已使用 Git 管理的项目：

```bash
# 在项目根目录执行
git submodule add https://github.com/yourusername/OpenIBBOs.git
git submodule update --init --recursive
```

### 方式二：直接克隆

```bash
git clone https://github.com/yourusername/OpenIBBOs.git
```

### 方式三：手动集成

下载源代码并解压到项目目录。

## CMake 配置

### 基本配置

```cmake
cmake_minimum_required(VERSION 3.10)
project(your_project C CXX)

# 添加 OpenIBBOs
add_subdirectory(OpenIBBOs/zero_topic_core)

# 链接库
target_link_libraries(your_target PRIVATE zero_topic_core)

# 包含头文件
target_include_directories(your_target PRIVATE 
    OpenIBBOs/zero_topic_core/topic_bus
    OpenIBBOs/zero_topic_core/obj_dict
    OpenIBBOs/zero_topic_core/ring_buffer
)
```

### 配置选项

在 `CMakeLists.txt` 中设置配置选项：

```cmake
# 启用调试信息
set(ZERO_TOPIC_DEBUG ON)

# 设置最大 Topic 数量
set(TOPIC_BUS_MAX_TOPICS 32)

# 设置对象字典最大条目数
set(OBJ_DICT_MAX_ENTRIES 128)
```

## 配置文件

### Topic 总线配置

编辑 `topic_bus_config.h`:

```c
#define TOPIC_BUS_MAX_TOPICS      32
#define TOPIC_BUS_MAX_SUBSCRIBERS 16
#define TOPIC_EVENT_TIMEOUT_MS    1000
```

### 对象字典配置

编辑 `obj_dict_config.h`:

```c
#define OBJ_DICT_MAX_ENTRIES  128
#define OBJ_DICT_USE_MEMPOOL  1
#define OBJ_DICT_MEMPOOL_SIZE 4096
```

## 编译

### 使用 CMake

```bash
mkdir build && cd build
cmake ..
make
```

### 使用 Makefile

```bash
cd OpenIBBOs/zero_topic_core
make
```

## 验证安装

创建测试程序验证安装：

```c
#include "topic_bus.h"
#include "obj_dict.h"

int main() {
    // 初始化
    topic_bus_init();
    obj_dict_init();
    
    // 创建测试 Topic
    topic_id_t test_topic = 0x0001;
    topic_bus_create_topic(test_topic, 0, 4);
    
    printf("OpenIBBOs installed successfully!\n");
    return 0;
}
```

编译并运行，如果看到成功消息，说明安装正确。

## 故障排除

### 编译错误

**问题**: 找不到头文件
- **解决**: 检查 `target_include_directories` 配置

**问题**: 链接错误
- **解决**: 确保已链接 `zero_topic_core` 库

### 运行时错误

**问题**: 内存不足
- **解决**: 调整配置中的最大值参数

**问题**: 初始化失败
- **解决**: 检查堆栈大小和内存分配

## 下一步

- 阅读 [快速开始指南](getting-started.md)
- 查看 [API 参考文档](zero_topic_core/topic_bus/topic_bus.md)
- 参考示例代码

## 相关文档

- [WSL/CMake/GCC 配置指南](../Doc/wsl_cmake_gcc.md)
- [贡献指南](../CONTRIBUTING.md)
