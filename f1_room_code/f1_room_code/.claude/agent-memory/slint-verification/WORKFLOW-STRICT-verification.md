# KR Door Slint 工作流验证报告

- Node 范围: work.md 页面清单 44 个页面 + Home 跳转目标 5hkGo，共 45 个页面
- Slint 入口: `ui/workflow-generated.slint`
- 页面目录: `ui/pages/*.slint`
- 布局报告: `.claude/agent-memory/ui-layout-analyzer/layouts/WORKFLOW-STRICT-20260411-175716.md`
- 最终渲染 PNG: `slint-screenshots/workflow-generated.png`
- 代表性渲染 PNG: `slint-screenshots/79bxG.png`, `slint-screenshots/NmGH2.png`
- 验证状态: PARTIAL

## 执行的 AGENTS.md 工作流

1. 已读取 `work.md`，并按其中页面跳转、三击隐藏入口、倒计时、错误状态补齐 Rust 控制器逻辑。
2. 已读取 `.claude/agent-memory/ui-layout-analyzer/MEMORY.md`、`.claude/agent-memory/slint-ui-generator/MEMORY.md`、`slint-syntax.md`、`design-tokens.md`。
3. 已使用 Pencil MCP:
   - `snapshot_layout` 抽样验证 `9TuZV` / `79bxG` 坐标。
   - `batch_get` 读取 45 个目标页面节点属性。
   - `export_nodes` 以 `scale: 1` 导出 98 个 Icon 候选节点、2 个原始图片填充节点、1 个天气符号节点。
4. 已重新生成 Slint 页面，每个页面独立保存为 `ui/pages/[NodeID].slint`，入口路由保留在 `ui/workflow-generated.slint`。
5. 已删除整页截图依赖；当前 Slint 仅对 Icon/原始图片填充/天气符号使用 `Image`，没有使用 `assets/images/workflow-screens/*`。
6. 已用 `/usr/bin/pencil_to_slint ui/workflow-generated.slint slint-screenshots/workflow-generated.png 800 1280` 渲染最终 PNG。

## 编译/渲染验证

- `slint-compiler -o /dev/null ui/workflow-generated.slint`: PASS
- `cargo check`: PASS
- `/usr/bin/pencil_to_slint ui/workflow-generated.slint slint-screenshots/workflow-generated.png 800 1280`: PASS
- `/usr/bin/pencil_to_slint /tmp/kr-door-render-79bxG.slint slint-screenshots/79bxG.png 800 1280`: PASS
- `/usr/bin/pencil_to_slint /tmp/kr-door-render-NmGH2.slint slint-screenshots/NmGH2.png 800 1280`: PASS

## 交互逻辑检查

- `ui/workflow-generated.slint` 暴露 `current_page`, `node_id`, `status_text`, `hotspot_clicked(string)`, `fallback_tapped()`。
- `ui/pages/*.slint` 每页暴露 `hotspot_clicked(string)` / `fallback_tapped()`；存在 work.md 点击事件的页面添加了对应 `TouchArea`。
- `src/main.rs` 保留 work.md 逻辑:
  - Home: `cTmoY -> YxiHx`, `FNWS4 -> xkLz3`, `nR1q4 -> 3E2uV`, `rJp6p -> 5hkGo`, `J6ysr` 三击进入 `79bxG`。
  - Settings/SystemTest/FaceManager/FaceDataMain/UnitInput 页面跳转已实现。
  - `YxiHx`, `lkLjP`, `5hkGo`, `N9uWU`, `bTkFh`, `9xUZb` 的倒计时逻辑已实现。
  - `3E2uV` 密码失败次数和三次锁定逻辑已实现。

## 视觉验证

当前环境没有 `understand_image` 工具；本报告使用 `view_image` 人工检查和 Pillow 像素差异作为替代。

- Home (`9TuZV`): 与参考 `assets/images/workflow-screens/9TuZV.png` 对比，`RMS=27.32`, `changed_gt15=11.45%`。
- Settings (`79bxG`): 与参考 `assets/images/workflow-screens/79bxG.png` 对比，`RMS=19.98`, `changed_gt15=5.56%`。
- `NmGH2`: 渲染结构、列表间距、箭头 Icon、按钮可见性人工检查通过。

## 修复摘要

- 修复了旧实现使用整页图片作为页面背景的问题。
- 将 Icon 和复杂图形节点改为 Pencil `export_nodes` 导出的 PNG，Slint 中用 `Image` 引用。
- 为 `.pen` 中 auto-layout 但无显式 `x/y` 的节点补算坐标，修复 Icon/文字重叠。
- 为 `.pen` 中缺少 `width/height` 的少数节点补齐尺寸，修复 `79bxG` 底部卡片拉伸、`N9uWU`/`gqX6Q` 文案容器重叠问题。
- 将天气符号 `3UcMH` 改为 Pencil 导出的 PNG，避免 Slint emoji 字体渲染与设计稿不一致。

## 结论

PARTIAL: 代码结构、编译、渲染、交互入口和 Icon 资源工作流已经按 AGENTS.md 修正；代表页面视觉已明显接近设计稿。但由于仍存在字体/抗锯齿/少量像素级差异，当前不声明像素级 PASS。

## 命名更新

- 已按 Pencil MCP 返回的 `name` 字段重命名页面文件和 Slint 组件。
- 重复 Name 使用数字后缀去重，例如 `FaceRecognition.slint` / `FaceRecognition2.slint`、`PasswordError.slint` / `PasswordError2.slint` / `PasswordError3.slint`。
- 运行时 `current_page` 仍保留 Node ID，因为 `work.md` 和 Rust 跳转逻辑以 Node ID 为稳定状态键。
- 重命名后已重新执行 `slint-compiler -o /dev/null ui/workflow-generated.slint`、`cargo check` 和 `/usr/bin/pencil_to_slint ui/workflow-generated.slint slint-screenshots/workflow-generated.png 800 1280`，均通过。
