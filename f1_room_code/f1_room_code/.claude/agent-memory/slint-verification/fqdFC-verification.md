# fqdFC / Container2 弹窗验证报告

## 输入
- Pencil Node ID: fqdFC
- Pencil Name: Home
- 目标 Slint: ui/pages/Container2.slint + ui/workflow-generated.slint
- 布局报告: .claude/agent-memory/ui-layout-analyzer/layouts/fqdFC-20260413-110558.md
- 渲染 PNG: slint-screenshots/Container2-popup.png

## work.md 交互检查
- work.md 未发现 fqdFC 专属交互逻辑。
- work.md 中 Home 的 rJp6p 原逻辑为进入 5hkGo/Container2。
- 当前实现保留 rJp6p -> Container2。
- Container2 弹窗新增两个业务入口：household_password、common_password。
- 因 work.md 未定义两者的目标页，当前两者均接入已有密码输入页 Container，便于完成“密码进入模式”选择后的流程。
- 点击弹窗外或 fallback 返回 Home。

## 视觉验证
- 使用命令：/usr/bin/pencil_to_slint /tmp/kr-door-container2-render.slint /home/dpower/data/KR_DOOR/slint-screenshots/Container2-popup.png 800 1280
- 渲染尺寸：800x1280。
- 验证方式：使用可用的截图查看工具进行人工视觉检查；当前环境没有 understand_image 专用工具。
- 结果：Container2 已作为透明叠层显示在 Home 背景上，弹窗位置约为 x=81, y=415, size=637x449。
- 弹窗标题、两枚按钮、颜色、圆角和叠层关系与 fqdFC 截图匹配。
- fqdFC 弹窗内部未发现 Icon 节点，因此未新增 Icon 导出。

## 编译验证
- slint-compiler -o /dev/null ui/workflow-generated.slint: PASS
- cargo check: PASS

## 修复摘要
- 将 Container2 从独立深色全屏页改为透明弹窗层。
- workflow 在 current_page == "Container2" 时同时渲染 Page_Home 和 Page_Container2。
- 移除 Container2 作为二维码扫描页的自动定时逻辑。
- 新增两个弹窗按钮热点并接入密码输入页。

## 状态
PASS
