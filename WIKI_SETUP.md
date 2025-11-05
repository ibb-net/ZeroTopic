# Wiki 文档集成说明

本文档说明如何在代码仓库中集成 Wiki 文档系统。

## ✨ 已完成的配置

### 1. MkDocs 配置文件

- ✅ `mkdocs.yml` - 主配置文件
- ✅ `requirements-docs.txt` - Python 依赖包

### 2. 文档结构

- ✅ `docs/index.md` - Wiki 首页
- ✅ `docs/getting-started.md` - 快速开始指南
- ✅ `docs/installation.md` - 安装指南
- ✅ `docs/README.md` - 文档使用说明

### 3. 自动化部署

- ✅ `.github/workflows/docs.yml` - GitHub Actions 自动部署工作流

## 🚀 快速开始

### 步骤 1: 安装依赖

```bash
cd OpenIBBOs
pip install -r requirements-docs.txt
```

### 步骤 2: 本地预览

```bash
mkdocs serve
```

访问 http://127.0.0.1:8000 查看文档。

### 步骤 3: 启用 GitHub Pages（首次使用）

1. 进入 GitHub 仓库设置
2. 找到 "Pages" 设置
3. 选择 Source: "GitHub Actions"
4. 保存设置

### 步骤 4: 推送代码

```bash
git add .
git commit -m "添加 Wiki 文档系统"
git push
```

推送后，GitHub Actions 会自动构建并部署文档。

## 📝 配置自定义

### 修改站点信息

编辑 `mkdocs.yml`：

```yaml
site_name: OpenIBBOs Wiki
site_url: https://yourusername.github.io/OpenIBBOs  # 修改为您的仓库地址
```

### 添加文档到导航

编辑 `mkdocs.yml` 的 `nav` 部分：

```yaml
nav:
  - 首页: index.md
  - 新章节:
    - 新文档: path/to/doc.md
```

### 自定义主题

Material 主题支持丰富的自定义选项，参考：
https://squidfunk.github.io/mkdocs-material/getting-started/

## 📚 文档组织建议

### 目录结构

```
docs/
├── index.md                    # 首页
├── getting-started.md          # 快速开始
├── installation.md             # 安装指南
├── api/                        # API 文档
│   ├── topic-bus.md
│   └── obj-dict.md
├── guides/                     # 使用指南
│   ├── basic-usage.md
│   └── advanced.md
└── images/                     # 图片资源
    └── ...
```

### 引用现有文档

项目中的现有 Markdown 文档可以直接引用：

```yaml
nav:
  - 核心概念:
    - Topic 总线: zero_topic_core/topic_bus/topic_bus.md
    - 对象字典: zero_topic_core/obj_dict/obj_dict.md
```

## 🔧 高级功能

### 版本控制

为文档添加版本标签：

```yaml
theme:
  version:
    provider: mike  # 需要安装 mkdocs-mike
```

### 多语言支持

Material 主题支持多语言，配置示例：

```yaml
theme:
  language: zh
  alternate:
    - name: English
      link: /en/
      lang: en
```

### 自定义域名

1. 在 GitHub Pages 设置中配置自定义域名
2. 更新 `mkdocs.yml`：

```yaml
site_url: https://docs.yourdomain.com
```

## 🎨 主题定制

### 修改颜色

编辑 `mkdocs.yml`：

```yaml
theme:
  palette:
    - scheme: default
      primary: blue      # 修改主色调
      accent: blue       # 修改强调色
```

### 添加 Logo

```yaml
theme:
  logo: images/logo.png
```

### 自定义 CSS

创建 `docs/stylesheets/extra.css`，MkDocs 会自动加载。

## 📊 文档统计

查看文档统计信息：

```bash
mkdocs build --verbose
```

## 🐛 常见问题

### Q: 本地预览正常，但部署后页面空白？

A: 检查 `site_url` 配置是否正确，确保与 GitHub Pages 地址匹配。

### Q: 如何添加搜索功能？

A: 搜索功能已默认启用，使用 `search` 插件即可。

### Q: 如何添加数学公式？

A: 启用 MathJax 插件：

```yaml
markdown_extensions:
  - pymdownx.arithmatex:
      generic: true
extra_javascript:
  - javascripts/mathjax.js
  - https://polyfill.io/v3/polyfill.min.js?features=es6
  - https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js
```

### Q: 如何添加评论功能？

A: 可以使用 Giscus 或 Utterances，参考 Material 文档。

## 📖 参考资源

- [MkDocs 官方文档](https://www.mkdocs.org/)
- [Material for MkDocs](https://squidfunk.github.io/mkdocs-material/)
- [GitHub Pages 文档](https://docs.github.com/en/pages)

## ✅ 检查清单

完成以下检查确保 Wiki 正常工作：

- [ ] `mkdocs.yml` 配置正确
- [ ] 所有文档路径正确
- [ ] GitHub Pages 已启用
- [ ] GitHub Actions 工作流正常运行
- [ ] 本地预览正常
- [ ] 部署后网站可访问
- [ ] 搜索功能正常
- [ ] 所有链接有效

---

**需要帮助？** 请查看文档或提交 Issue。
