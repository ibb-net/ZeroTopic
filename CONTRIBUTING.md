# 贡献指南

感谢您对 ZeroTopic 项目的关注！我们欢迎所有形式的贡献。

## 📋 目录

- [行为准则](#行为准则)
- [如何贡献](#如何贡献)
  - [报告问题](#报告问题)
  - [提交功能请求](#提交功能请求)
  - [提交代码](#提交代码)
- [开发环境设置](#开发环境设置)
- [代码规范](#代码规范)
- [提交规范](#提交规范)

## 行为准则

本项目遵循贡献者行为准则。参与项目时，请保持尊重和包容。

## 如何贡献

### 报告问题

如果您发现了一个 bug，请：

1. **检查现有问题**：在提交新问题前，请先查看 [Issues](https://github.com/yourusername/ZeroTopic/issues) 确认问题未被报告
2. **创建新 Issue**：使用 [Bug Report 模板](.github/ISSUE_TEMPLATE/bug_report.md)
3. **提供详细信息**：
   - 问题描述
   - 复现步骤
   - 预期行为
   - 实际行为
   - 环境信息（OS、编译器版本等）
   - 相关代码片段

### 提交功能请求

如果您有新的功能想法：

1. **检查现有功能请求**：确认功能未被提出
2. **创建新 Issue**：使用 [Feature Request 模板](.github/ISSUE_TEMPLATE/feature_request.md)
3. **详细描述**：
   - 功能描述
   - 使用场景
   - 预期效果
   - 可能的实现方案

### 提交代码

#### 1. Fork 项目

点击 GitHub 页面上的 "Fork" 按钮，创建您自己的项目副本。

#### 2. 克隆仓库

```bash
git clone https://github.com/yourusername/ZeroTopic.git
cd ZeroTopic
```

#### 3. 创建分支

```bash
git checkout -b feature/your-feature-name
# 或
git checkout -b fix/your-bug-fix
```

#### 4. 进行修改

- 遵循代码规范
- 添加必要的注释
- 更新相关文档
- 添加测试用例（如适用）

#### 5. 提交更改

```bash
git add .
git commit -m "feat: add your feature description"
```

遵循 [提交规范](#提交规范)。

#### 6. 推送到 Fork

```bash
git push origin feature/your-feature-name
```

#### 7. 创建 Pull Request

1. 在 GitHub 上打开您的 Fork
2. 点击 "New Pull Request"
3. 选择您的分支
4. 填写 PR 描述，使用 [PR 模板](.github/pull_request_template.md)
5. 等待代码审查

## 开发环境设置

### 环境要求

- **编译器**：GCC 11.3.1 或更高版本（ARM Cortex-M）
- **构建系统**：CMake 3.20 或更高版本
- **调试工具**：OpenOCD（可选）

### 构建步骤

```bash
mkdir build
cd build
cmake ..
make
```

### 运行测试

```bash
make test
```

## 代码规范

### C 代码风格

- **缩进**：使用 4 个空格
- **命名**：使用下划线命名（snake_case）
- **函数命名**：`模块名_功能名`，如 `topic_bus_init`
- **类型命名**：`类型名_t`，如 `topic_bus_t`
- **常量命名**：全大写，如 `MAX_TOPICS`

### 示例代码

```c
/*
 * @brief 初始化 Topic 总线
 * @param bus Topic 总线指针
 * @param topics Topic 条目数组
 * @param max_topics 最大 Topic 数量
 * @param dict 关联的对象字典
 * @return 0 成功，-1 失败
 */
int topic_bus_init(topic_bus_t* bus, topic_entry_t* topics, 
                   size_t max_topics, obj_dict_t* dict) {
    if (!bus || !topics || max_topics == 0 || !dict) {
        return -1;
    }
    // 实现...
    return 0;
}
```

### 文档注释

- 使用 Doxygen 风格注释
- 为所有公共 API 添加文档
- 说明参数、返回值、错误情况

## 提交规范

我们使用 [Conventional Commits](https://www.conventionalcommits.org/) 规范：

### 提交类型

- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `style`: 代码格式调整（不影响功能）
- `refactor`: 代码重构
- `test`: 测试相关
- `chore`: 构建过程或辅助工具的变动

### 提交格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### 示例

```
feat(topic_bus): add timeout support for AND rules

Add event timeout checking mechanism for AND rules to ensure
data freshness. Each event can have its own timeout configuration.

Closes #123
```

## 代码审查流程

1. **自动化检查**：PR 提交后会自动运行 CI 检查
2. **代码审查**：维护者会审查代码
3. **反馈和修改**：根据反馈进行修改
4. **合并**：审查通过后合并到主分支

## 问题反馈

如果您在贡献过程中遇到问题，可以：

- 在 Issues 中提问
- 查看现有文档
- 联系维护者

感谢您的贡献！🎉

