# 设置页标题间距全量截图检查 - 2026-04-21

## 检查范围

- 页面数量：52 个 `ui/pages/*.slint` 页面，排除共享组件 `page-base.slint`。
- 截图目录：`slint-screenshots/title-audit-20260421/`
- 顶部标题区域接触图：`slint-screenshots/title-audit-20260421/contact/top-title-contact-after-fix.png`
- 底部区域复查图：`slint-screenshots/title-audit-20260421/contact/bottom-contact-after-title-fix.png`
- 全量页面接触图：`slint-screenshots/title-audit-20260421/contact/all-pages-contact-after-title-fix.png`

## 发现的问题

- 多个设置页标题位于 `y:105px` 时，首个列表、输入框、表格或控制项从 `y:125~170px` 开始，视觉上贴近标题。
- `PwdSet` 的顶部单选项位于标题附近，常规标题上移后仍然容易与标题区域拥挤。

## 修复内容

- 所有使用 `SettingsStatusHeader` 的设置页标题统一上移到 `y:60px`，增加标题与首个组件之间的间距，同时不下推底部内容。
- `PwdSet` 页面标题单独上移到 `y:35px`，避免与“开锁密码 / 管理员密码”单选项挤在一起。

## 验证结果

- `slint-compiler -o /dev/null ui/workflow-generated.slint`：通过。
- `/usr/bin/pencil_to_slint`：52 个页面全部渲染成功。
- `cargo check`：通过，仅有 `slint-embedded-soft` 依赖内既有 dead_code warning。
- 复查结论：未再发现设置页标题与下方首个组件明显贴近或重叠；底部返回键区域未引入新的覆盖问题。
