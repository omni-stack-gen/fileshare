# lUC51 DataBackup 弹窗验证报告

- Node ID: `lUC51`
- Name: `DataBackup2`
- Slint 文件: `ui/pages/DataBackup.slint`
- 布局报告: `.claude/agent-memory/ui-layout-analyzer/layouts/lUC51-20260413-DataBackup2.md`
- 默认截图: `slint-screenshots/DataBackup-popup.png`
- 弹窗截图: `slint-screenshots/DataBackup-popup-open.png`

## work.md 交互检查

`work.md` 未发现 `lUC51` / `DataBackup2` 专项逻辑。按用户需求实现为：点击 DataBackup 页面中的 `备份 / 恢复 / 同步` 显示确认弹窗；弹窗 `否 / 是` 关闭弹窗，作为后续业务集成点。

## 实现检查

- `dialog_kind = 0`: 不显示弹窗。
- `dialog_kind = 1`: 备份确认弹窗，正文匹配设计稿 `是否把本地数据库备份到SD卡中？`。
- `dialog_kind = 2`: 恢复确认弹窗。
- `dialog_kind = 3`: 同步确认弹窗。
- 遮罩颜色使用 `#0000004f`。
- 弹窗尺寸/位置使用 lUC51 报告：x=141, y=516, w=517, h=249。
- 弹窗按钮 `否 / 是` 均关闭弹窗；未臆造真实备份/恢复/同步业务逻辑。

## 编译/渲染

- `slint-compiler -o /dev/null ui/workflow-generated.slint`: PASS
- `cargo check`: PASS
- `/usr/bin/pencil_to_slint ui/pages/DataBackup.slint slint-screenshots/DataBackup-popup.png 800 1280`: PASS
- 使用临时同目录 Slint 文件渲染打开态 `DataBackup-popup-open.png`: PASS

## 结论

PASS。当前环境没有 `understand_image` 工具；已使用 Pencil MCP 截图和 `pencil_to_slint` 输出截图做替代验证。
