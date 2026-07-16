# GUr03 / Dr8p5 Slint 验证报告

## 验证对象

- `ui/CctvList.slint`
- `ui/CctvDeleteConfirmDialog.slint`
- `ui/CctvSetting.slint`
- `ui/EtcSetting.slint`
- `ui/app.slint`
- `ui/Overlays.slint`
- `src/main.rs`
- 渲染截图：
  - `slint-screenshots/CctvList-GUr03.png`
  - `slint-screenshots/CctvDeleteConfirmDialog-Dr8p5.png`

## 视觉检查

- PASS：`GUr03` 列表页保留 Header、说明文案、两条列表项、删除按钮、右箭头和底部添加按钮。
- PASS：`Dr8p5` 删除弹窗保留遮罩、440 x 245 弹窗、顶部删除标题、确认文案和取消/删除按钮。
- PASS：图标节点已导出为 PNG 并通过 `Image` 引用：`o0SioU.png`、`HrM48.png`、`E0G49.png`、`NIcwn.png`。

## 交互检查

- PASS：`MpUw5` 从其他设置进入 `GUr03`。
- PASS：列表成员点击调用 `cctv-select-setting-camera(index)` 并进入 `foYV2`。
- PASS：`CZq0T` 设置待删除索引并打开 `Dr8p5`。
- PASS：`zZElu` 清空新增表单状态并进入 `UGcOs`。
- PASS：`Dr8p5` 取消关闭弹窗，删除调用 `cctv-delete-setting-camera(index)`。
- PASS：Rust 侧删除后更新数据库 IPC camera 列表，并刷新 Slint 列表状态。

## 编译与渲染

- PASS：`slint-compiler -o /dev/null ui/CctvList.slint`
- PASS：`slint-compiler -o /dev/null ui/CctvDeleteConfirmDialog.slint`
- PASS：`slint-compiler -o /dev/null ui/app.slint`
- PASS：`/usr/bin/pencil_to_slint ui/CctvList.slint slint-screenshots/CctvList-GUr03.png 1024 600`
- PASS：`/usr/bin/pencil_to_slint ui/CctvDeleteConfirmDialog.slint slint-screenshots/CctvDeleteConfirmDialog-Dr8p5.png 1024 600`

## 限制

当前环境没有 `understand_image` 工具；使用 Pencil 截图、Slint 渲染 PNG 和人工视觉对照替代。
