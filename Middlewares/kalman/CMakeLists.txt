# CMakeLists.txt for Adaptive Kalman Filter

cmake_minimum_required(VERSION 3.10)

project(AdaptiveKalmanFilter)

# 设置C标准
set(CMAKE_C_STANDARD 99)

# 添加源文件
set(KALMAN_SOURCES
    kalman_filter.c
)

# 添加头文件
set(KALMAN_HEADERS
    kalman_filter.h
)

# 创建静态库
add_library(kalman_filter STATIC ${KALMAN_SOURCES})

# 设置包含目录
target_include_directories(kalman_filter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 编译选项
target_compile_options(kalman_filter PRIVATE
    -Wall
    -Wextra
    -O2
    -ffast-math
)

# 添加示例程序（可选）
option(BUILD_KALMAN_EXAMPLE "Build Kalman filter example" OFF)

if(BUILD_KALMAN_EXAMPLE)
    add_executable(kalman_example adaptive_kalman_example.c)
    target_link_libraries(kalman_example kalman_filter)
    target_link_libraries(kalman_example m)  # 链接数学库
endif()

# 安装规则
install(TARGETS kalman_filter
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${KALMAN_HEADERS}
    DESTINATION include/kalman
)
