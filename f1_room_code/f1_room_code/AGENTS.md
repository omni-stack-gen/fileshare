# AGENTS.md

本文件定义本项目中从 Pencil 设计节点生成 Slint UI 的多子 agent 工作流。处理任何“根据 Node ID / 布局报告生成 Slint 界面 / 验证并修复渲染”的任务时，优先按本文流程执行。

## 项目上下文

- Pencil 设计文件通常为当前打开的 `.pen` 文件，或项目根目录下的 `door_ui_2.pen` / `door.pen`。
- 布局报告输出目录：`.claude/agent-memory/ui-layout-analyzer/layouts/`
- Slint 输出目录：`ui/`
- 图标导出目录：`assets/images/`
- 渲染截图目录：`slint-screenshots/`
- 验证报告目录：`.claude/agent-memory/slint-verification/`
- 交互逻辑文档：`work.md`

## 交互逻辑文档

生成 Slint 代码前必须读取项目根目录的 `work.md`，用于了解界面交互逻辑、页面跳转、按钮行为、状态变化、倒计时、错误处理、输入流程等非视觉信息。

处理规则：

1. `slint-ui-generator` 在读取布局报告后，必须读取 `work.md`。
2. 如果 `work.md` 包含与当前 Node ID 或界面名称相关的交互逻辑，生成 Slint 时必须携带这些逻辑。
3. 交互逻辑应体现在 Slint 代码中，例如：
   - `callback` 声明
   - `in` / `in-out` 属性
   - 状态属性和条件渲染
   - 倒计时、错误状态、禁用状态等 UI 状态表达
   - 按钮点击后的回调接口或跳转占位
4. 如果 `work.md` 未提供当前界面的交互逻辑，必须在生成结果或验证报告中说明“未发现对应交互逻辑”，不得臆造业务行为。
5. `slint-verifier` 验证时必须检查生成的 Slint 是否保留了 `work.md` 中要求的交互入口和状态表达；视觉验证之外，还要标注交互逻辑是否缺失。

## 子 Agent 职责

### ui-layout-analyzer

来源定义：`.claude/agents/ui-layout-analyzer.md`

用于从 Pencil 节点提取完整 UI 信息并生成布局分析报告。

必须执行：

1. 读取 `.claude/agent-memory/ui-layout-analyzer/MEMORY.md`。
2. 使用 Pencil MCP 捕获目标 Node ID 或当前选中节点。
3. 使用 `snapshot_layout` 获取坐标，建议 `maxDepth=5` 或按复杂度加深。
4. 使用 `batch_get` 获取节点属性，建议 `readDepth=4` 或按复杂度加深。
5. 输出完整 Visual Hierarchy Tree，必须严格匹配 Pencil 原始 `children` 层级。
6. 输出完整坐标表，覆盖所有元素，包含 Node ID、Name、Type、相对 X/Y、宽高、文本内容。
7. 输出详细属性表，包含圆角、填充/渐变、透明度、描边、阴影、clip、字体、颜色等。
8. 输出 Color Palette、Typography Scale 和 Layout Observations。
9. 对 Icon、裁剪、重叠、越界等问题做明确标注。
10. 保存报告到 `.claude/agent-memory/ui-layout-analyzer/layouts/[NodeID]-[YYYYMMDD-HHmmss].md`。
11. 返回报告文件路径。

质量要求：

- 不得伪造 Pencil 未提供的数据。
- 根节点的画布偏移只用于报告记录，生成 Slint 时通常不应复制为窗口内偏移。
- 坐标表使用相对父节点坐标；根节点记录画布绝对坐标。
- 中文请求默认生成中文报告。

## slint-ui-generator

来源定义：`.claude/agents/slint-ui-generator.md`

用于把 `ui-layout-analyzer` 生成的布局报告转换为 Slint 代码。

必须执行：

1. 读取 `.claude/agent-memory/slint-ui-generator/MEMORY.md`。
2. 读取 `.claude/agent-memory/slint-ui-generator/slint-syntax.md` 和 `design-tokens.md`。
3. 读取项目根目录 `work.md`，提取与当前 Node ID / 界面名称相关的交互逻辑。
4. 解析布局报告中的层级、坐标、尺寸、颜色、渐变、圆角、阴影、clip、字体等。
5. 在 `ui/` 下生成独立 `.slint` 文件，建议命名为 `ui/[界面名称].slint`。
6. 保留报告中的父子结构；透明容器、clip 容器、背景层、阴影层、布局 spacer 不得随意删除。
7. 将 `work.md` 中匹配到的交互逻辑带入 Slint：至少保留 callback、状态属性、输入输出属性或 TODO 注释，确保业务集成点清晰。
8. 对所有 `Name = Icon` 或 `Type = Icon` 的节点，必须使用 Pencil MCP `export_nodes` 以 `scale: 1` 导出为 PNG 到 `assets/images/`。
9. Slint 中必须用 `Image { source: @image-url("../assets/images/<filename>.png"); }` 引用导出的 Icon。
10. 禁止用 emoji、纯色 Rectangle、手写 Path 替代 Icon。
11. 生成后运行 `slint-compiler -o /dev/null ui/[file].slint`，确保 0 errors。
12. 返回 Slint 文件路径、编译结果、导出的图片文件清单、已携带的交互逻辑摘要。

Slint 生成约束：

- 坐标和尺寸优先严格匹配报告；原设计是绝对定位时使用 `x/y/width/height`。
- 原设计使用 flex/flow 布局时可使用 `HorizontalLayout` / `VerticalLayout`，但必须保持视觉结果一致。
- 图片路径相对 `.slint` 文件位置，项目中应使用 `../assets/images/...`。
- 使用 `border-radius`，不要使用 `corner-radius`。
- 使用 `drop-shadow-*`，不要使用 CSS 式 `shadow`。
- 不使用 `margin`，间距用父容器 padding/spacing 或绝对坐标表达。
- 颜色必须使用报告中的精确 hex 值，包含 alpha 时不要随意改写。

## slint-verifier

来源定义：`.claude/agents/slint-verifier.md`

用于渲染 Slint、分析渲染结果、对比设计报告并自动修正问题。

必须执行：

1. 读取 `.claude/agent-memory/slint-ui-generator/MEMORY.md`。
2. 读取待验证的布局报告和 Slint 文件。
3. 读取 `work.md`，确认当前界面要求的交互逻辑是否已体现在 Slint 中。
4. 确认 `/usr/bin/pencil_to_slint` 可用。
5. 运行：

   ```bash
   /usr/bin/pencil_to_slint [SLINT_FILE_PATH] [OUTPUT_PNG_PATH] [width] [height]
   ```

6. 分析渲染 PNG，关注文本位置、文本是否截断、Icon 是否可见、容器颜色/渐变/圆角、布局偏移、缺失元素。
7. 和设计报告对比，默认阈值：
   - 位置偏差：≤ 15px 通过
   - 字体大小：±1px 通过
   - 颜色：要求精确匹配
   - 文本截断：零容忍
8. 如果 `work.md` 中的交互入口、状态或回调缺失，直接修正 Slint 文件并重新编译。
9. 如有视觉问题，直接修正 Slint 文件，并重新渲染验证，最多 3 轮。
10. 保存验证报告到 `.claude/agent-memory/slint-verification/[NodeID]-verification.md`。
11. 返回最终 Slint 文件路径、验证报告路径、最终 PNG 路径、修复摘要、交互逻辑检查结果和 PASS/PARTIAL/FAIL 状态。

如果当前运行环境没有 `understand_image` 工具，必须在验证报告中说明限制，并使用可用的截图查看/图像检查方式进行替代验证。

## 端到端工作流

当用户提供 Node ID 并要求生成 Slint 界面时，按以下顺序执行：

1. `ui-layout-analyzer` 分析界面。
   - 输入：Node ID 或“当前选中节点”
   - 输出：布局分析报告路径

2. `slint-ui-generator` 生成代码。
   - 输入：布局分析报告路径、`work.md` 交互逻辑
   - 输出：`ui/[node-id-lowercase].slint`、导出的 Icon PNG 清单、编译结果、交互逻辑摘要

3. `slint-verifier` 验证并修正。
   - 输入：Node ID、Slint 文件路径、布局报告路径、`work.md` 交互逻辑、渲染尺寸
   - 输出：最终 Slint 文件、验证报告、最终渲染截图、修复摘要、交互逻辑检查结果

4. 汇总交付。
   - 报告最终 Slint 路径
   - 报告布局报告路径
   - 报告验证报告路径
   - 报告导出的 Icon 资源
   - 报告编译/渲染验证结果

## 工作流状态记录

如项目脚本可用，记录各阶段产物：

```bash
python3 .claude/agent-memory/scripts/workflow-state.py artifact layoutReport [REPORT_PATH]
python3 .claude/agent-memory/scripts/workflow-state.py artifact slintFile [SLINT_PATH]
python3 .claude/agent-memory/scripts/workflow-state.py artifact verificationReport [REPORT_PATH]
```

注意：以实际脚本支持的命令为准。如果 `status`、`phase` 或 `complete` 子命令不可用，不要阻塞主流程；继续完成报告、代码生成、编译和验证，并在最终回复中说明。

## 交付检查清单

交付前必须确认：

- [ ] 布局报告已保存到 `.claude/agent-memory/ui-layout-analyzer/layouts/`
- [ ] 生成代码前已读取 `work.md`
- [ ] `work.md` 中与当前界面匹配的交互逻辑已携带到 Slint 代码
- [ ] 如 `work.md` 没有对应逻辑，已在结果中说明
- [ ] Slint 文件已保存到 `ui/`
- [ ] 所有 Icon 节点已通过 Pencil 导出到 `assets/images/`
- [ ] Slint 中所有 Icon 均使用 `Image` 引用 PNG
- [ ] Slint 中没有 emoji Icon 占位、纯色块 Icon 占位、手写 Path Icon
- [ ] `slint-compiler -o /dev/null ui/[file].slint` 为 0 errors
- [ ] 已用 `/usr/bin/pencil_to_slint` 渲染最终 PNG
- [ ] 已验证 Slint 中保留了 `work.md` 要求的 callback、状态属性或业务集成点
- [ ] 验证报告已保存到 `.claude/agent-memory/slint-verification/`
- [ ] 如发现渲染问题，已修复并重新验证
