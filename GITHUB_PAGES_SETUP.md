# GitHub Pages 设置指南

如果看到 404 页面，请按照以下步骤设置 GitHub Pages：

## 步骤 1: 启用 GitHub Pages

1. 访问仓库设置页面：
   ```
   https://github.com/ibb-net/ZeroTopic/settings/pages
   ```

2. 在 "Source" 部分：
   - 选择 **"GitHub Actions"** 作为源
   - 不要选择 "Deploy from a branch"

3. 点击 **"Save"** 保存设置

## 步骤 2: 检查 GitHub Actions 状态

1. 访问 Actions 页面：
   ```
   https://github.com/ibb-net/ZeroTopic/actions
   ```

2. 查看 "Deploy Docs" 工作流：
   - 如果显示 ✅ 绿色，说明构建成功
   - 如果显示 ❌ 红色，点击查看错误日志

3. 如果工作流没有自动运行，可以：
   - 手动触发：点击 "Run workflow"
   - 或者推送一个小的更改（如更新 README）

## 步骤 3: 等待部署完成

- 首次部署通常需要 2-5 分钟
- 后续更新部署需要 1-3 分钟

## 步骤 4: 访问文档网站

部署成功后，访问：
```
https://ibb-net.github.io/ZeroTopic/
```

## 故障排除

### 问题 1: Actions 显示失败

**解决方案**：
1. 点击失败的 workflow 查看日志
2. 检查构建错误信息
3. 修复错误后重新推送

### 问题 2: 404 页面

**可能原因**：
- GitHub Pages 未启用
- 部署未完成（等待几分钟）
- site_url 配置错误

**解决方案**：
1. 确认已按照步骤 1 启用 GitHub Pages
2. 检查 `mkdocs.yml` 中的 `site_url` 是否正确：
   ```yaml
   site_url: https://ibb-net.github.io/ZeroTopic
   ```
3. 查看 Actions 是否有成功的部署记录

### 问题 3: 页面显示但不更新

**解决方案**：
1. 清除浏览器缓存
2. 等待 GitHub Pages 缓存更新（最多 10 分钟）
3. 强制刷新：`Ctrl+F5` 或 `Cmd+Shift+R`

## 验证清单

- [ ] GitHub Pages 已启用（Settings → Pages → Source: GitHub Actions）
- [ ] GitHub Actions 工作流成功运行
- [ ] `mkdocs.yml` 中的 `site_url` 正确
- [ ] 等待至少 2-5 分钟让首次部署完成

## 相关链接

- [GitHub Pages 文档](https://docs.github.com/en/pages)
- [MkDocs gh-deploy 文档](https://www.mkdocs.org/user-guide/deploying-your-docs/#github-pages)
- [仓库 Actions](https://github.com/ibb-net/ZeroTopic/actions)

---

**提示**：如果按照以上步骤仍然无法访问，请检查：
1. 仓库是否为公开仓库（私有仓库需要 GitHub Pro）
2. 是否有足够的 GitHub Actions 配额
3. 是否有正确的权限设置
