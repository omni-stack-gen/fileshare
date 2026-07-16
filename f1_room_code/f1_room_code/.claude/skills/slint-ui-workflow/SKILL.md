---
name: slint-ui-workflow
description: 调度子代理完成从 Pencil 设计到 Slint UI 的完整工作流程
trigger: "when user wants to create Slint UI from pencil design"
version: 2.2.0
---

# Slint UI 工作流程

> 收到需求，主代理的职责是**调度资源、拉通流程、验收结果**，而非亲力亲为分析界面或生成代码。所有与 UI 分析、Slint 生成相关的专业工作，必须**全权下放**给对应的子代理。

## 核心原则

1. **主代理不做任何界面分析**：不读取 `.pen` 文件，不解析节点坐标，不判断层级关系。
2. **子代理必须读自己的 memory**：启动 `ui-layout-analyzer`、`slint-ui-generator` 和 `slint-verifier` 前，确保它们会读取各自在 `.claude/agent-memory/` 下的 MEMORY.md 和沉淀的规则文件。
3. **严格按 agent 自带 SOP 执行**：不在主代理层重复描述细粒度步骤，而是通过 prompt 强制子代理遵循其内部定义的工作流。
4. **结果导向**：每阶段必须拿到子代理的交付物（文件路径 / 编译结果），确认无误后再进入下一阶段。
5. **状态追踪**：使用 `workflow-state.json` 追踪任务进度，支持断点续作。
6. **交互逻辑随代码生成**：生成 Slint 前必须读取项目根目录 `work.md`，提取与当前 Node ID / 界面名称相关的交互逻辑，并要求 `slint-ui-generator` 将这些逻辑体现在生成代码中；若 `work.md` 无对应逻辑，必须明确记录“未发现对应交互逻辑”，不得臆造。

## 工作流状态管理

所有任务必须通过状态系统追踪。状态文件位于 `.claude/agent-memory/workflow-state.json`。

### 状态检查前置步骤

在任何工作流开始前，**必须**先检查状态：

```bash
# 查看当前状态
python3 .claude/agent-memory/scripts/workflow-state.py status
```

**检查点：**
- [ ] 是否有 `in_progress` 的任务？如有，询问用户是否继续或重新开始
- [ ] 该 Node ID 是否已完成？如已完成，询问是否重新处理
- [ ] 队列中是否有待处理任务？

### 状态更新命令

```bash
# 开始新任务
python3 .claude/agent-memory/scripts/workflow-state.py start [NODE_ID] --phase analysis

# 切换阶段
python3 .claude/agent-memory/scripts/workflow-state.py phase [generation|verification]

# 记录产物
python3 .claude/agent-memory/scripts/workflow-state.py artifact [KEY] [PATH]

# 记录错误
python3 .claude/agent-memory/scripts/workflow-state.py error "错误信息"

# 完成任务
python3 .claude/agent-memory/scripts/workflow-state.py complete --success
```

### 阶段状态 Checklist

| 阶段 | 状态标记 | 产物记录 |
|------|----------|----------|
| 分析 | `phase: analysis` | `layoutReport: [path]` |
| 生成 | `phase: generation` | `slintFile: [path]` |
| 验证 | `phase: verification` | `verificationReport: [path]` (enhanced) |
| 完成 | `status: completed` | 归档到 `history` |

---

## 阶段一：分析 UI 布局

### 触发条件
- 用户提供了 Node ID
- 用户说"分析这个界面" / "提取布局信息"

### 主代理动作

**前置检查：**
1. 运行 `python3 .claude/agent-memory/scripts/workflow-state.py status` 检查当前状态
2. 如有进行中的任务，询问用户是否继续
3. 开始新任务：`python3 .claude/agent-memory/scripts/workflow-state.py start [NODE_ID] --phase analysis`

**启动子代理：**
直接启动 `ui-layout-analyzer`，在其 prompt 中**强制要求**：
1. 读取 `.claude/agent-memory/ui-layout-analyzer/MEMORY.md` 和已有模式文件
2. 使用 pencil MCP 获取指定 Node ID 的原始数据
3. **严格遵守其自身的 SOP** 构建树、跑 `print_hierarchy.py` 校验、保存报告
4. 返回报告文件的绝对路径

**状态更新：**
收到报告路径后，立即更新状态：
```bash
python3 .claude/agent-memory/scripts/workflow-state.py artifact layoutReport [REPORT_PATH]
python3 .claude/agent-memory/scripts/workflow-state.py phase generation
```

**Agent 调用方式**

```xml
<use_tool>
  <name>Agent</name>
  <arguments>
    {
      "description": "Analyze UI layout for [NodeID]",
      "prompt": "You are the ui-layout-analyzer agent. Analyze Node ID: [NODE_ID] from /home/dpower/Desktop/SDK/claude_code/door.pen.\n\nBefore starting, READ your agent memory at .claude/agent-memory/ui-layout-analyzer/MEMORY.md and any related files in that directory.\n\nThen execute your own standard SOP exactly:\n- Use pencil MCP to capture the full node tree and coordinates.\n- Build the Visual Hierarchy Tree matching the raw JSON structure.\n- Run print_hierarchy.py validation and fix any mismatches.\n- Save the report to .claude/agent-memory/ui-layout-analyzer/layouts/[NodeID]-[timestamp].md\n- Return ONLY the saved file path.",
      "subagent_type": "ui-layout-analyzer"
    }
  </arguments>
</use_tool>
```

---

## 阶段二：生成 Slint 代码

### 触发条件
- `ui-layout-analyzer` 已返回报告路径
- 用户直接提供了报告文件路径
- 用户说"生成 Slint 代码"

### 主代理动作

**前置检查：**
1. 确认状态 `phase` 为 `analysis` 且已有 `layoutReport`
2. 更新阶段：`python3 .claude/agent-memory/scripts/workflow-state.py phase generation`

**启动子代理：**
直接启动 `slint-ui-generator`，在其 prompt 中**强制要求**：
1. 读取 `.claude/agent-memory/slint-ui-generator/MEMORY.md` 和已有模式文件（如 `slint-syntax.md`、`design-tokens.md`）
2. 读取项目根目录 `work.md`，提取与当前 Node ID / 界面名称相关的交互逻辑
3. 解析给定的布局分析报告
4. **严格遵守其自身的 SOP** 生成 `.slint` 文件，并携带 `work.md` 中匹配到的交互逻辑（如 callback、状态属性、输入输出属性、倒计时/错误/跳转逻辑等）
5. 若 `work.md` 没有当前界面的交互逻辑，必须在返回结果中说明“未发现对应交互逻辑”，不得臆造业务行为
6. **运行 `slint-compiler` 验证，确保 0 errors**
7. **返回生成的 `.slint` 文件路径、编译结果、导出的图片文件清单，以及已携带的交互逻辑摘要**

**状态更新：**
收到 Slint 文件路径后，立即更新状态：
```bash
python3 .claude/agent-memory/scripts/workflow-state.py artifact slintFile [SLINT_PATH]
python3 .claude/agent-memory/scripts/workflow-state.py phase verification
```

### Icon 导出强制规则（Critical）

`slint-ui-generator` 在生成代码时，**必须**对报告中所有 `Name = Icon` 的节点执行以下操作：
- 使用 **pencil MCP `export_nodes`** 将该 Icon 节点导出为 PNG（scale: 1）
- 将 PNG 保存到 `assets/images/` 目录
- 在 `.slint` 文件中使用 `Image { source: @image-url("../assets/images/<filename>.png"); }` 引用
- **禁止使用 Text 占位符、Rectangle 色块或手写 Path 替代 Icon**

**Agent 调用方式**

```xml
<use_tool>
  <name>Agent</name>
  <arguments>
    {
      "description": "Generate Slint UI from layout report",
      "prompt": "You are the slint-ui-generator agent. Generate Slint UI code from the layout analysis report.\n\nBefore starting, READ your agent memory at .claude/agent-memory/slint-ui-generator/MEMORY.md and any related files in that directory (e.g. slint-syntax.md, design-tokens.md). Also READ project root work.md and extract interaction logic related to the current Node ID / screen name. If work.md has no matching logic, explicitly report that no matching interaction logic was found and do not invent business behavior.\n\nReport file path: [REPORT_FILE_PATH]\n\nThen execute your own standard SOP exactly:\n- Parse the report's Visual Hierarchy Tree and coordinates table.\n- Read work.md and carry matching interaction logic into Slint via callbacks, state properties, input/output properties, countdown/error/jump state, or TODO integration comments as appropriate.\n- For EVERY node with Name = 'Icon' in the report, MANDATORY export it via pencil MCP export_nodes (scale: 1) to assets/images/.\n- Generate Slint code that strictly follows the hierarchy and coordinates.\n- Apply all layout and styling rules stored in your memory.\n- Save to ui/[component-name].slint.\n- MANDATORY: run slint-compiler -o /dev/null ui/[component-name].slint and confirm 0 errors.\n- Return the saved file path, the compiler output, the carried interaction logic summary, AND a list of all exported image files.",
      "subagent_type": "slint-ui-generator"
    }
  </arguments>
</use_tool>
```

---

## 阶段三：增强渲染验证与修复 (CRITICAL)

### 触发条件
- `slint-ui-generator` 已返回生成的 Slint 文件路径
- 用户说"验证渲染结果" / "对比截图" / "修复渲染问题"

### 主代理动作

**前置检查：**
1. 确认状态 `phase` 为 `generation` 且已有 `slintFile`
2. 更新阶段：`python3 .claude/agent-memory/scripts/workflow-state.py phase verification`

**启动子代理：**
直接启动 `slint-verifier-enhanced`，在其 prompt 中**强制要求**：
1. 读取生成的 `.slint` 文件路径和对应的布局分析报告
2. 读取 `.claude/agent-memory/slint-verifier-enhanced/MEMORY.md` 验证模式
3. 读取项目根目录 `work.md`，确认当前界面的交互逻辑是否已体现在 Slint 中
4. 使用 `/usr/bin/pencil_to_slint` 将 Slint 代码渲染为 PNG 截图
5. **使用 `understand_image` 工具分析渲染后的 PNG**，提取实际像素位置、颜色值
6. 对比设计报告中的预期值与实际渲染像素值，识别偏差
7. 对比 `work.md` 中的交互逻辑，识别 callback、状态属性、输入输出属性、倒计时/错误/跳转逻辑是否缺失
8. 修复代码中的问题（位置偏移、颜色错误、文本截断、交互入口缺失等）
9. 重新渲染并验证，直到偏差在可接受范围内
10. **返回修复后的 Slint 文件路径、验证报告和交互逻辑检查结果**

**状态更新：**
验证完成后，立即更新状态：
```bash
python3 .claude/agent-memory/scripts/workflow-state.py artifact verificationReport [REPORT_PATH]
python3 .claude/agent-memory/scripts/workflow-state.py complete --success
```

### 验证检查清单 (Checklist)

**像素级位置偏差检查:**
- [ ] 使用 `understand_image` 提取实际像素位置
- [ ] 各文本元素的垂直位置偏差 ≤ 15px
- [ ] 容器内子元素位置与设计稿一致
- [ ] 整体布局无偏移

**像素级样式正确性检查:**
- [ ] 使用 `understand_image` 提取实际渲染颜色
- [ ] 圆角半径匹配设计稿
- [ ] 背景颜色/渐变正确
- [ ] 阴影效果存在且参数正确
- [ ] 透明度设置正确

**文本渲染检查:**
- [ ] `understand_image` 检测文本内容完整无截断
- [ ] 字体大小匹配设计稿
- [ ] 文本颜色正确
- [ ] 特殊字符（如数字"5"）可见且清晰

**图标检查:**
- [ ] `understand_image` 检测到所有 Icon 节点
- [ ] 图标尺寸和位置正确

### 增强验证核心要求

**必须使用 `understand_image` 工具：**
- 原版：对比 Slint 代码值 vs 设计报告值
- 增强版：**对比渲染 PNG 像素值 vs 设计报告值**

**验证阈值：**
- 位置偏差 > 15px = ❌
- 颜色不匹配 = ❌
- 文本截断 = ❌

**Agent 调用方式**

```xml
<use_tool>
  <name>Agent</name>
  <arguments>
    {
      "description": "Verify and fix Slint rendering with pixel-level analysis",
      "prompt": "Verify the generated Slint code against the design specifications using pixel-level analysis and fix any rendering issues.\n\nInputs:\n- Slint file path: [SLINT_FILE_PATH]\n- Design report path: [REPORT_FILE_PATH]\n\nExecute the enhanced verification workflow:\n1. Read your MEMORY.md at .claude/agent-memory/slint-verifier-enhanced/MEMORY.md\n2. Read the layout report to get design expected values\n3. Read project root work.md to get interaction logic for the current Node ID / screen name, and verify the Slint file carries matching callbacks/state/input-output/countdown/error/jump integration points. If no matching work.md logic exists, document that explicitly.\n4. Use `/usr/bin/pencil_to_slint [SLINT_FILE_PATH] [OUTPUT_PNG_PATH] [width] [height]` to render the UI\n5. **CRITICAL: Call `understand_image` tool to analyze the rendered PNG**, extract:\n   - All text elements with actual y-coordinates (pixels from top)\n   - All icons/buttons with actual x/y positions and dimensions\n   - Background colors (actual rendered RGB values)\n   - Layout issues (text truncation, element overlap, alignment)\n6. **MUST include the complete understand_image output in your report**\n7. Create pixel-level comparison table:\n   | Element | Property | Design Value | Actual Rendered Value | Pixel Deviation | Status |\n8. Compare with design report - flag deviations > 15px as FAILED\n9. Fix identified issues in the Slint code:\n   - Adjust container positions\n   - Fix color values\n   - Correct font sizes\n   - Ensure text containers are wide enough\n   - Restore missing interaction callbacks/state properties from work.md\n10. Re-render and re-verify until all checks pass (max 3 iterations)\n11. Save verification report to `.claude/agent-memory/slint-verification/[NodeID]-enhanced.md`\n12. **Report MUST contain:**\n    - Complete UNDERSTAND_IMAGE output (raw)\n    - Pixel-level comparison table\n    - Interaction logic check against work.md\n    - Deviation analysis\n    - Fix history\n13. Return the fixed Slint file path, interaction logic check result, and summary of fixes applied",
      "subagent_type": "slint-verifier-enhanced"
    }
  </arguments>
</use_tool>
```

---

## 完整工作流示例

### 示例 1: 从 Node ID 开始 (完整流程)

用户: "帮我把这个界面转换成 Slint 代码，Node ID 是 abc123"

主代理执行:
```
1. 启动 ui-layout-analyzer 分析 abc123
2. 拿到报告路径后启动 slint-ui-generator
3. 拿到 Slint 文件后启动 slint-verifier 验证并修复
4. 返回修复后的 Slint 文件路径和验证报告
```

### 示例 2: 从已有报告开始

用户: "根据这个报告生成 Slint 代码: .claude/agent-memory/ui-layout-analyzer/layouts/abc123-20260401-120000.md"

主代理执行:
```
1. 跳过分析阶段
2. 直接启动 slint-ui-generator
3. 返回生成的 Slint 文件路径
```

---

## 文件输出规范

| 阶段 | 输出位置 | 命名格式 |
|------|----------|----------|
| 分析报告 | `.claude/agent-memory/ui-layout-analyzer/layouts/` | `[NodeID]-[YYYYMMDD-HHmmss].md` |
| Slint 代码 | `ui/` | `[component-name].slint` |
| 渲染截图 | `slint-screenshots/` | `[component-name].png` |
| 验证报告 | `.claude/agent-memory/slint-verification/` | `[NodeID]-enhanced.md` |

---

## 主代理禁止清单

- 禁止主代理自己用 `Read` 或 `pencil` 工具去翻看 `.pen` 文件内容
- 禁止主代理自己修改生成的 `.slint` 文件做修补（发现问题的正确做法：让子代理重做）
- 禁止主代理在调度前对层级关系、坐标、颜色做任何主观判断

---

## 阶段四：批量处理 (可选)

### 触发条件
- 用户说"批量处理 Plan X"
- 用户说"处理所有界面"
- 用户说"处理队列中的任务"

### Plan 节点映射

| Plan | 功能 | 节点列表 |
|------|------|----------|
| plan1 | 房号呼叫 | Rzj87, k016Y, KW3lm, K7Jgw |
| plan2 | 二维码开门 | TQQlc, 8TvZW, BTy4s, mTdn6 |
| plan3 | 密码开门 | 2jMG5, WKF8g, r93l7, dyulI, gdp75 |
| plan4 | 人脸识别 | 4glnf, APrBb, cWB4g |

### 主代理动作

**方式 1: 使用辅助脚本（推荐）**

```bash
# 处理单个 Plan
cd .claude/agent-memory/scripts
./batch-helper.sh plan1

# 并行处理（2个并发）
./batch-helper.sh plan2 -p 2

# 试运行（不实际执行）
./batch-helper.sh plan3 --dry-run

# 强制重新处理
./batch-helper.sh plan1 --force

# 处理所有 Plan
./batch-helper.sh all

# 处理队列中的任务
./batch-helper.sh queue

# 处理指定节点
./batch-helper.sh nodes TQQlc Rzj87
```

**方式 2: 使用 Python 脚本**

```bash
# 处理 Plan
cd scripts
python3 batch-processor.py --plan plan1

# 并行处理
python3 batch-processor.py --plan plan2 --parallel 3

# 处理指定节点
python3 batch-processor.py --nodes TQQlc Rzj87 k016Y

# 从队列处理
python3 batch-processor.py --from-queue
```

### 批量处理流程

```
1. 解析 Plan → 获取节点列表
2. 检查历史记录（跳过已完成节点）
3. 对每个节点执行: 分析 → 生成 → 验证
4. 记录每个节点的状态
5. 生成批量处理报告
```

### 批量处理报告

报告格式: `batch-report-[timestamp].md`

```markdown
# 批量处理报告

生成时间: 2026-04-08 14:00:00

## 汇总

| 指标 | 数值 |
|------|------|
| 总任务数 | 4 |
| ✅ 成功 | 3 |
| ❌ 失败 | 1 |
| ⏭️ 跳过 | 0 |
| 成功率 | 75.0% |
| 总耗时 | 45.2s |

## 详细结果

| 节点 | 状态 | 阶段 | 耗时 | 产物 |
|------|------|------|------|------|
| TQQlc | ✅ success | completed | 12.3s | layoutReport, slintFile |
| Rzj87 | ✅ success | completed | 10.1s | layoutReport, slintFile |
| k016Y | ❌ failed | generation | 8.5s | layoutReport |
| BTy4s | ✅ success | completed | 14.3s | layoutReport, slintFile |
```

---

## 阶段五：回归测试 (可选但推荐)

### 触发条件
- 用户说"运行回归测试"
- 批量处理完成后自动触发
- CI/CD 流程中自动执行

### 前置条件
1. 设计稿参考图已提取到 `design-references/`
2. Slint 代码已生成并验证

### 主代理动作

**运行回归测试：**

```bash
# 完整测试流程（渲染 + 对比 + 报告）
./scripts/regression-test.sh

# 仅测试特定节点
./scripts/regression-test.sh --node [NODE_ID]
```

**检查测试结果：**

1. 查看报告: `cat test-reports/summary-latest.md`
2. 检查差异图: `test-reports/diffs/*-diff.png`
3. 判断是否通过阈值

**处理失败项：**

- 如果差异在可接受范围内（如 < 1%）：记录警告，继续
- 如果差异过大：创建修复任务，重新进入阶段三

### 回归测试配置

测试参数位于 `scripts/regression-config.json`。

### 测试报告格式

报告保存为 `test-reports/summary-latest.md`，包含：
- 汇总统计
- 详细结果表格
- 失败项详情
- 下一步行动建议
