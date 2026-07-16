# Q5kNvw Slint 验证报告

目标节点：`Q5kNvw` / `pJGE2`

## 验证对象

- Slint 文件：`ui/CctvNumpadDialog.slint`
- 交互状态：`ui/shared.slint`
- Rust 回调：`src/main.rs`
- 渲染截图：`slint-screenshots/CctvNumpadDialog-pJGE2.png`

## 检查结果

- PASS：IP 输入区域已从单字符串输入改为 4 个独立段输入框。
- PASS：空输入状态显示第一段 `|` 并高亮，其余三段显示 `0`，确认按钮禁用，匹配 `pJGE2`。
- PASS：`Q5kNvw` 的 4 段结构、点号分隔、激活段蓝色视觉已保留。
- PASS：`work.md` 已补充 `Q5kNvw` 的 4 段 IP 输入交互要求。
- PASS：`slint-compiler -o /dev/null ui/CctvNumpadDialog.slint` 通过。
- PASS：`slint-compiler -o /dev/null ui/app.slint` 通过。
- PASS：`cargo build --target riscv64gc-unknown-linux-gnu --release` 通过。

## 交互检查

- 数字输入：写入当前 IP 段，单段最多 3 位，并限制到 `0..255`。
- 点号输入：切换到下一 IP 段。
- 退格：删除当前段内容；当前段为空时回到上一段删除。
- 确认：4 段均非空时提交为 `a.b.c.d`。

## 限制

当前环境没有 `understand_image` 工具；已使用 Pencil 截图和 `view_image` 人工对照渲染结果。
