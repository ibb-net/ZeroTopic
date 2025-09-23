#!/usr/bin/env bash
set -euo pipefail

# WSL 环境一键搭建脚本（相对路径版）
# 作用：
# 1) 安装构建与交叉编译依赖（APT）
# 2) 统一设置 SDK_TOOL_PATH 指向 OpenIBBOs/tool
# 3) 将 /usr/bin 映射为项目期望的 arm-none-eabi-gcc/Linux/bin（符号链接）
# 4) 基本自检与提示

echo "[1/5] 解析项目根目录..."
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
# SCRIPT_DIR = <repo>/OpenIBBOs/tool
ROOT_DIR=$(cd -- "${SCRIPT_DIR}/../.." && pwd)
OPENIBBOS_DIR="${ROOT_DIR}/OpenIBBOs"
TOOL_DIR="${OPENIBBOS_DIR}/tool"
GCC_DIR="${TOOL_DIR}/arm-none-eabi-gcc/Linux"

echo "SCRIPT_DIR = ${SCRIPT_DIR}"
echo "ROOT_DIR   = ${ROOT_DIR}"

echo "[2/5] 安装 APT 依赖（需要 sudo）..."
# 检查 sudo 或 root 权限
SUDO=""
if [ "$(id -u)" -ne 0 ]; then
  if command -v sudo >/dev/null 2>&1; then
    if sudo -n true 2>/dev/null; then
      SUDO="sudo"
    else
      echo "需要 sudo 权限但当前不可用。请先运行 'sudo -v' 缓存口令，或以 root 运行本脚本。"
      exit 1
    fi
  else
    echo "未检测到 sudo，请以 root 运行本脚本。"
    exit 1
  fi
fi

${SUDO} apt update
${SUDO} apt install -y \
  cmake ninja-build build-essential \
  gcc-arm-none-eabi gdb-multiarch \
  ca-certificates openssl \
  wget curl xz-utils

echo "[3/5] 准备工具链目录与符号链接..."
mkdir -p "${GCC_DIR}"
# 将系统中的 /usr/bin 作为工具链 bin 的来源（APT 方案）
ln -sfn /usr/bin "${GCC_DIR}/bin"

echo "[4/5] 设置 SDK_TOOL_PATH（当前会话有效）..."
export SDK_TOOL_PATH="${TOOL_DIR}"
echo "SDK_TOOL_PATH=${SDK_TOOL_PATH}"

# 如需持久化，请追加到 ~/.bashrc（若不存在则追加）
if ! grep -q "SDK_TOOL_PATH=.*OpenIBBOs/tool" "$HOME/.bashrc" 2>/dev/null; then
  echo "export SDK_TOOL_PATH=\"${TOOL_DIR}\"" >> "$HOME/.bashrc"
  echo "已将 SDK_TOOL_PATH 追加到 ~/.bashrc"
fi

echo "[5/5] 自检..."
set +e
"${GCC_DIR}/bin/arm-none-eabi-gcc" --version >/dev/null 2>&1
gcc_status=$?
set -e
if [ $gcc_status -ne 0 ]; then
  echo "警告：未找到 arm-none-eabi-gcc，可检查 APT 是否安装成功或 PATH/链接是否有效。"
  echo "期望可执行：${GCC_DIR}/bin/arm-none-eabi-gcc"
else
  echo "arm-none-eabi-gcc 可用："
  "${GCC_DIR}/bin/arm-none-eabi-gcc" --version | head -n 1
fi

cat <<'USAGE'

=== 使用说明 ===
1) 进入工程并配置构建（使用 CMake Presets）：
   cd dp2501project
   cmake --preset Debug
   cmake --build --preset Debug -j

2) 如需清理重配：
   rm -rf dp2501project/Build
   cmake --preset Release
   cmake --build --preset Release -j

3) 如需使用 xPack 工具链（可选）：
   - 将解压后的 xpack-arm-none-eabi-gcc-*/bin 目录以符号链接指向 OpenIBBOs/tool/arm-none-eabi-gcc/Linux/bin
     例如：
       cd OpenIBBOs/tool
       mkdir -p arm-none-eabi-gcc/Linux
       ln -sfn ./xpack-arm-none-eabi-gcc-*/bin ./arm-none-eabi-gcc/Linux/bin

USAGE

echo "完成。请按上面的使用说明进行构建。"


