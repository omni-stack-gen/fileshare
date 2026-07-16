# 设置页底部间距全量截图检查 - 2026-04-21

## 检查范围

- 页面数量：52 个 `ui/pages/*.slint` 页面，排除共享组件 `page-base.slint`。
- 截图目录：`slint-screenshots/full-audit-20260421/`
- 全量接触图：`slint-screenshots/full-audit-20260421/contact/all-pages-contact-after-fix.png`
- 设置页底部区域接触图：`slint-screenshots/full-audit-20260421/contact/settings-bottom-crops-after-fix.png`

## 发现的问题

- `FaceLicenseInput`：数字键盘底行覆盖标准设置返回键区域。
- `FacePwd`：数字键盘底行覆盖标准设置返回键区域。
- `FacePwdInput`：数字键盘底行覆盖标准设置返回键区域。

其他页面未发现同类的底部组件与返回键视觉重叠或贴边问题。

## 修复内容

- `ui/pages/FaceLicenseInput.slint`：数字键盘整体上移，并同步修正确认键点击区域。
- `ui/pages/FacePwdInput.slint`：数字键盘整体上移，并同步修正确认键点击区域。
- `ui/pages/FacePwd.slint`：在保留上方三行密码输入框的前提下，缩小并居中数字键盘，同步修正所有数字键、确认键和退格键点击区域。

## 验证结果

- `slint-compiler -o /dev/null ui/workflow-generated.slint`：通过。
- `/usr/bin/pencil_to_slint`：52 个页面全部渲染成功。
- `cargo check`：通过，仅有 `slint-embedded-soft` 依赖内既有 dead_code warning。
- 复查结论：设置页面标准返回键区域不再被底部组件覆盖。
