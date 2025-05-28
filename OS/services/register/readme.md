# RegisterDemoLib 使用说明

## 原理介绍

`RegisterDemoLib` 是一个基于宏定义的注册表实现，用于将命令或数据项注册到一个固定的内存区域中，便于后续的统一管理和访问。其核心原理如下：

1. **宏定义注册**：
   - 使用 `REGISTER_EXPORT_CMD` 宏将命令或数据项注册到一个特定的内存段（`RegisterMap`）。
   - 宏会生成一个结构体实例，并将其放置到指定的内存段中。

2. **内存段管理**：
   - 不同编译器通过不同的方式定义和访问内存段：
     - ARMCC 使用 `RegisterMap$$Base` 和 `RegisterMap$$Limit`。
     - IAR 使用 `__section_begin` 和 `__section_end`。
     - GCC 使用 `_RegisterMapStart` 和 `_RegisterMapEnd`。

3. **运行时初始化**：
   - 在运行时，通过 `RegisterInit` 函数读取内存段的起始地址和大小，构建一个 `registerStruct` 对象，便于后续遍历和访问。

## 移植步骤

如果需要将 `RegisterDemoLib` 移植到其他项目或平台，请按照以下步骤操作：

### 1. 确认编译器支持
确保目标平台的编译器支持以下特性：
- 自定义内存段（如 `section` 或 `pragma`）。
- `__attribute__((used))` 或类似功能，确保编译器不会优化掉未直接引用的变量。

### 2. 定义内存段
根据目标编译器的特性，修改 `register.h` 中的 `REGISTER_SECTION` 和 `REGISTER_MAP_USED` 宏。例如：
- 对于新的编译器，添加对应的 `#elif` 分支，定义内存段和对齐方式。

### 3. 修改链接脚本
在链接脚本中定义 `RegisterMap` 段。例如：
- 对于 GCC，添加以下内容：
  ```ld
  .RegisterMap :
  {
      _RegisterMapStart = .;
      *(.RegisterMap)
      _RegisterMapEnd = .;
  } > RAM
  ```

### 4. 调整初始化代码
在 `register.c` 中的 `RegisterInit` 函数中，添加对新编译器的支持。例如：
```c
#elif defined(__NEW_COMPILER__)
    registerMap.base  = (registerStruct *)(__new_compiler_section_begin("RegisterMap"));
    registerMap.count = ((size_t)(__new_compiler_section_end("RegisterMap")) - (size_t)(__new_compiler_section_begin("RegisterMap"))) / sizeof(registerStruct);
```

### 5. 测试功能
- 使用 `REGISTER_EXPORT_CMD` 宏注册一些测试命令。
- 调用 `RegisterInit` 函数，打印注册表内容，确保功能正常。

## 示例代码

以下是一个简单的示例：
```c
REGISTER_EXPORT_CMD(test1, "This is test command 1");
REGISTER_EXPORT_CMD(test2, "This is test command 2");

void main(void) {
    RegisterInit();
    // 输出注册表内容
}
```

## 注意事项
- 确保链接脚本和代码中的内存段名称一致。
- 如果目标平台不支持自定义内存段，可以考虑使用命令表模式（通过数组实现）。
