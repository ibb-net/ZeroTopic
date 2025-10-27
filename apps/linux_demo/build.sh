#!/bin/bash

# OpenIBBOs Linux Demo 构建脚本

echo "=== OpenIBBOs Linux Demo 构建脚本 ==="

# 检查是否在正确的目录
if [ ! -f "CMakeLists.txt" ]; then
    echo "错误: 请在包含CMakeLists.txt的目录中运行此脚本"
    exit 1
fi

# 创建构建目录
echo "创建构建目录..."
mkdir -p build
cd build

# 配置CMake
echo "配置CMake..."
cmake ..

# 构建项目
echo "构建项目..."
make

# 检查构建是否成功
if [ $? -eq 0 ]; then
    echo "构建成功!"
    echo "可执行文件位置: build/bin/OpenIBBOs_Linux_Demo"
    echo ""
    echo "运行程序..."
    ./bin/OpenIBBOs_Linux_Demo
else
    echo "构建失败!"
    exit 1
fi
