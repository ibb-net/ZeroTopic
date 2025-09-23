## 在 WSL 下使用 CMake + GCC（arm-none-eabi）编译 OpenIBBOs

本指南说明如何在 WSL(Ubuntu) 环境使用 CMake + Ninja + `arm-none-eabi-gcc` 构建本项目（以 `dp2501project` 为例）。

### 0. 目录约定
在 WSL 中推荐目录结构如下（`borui` 为项目根，以下命令均在该目录执行）：
```plaintext
borui/
├── OpenIBBOs/             # SDK 与公共中间件
│  └── tool/               # 统一存放交叉工具链与相关工具
└── dp2501project/         # 实际工程（含 CMakePresets、cmake/、Application/ 等）
```
工具链统一放在 `OpenIBBOs/tool/`，通过环境变量 `SDK_TOOL_PATH` 指向该目录。

### 1. 安装必需工具
```bash
sudo apt update
sudo apt install -y cmake ninja-build build-essential wget xz-utils
```

### 2. 获取 ARM 交叉编译工具链
  ```bash
  sudo apt install -y gcc-arm-none-eabi gdb-multiarch

  cd OpenIBBOs/tool
  mkdir -p arm-none-eabi-gcc/Linux
  ln -s /usr/bin ./arm-none-eabi-gcc/Linux/bin
  ```


### 3. 配置环境变量
`dp2501project/cmake/arm-none-eabi-gcc.cmake` 通过以下变量定位：
- `SDK_TOOL_PATH`：指向 `OpenIBBOs/tool`
- `SDK_OS_PATH`：指向 `OpenIBBOs`

在项目根 `borui/` 下执行：
```bash
export SDK_TOOL_PATH=$(pwd)/OpenIBBOs/tool
export SDK_OS_PATH=$(pwd)/OpenIBBOs

# 可选：写入 ~/.bashrc 持久化
grep -q 'SDK_TOOL_PATH' ~/.bashrc || echo 'export SDK_TOOL_PATH=$HOME/borui/OpenIBBOs/tool' >> ~/.bashrc
grep -q 'SDK_OS_PATH' ~/.bashrc || echo 'export SDK_OS_PATH=$HOME/borui/OpenIBBOs' >> ~/.bashrc
```

### 4. 使用 CMake Presets 配置与编译
进入工程目录并选择预设（Debug/Release）：
```bash
cd dp2501project

cmake --preset Debug
cmake --build --preset Debug -j
# 或
cmake --preset Release
cmake --build --preset Release -j
```

如需切换工具链或清理重配：
```bash
rm -rf dp2501project/Build
cmake --preset Debug
cmake --build --preset Debug -j
```

### 5. 构建产物与验证
- 产物路径：`dp2501project/Build/<PresetName>/Application/*.elf|*.bin|*.hex`
- 配置日志应出现：
  - `Open IBBOs Basic Tools Directory: ...`
  - `TOOLCHAIN_DIRECTORY : .../arm-none-eabi-gcc/Linux/bin`
  - `arm-none-eabi-gcc cmake done.`

工具链版本自检：
```bash
OpenIBBOs/tool/arm-none-eabi-gcc/Linux/bin/arm-none-eabi-gcc --version
```

### 6. 常见问题
- 找不到 `arm-none-eabi-gcc`：确认 `SDK_TOOL_PATH` 正确且 `Tools/arm-none-eabi-gcc/Linux/bin` 存在并可执行（或符号链接有效）。
- 缺少 Ninja：`sudo apt install -y ninja-build`
- 路径过长/含空格：建议使用 WSL 本地短路径（如 `/home/<user>/borui`）。
 - 下载 404：常因代理或版本号变更，先 `unset https_proxy http_proxy all_proxy`，再尝试新版 xPack 发行包。


