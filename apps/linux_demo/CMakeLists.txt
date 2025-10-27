cmake_minimum_required(VERSION 3.10)

# 项目名称
project(OpenIBBOs_Linux_Demo)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# 设置RTE路径
set(RTE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(RTE_INC_DIR ${RTE_ROOT}/Rte/inc)
set(RTE_SRC_DIR ${RTE_ROOT}/Rte/src)

# 包含头文件目录
include_directories(${RTE_INC_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/vfb)

# RTE源文件
set(RTE_SOURCES
    ${RTE_SRC_DIR}/linux/os_init.c
    ${RTE_SRC_DIR}/posix/os_printf.c
    ${RTE_SRC_DIR}/posix/os_thread.c
    ${RTE_SRC_DIR}/linux/os_mutex.c
    ${RTE_SRC_DIR}/linux/os_mmap.c
    ${RTE_SRC_DIR}/posix/os_heap.c
    ${RTE_SRC_DIR}/posix/os_queue.c
    ${RTE_SRC_DIR}/posix/os_tick.c
    ${RTE_SRC_DIR}/linux/os_semaphore.c
)

# Middleware源文件
set(MIDDLEWARE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/common/os_list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/common/ringbuf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/vfb/vfb.c
)

# 主程序源文件
set(MAIN_SOURCES
    main.c
    thread_a.c
    thread_b.c
    vfb_test.c
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${MAIN_SOURCES} ${RTE_SOURCES} ${MIDDLEWARE_SOURCES})

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
)

# 打印配置信息
message(STATUS "RTE Root: ${RTE_ROOT}")
message(STATUS "RTE Include: ${RTE_INC_DIR}")
message(STATUS "RTE Sources: ${RTE_SOURCES}")
message(STATUS "Middleware Sources: ${MIDDLEWARE_SOURCES}")

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
