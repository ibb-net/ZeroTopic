# GitHub Pages 设置指南

如果看到 404 页面，请按照以下步骤设置 GitHub Pages：

## 两种部署方式

### 方式 1: GitHub Actions 部署（推荐，已配置）

当前工作流使用标准的 GitHub Actions Pages 部署方式，需要：
1. Settings → Pages → Source 选择 **"GitHub Actions"**
2. 工作流会自动部署到 `gh-pages` 分支

### 方式 2: 从 gh-pages 分支部署（如果方式1不工作）

如果使用 `mkdocs gh-deploy`，需要：
1. Settings → Pages → Source 选择 **"Deploy from a branch"**
2. 选择 `gh-pages` 分支
3. 选择 `/ (root)` 目录
