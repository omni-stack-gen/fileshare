# q5E1Bu CCTV 连接测试进行中态验证报告

## 目标

- `hohrT` 点击后不再直接切换成功/失败，而是调用 `test-cctv-setting()` 启动真实 IPC monitor 验证。
- 连接测试进行中检查项按 Pencil Node ID `q5E1Bu` 效果显示。
- `StreamReady` 才切换为成功弹窗 `hMGBM`；`Offline` / `Error` / 超时保持失败弹窗 `E0zNY`。

## 产物

- 布局报告：`.claude/agent-memory/ui-layout-analyzer/layouts/q5E1Bu-20260507-143842.md`
- Slint 文件：`ui/CctvConnectionTestDialog.slint`
- 相关状态/回调：`ui/shared.slint`
- 业务绑定：`src/main.rs`
- 渲染截图：`slint-screenshots/CctvConnectionTestDialog-q5E1Bu-running.png`
- 导出图标：
  - `assets/images/SHJ1B.png`
  - `assets/images/R07WZe.png`
  - `assets/images/YQjMO.png`

## 视觉验证

已使用 `/usr/bin/pencil_to_slint` 渲染进行中态，临时 wrapper 设置：
- `State.cctv-setting-test-running = true`
- `State.cctv-setting-test-step = 3`
- `success = false`

对比 `q5E1Bu`：
- Dialog 尺寸为 560 x 443.5，垂直居中位置匹配。
- 前两项为绿色完成态，第三项为蓝色进行中态，后两项为灰色待处理态。
- 检查项间距、圆角、颜色、状态文字与 `q5E1Bu` 一致。
- 底部按钮运行态为左右等宽 249px，右侧文案为 `테스트 중...`。

状态：PASS

## 交互验证

Slint：
- `CctvSetting` 点击 ready 状态的连接测试按钮时设置 `active-overlay = "E0zNY"` 并调用 `State.test-cctv-setting()`。
- `CctvConnectionTestDialog` 使用 `cctv-setting-test-running`、`cctv-setting-test-step`、`cctv-setting-test-error` 显示逐项状态。
- 失败态允许点击 `재시도` 重新调用 `test-cctv-setting()`，不再本地强制切成功。
- 测试进行中点击 `닫기` 会清除 `cctv-setting-test-running` 并调用 `cctv-stop()`，避免后台超时后重新弹窗。

Rust：
- `test-cctv-setting()` 从当前 IP、账号、密码构造临时 `IpcCameraConfig`。
- 通过 `IpcController::start_monitor()` 启动实际 IPC monitor。
- IPC dispatcher 收到 `StreamReady` 时设置 `cctv-setting-tested = true` 并切换 `hMGBM`。
- IPC dispatcher 收到 `Offline` / `Error` 或 10 秒超时时设置错误信息并保持 `E0zNY`。
- IPC 配置未启用或无已保存摄像头时，仍保留 IPC service 用于新增摄像头的真实测试。

状态：PASS

## 编译验证

已执行：

```bash
slint-compiler -o /dev/null ui/CctvConnectionTestDialog.slint
slint-compiler -o /dev/null ui/CctvConnectionTestDialogRunningVerify.slint
slint-compiler -o /dev/null ui/app.slint
/usr/bin/pencil_to_slint ui/CctvConnectionTestDialogRunningVerify.slint slint-screenshots/CctvConnectionTestDialog-q5E1Bu-running.png 1024 600
cargo build --target riscv64gc-unknown-linux-gnu --release
```

结果：
- Slint 编译 0 errors，仅有导出组件非 Window 的既有 warning。
- 渲染成功。
- RISC-V release build 成功，最终一次耗时 59.69s。
- cargo 输出仅包含外部 Slint/G2D 依赖 warning，项目本次修改无新增 error。

最终状态：PASS
