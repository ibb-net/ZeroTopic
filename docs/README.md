# Wiki 文档使用指南

本文档说明如何在代码仓库中使用和维护 Wiki 文档系统。

## 📖 概述

本项目使用 [MkDocs](https://www.mkdocs.org/) + [Material Theme](https://squidfunk.github.io/mkdocs-material/) 构建文档站点，支持：

- ✅ 基于 Markdown 编写文档
- ✅ 自动生成美观的网站
- ✅ 支持搜索功能
- ✅ 响应式设计，支持移动端
- ✅ 自动部署到 GitHub Pages

## 🚀 快速开始

### 本地预览文档

1. **安装依赖**

```bash
pip install -r requirements-docs.txt
```

2. **启动开发服务器**

```bash
mkdocs serve
```

3. **访问文档**

打开浏览器访问 http://127.0.0.1:8000

### 构建静态站点

```bash
mkdocs build
```

生成的静态文件在 `site/` 目录中。

## 📝 编写文档

### 文档目录结构

```
docs/
├── index.md              # 首页
├── getting-started.md    # 快速开始
├── installation.md       # 安装指南
└── ...                   # 其他文档
```

### 添加新文档

1. **创建 Markdown 文件**

在 `docs/` 目录下创建新的 `.md` 文件，例如：

```bash
docs/new-feature.md
```

2. **更新导航**

编辑 `mkdocs.yml` 文件，在 `nav` 部分添加新文档：

```yaml
nav:
  - 首页: index.md
  - 新功能: new-feature.md  # 添加这一行
```

3. **编写内容**

使用 Markdown 语法编写文档内容。

### 引用现有文档

如果文档在其他目录（如 `zero_topic_core/topic_bus/topic_bus.md`），直接在导航中引用：

```yaml
nav:
  - 核心概念:
    - Topic 总线: zero_topic_core/topic_bus/topic_bus.md
```

### Markdown 扩展功能

MkDocs Material 支持丰富的 Markdown 扩展：

#### 代码块

````markdown
```c
#include <stdio.h>
int main() {
    return 0;
}
```
````

#### 提示框

```markdown
!!! note "提示"
    这是一个提示信息。

!!! warning "警告"
    这是一个警告信息。

!!! danger "危险"
    这是一个危险提示。
```

#### 标签页

````markdown
=== "C"

    ```c
    printf("Hello");
    ```

=== "Python"

    ```python
    print("Hello")
    ```
````

#### 折叠内容

```markdown
??? note "点击展开"
    这是折叠的内容。
```

## 🔧 配置说明

主要配置文件是 `mkdocs.yml`，包含：

- **站点信息**: 名称、描述、作者等
- **主题配置**: Material 主题设置
- **导航结构**: 文档导航菜单
- **插件配置**: 搜索、Git 日期等插件

详细配置说明请参考 [MkDocs 文档](https://www.mkdocs.org/user-guide/configuration/)。

## 🌐 部署文档

### 自动部署（推荐）

文档会自动部署到 GitHub Pages：

1. 推送代码到 `main` 或 `master` 分支
2. GitHub Actions 会自动构建和部署
3. 访问 `https://yourusername.github.io/OpenIBBOs` 查看文档

### 手动部署

```bash
mkdocs gh-deploy
```

### 自定义域名

1. 在仓库设置中配置 GitHub Pages 自定义域名
2. 更新 `mkdocs.yml` 中的 `site_url`：

```yaml
site_url: https://docs.yourdomain.com
```

## 📋 文档规范

### 文件命名

- 使用小写字母和连字符：`getting-started.md`
- 避免使用空格和特殊字符

### 文档结构

每个文档应包含：

```markdown
# 标题

简要介绍

## 章节1

内容...

## 章节2

内容...

## 相关链接

- [链接1](url1)
- [链接2](url2)
```

### 代码示例

- 使用语法高亮
- 提供完整的可运行示例
- 添加必要的注释

### 图片和资源

将图片放在 `docs/images/` 目录，使用相对路径引用：

```markdown
![图片描述](images/example.png)
```

## 🔍 搜索功能

文档站点内置全文搜索功能：

- 支持中文搜索
- 搜索标题和内容
- 自动高亮匹配结果

## 🐛 故障排除

### 本地预览无法访问

- 检查端口是否被占用
- 尝试使用 `mkdocs serve -a 0.0.0.0:8000`

### 构建错误

- 检查 `mkdocs.yml` 语法
- 确认所有引用的文件存在
- 查看错误日志定位问题

### 部署失败

- 检查 GitHub Actions 日志
- 确认 GitHub Pages 已启用
- 验证仓库权限设置

## 📚 更多资源

- [MkDocs 官方文档](https://www.mkdocs.org/)
- [Material for MkDocs](https://squidfunk.github.io/mkdocs-material/)
- [Markdown 语法指南](https://www.markdownguide.org/)

## 💡 最佳实践

1. **定期更新**: 保持文档与代码同步
2. **清晰结构**: 使用合理的章节层次
3. **示例代码**: 提供可运行的代码示例
4. **链接检查**: 定期检查内部和外部链接
5. **反馈收集**: 收集用户反馈持续改进

---

**需要帮助？** 请查看 [问题反馈](https://github.com/yourusername/OpenIBBOs/issues) 或提交 PR。
