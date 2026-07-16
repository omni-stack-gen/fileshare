# CODEX Handoff

本文档由当前仓库只读扫描整理，用于后续在新服务器继续开发。未能从代码或现有文档确认的信息标记为 TODO。

## 1. 项目目标

- 项目名：`kr_room_ui`。
- 目标形态：嵌入式室内机 UI，主界面尺寸为 `1024x600`。
- 主要能力：
  - Slint 页面与全局状态驱动室内机 UI。
  - 本地 F1 indoor 呼叫、监视、D2D 开锁。
  - AIOT 平台登录、CallX、开锁、同步时间、WebRTC 账号下发。
  - WebRTC 云端音视频通话。
  - IPC/CCTV 监视、IPC 配置、截图记录。
  - WiFi 扫描、连接、配置保存。
  - 系统信息、设置页、密码修改等基础设置。
- 交互来源：`work.md` 与 Pencil Node ID 约定。

## 2. 技术栈

- Rust 2021：应用主控、状态协调、FFI 封装。
- Slint `1.16.0`：UI 组件与全局状态，根入口为 `ui/app.slint`。
- `slint-g2d-platform`：riscv64 目标平台上的 framebuffer/input 平台层。
- Native C bridge：`native/*_bridge` 接入 AIOT、WebRTC、IPC、F1 indoor runtime。
- F1 upstream C 代码：`native/indoor_bridge/upstream/common`。
- Native shared C 支撑：`native/shared`，包含 `utils`、codec、socket adapter、net/udhcp 等。
- 构建：Cargo + `build.rs` + `cc` crate + `slint-build`。
- 目标平台：`riscv64gc-unknown-linux-gnu`。
- 宿主机开发：非 riscv64 默认走 stub bridge，主要用于 Rust/Slint 编译检查。

## 3. 启动命令

- 宿主机直接运行：
  ```bash
  cargo run
  ```
  注意：宿主机通常走 stub 后端，不能验证真实 WiFi/AIOT/WebRTC/IPC/F1 硬件链路。

- 目标平台运行：
  ```bash
  cargo run --target riscv64gc-unknown-linux-gnu --release
  ```
  TODO：`.cargo/config.toml` 配置了 `runner = "bash adb_run.sh"`，但当前扫描未发现 `adb_run.sh` 文件，需要补齐或改成实际部署命令。

- 动态库路径辅助脚本：
  ```bash
  source ./lib.sh
  ```
  该脚本会把仓库 `lib/` 下目录加入 `LD_LIBRARY_PATH`。

## 4. 测试命令

- Rust 单元测试：
  ```bash
  cargo test
  ```
  当前只扫描到 `src/call.rs` 中少量 `#[test]`。

- Slint 编译检查：
  ```bash
  slint-compiler -o /dev/null ui/app.slint
  ```

- Diff 格式检查：
  ```bash
  git diff --check
  ```

- UI 设计生成/验证任务还可能使用：
  ```bash
  /usr/bin/pencil_to_slint <SLINT_FILE_PATH> <OUTPUT_PNG_PATH> <width> <height>
  ```
  该命令来自 `AGENTS.md`，不是普通业务构建必需步骤。

## 5. 构建命令

- 指定目标 release 构建：
  ```bash
  cargo build --target riscv64gc-unknown-linux-gnu --release
  ```

- 当前项目约定：修改代码后需要执行上述 riscv64 release 构建。

- 宿主机构建：
  ```bash
  cargo build
  ```
  注意：`.cargo/config.toml` 默认 target 已设置为 `riscv64gc-unknown-linux-gnu`，若要强制宿主机目标，需要显式传本机 target 或临时调整 Cargo 配置。

- 交叉工具链配置来自 `.cargo/config.toml`：
  - linker：`/opt/F1/bin/riscv64-unknown-linux-gnu-gcc`
  - ar：`/opt/F1/bin/riscv64-unknown-linux-gnu-ar`
  - 额外 link 路径：仓库 `lib/` 与 `/opt/F1/...`

## 6. 当前开发进度

- 架构已经从旧 call/d2d 双栈迁移到 `native/indoor_bridge` + Rust `indoor.rs`。
- Rust 层已引入 `RuntimeCoordinator`，用于收敛 AIOT/WebRTC/Indoor 跨模块联动。
- `AppEvent` 已收敛 AIOT、Indoor、WebRTC 三类事件；IPC/WiFi 尚未进入统一 AppEvent。
- CallX 状态机已拆到 `src/runtime_coordinator/callx.rs`，当前只保留父设备普通 CallX 会话。
- AIOT proxy CallX 呼叫路径已清理；native AIOT 仍保留二次确认机代理登录/登出能力。
- Native 普通事件队列已抽到 `native/shared/utils/event_queue.*`，AIOT/WebRTC 已使用；IPC/Indoor 事件队列仍保留各自实现。
- 当前工作区存在未提交改动，涉及：
  - `native/aiot_bridge/kr_aiot_bridge.c`
  - `native/indoor_bridge/*`
  - `src/call.rs`、`src/indoor.rs`、`src/indoor_sys.rs`、`src/main.rs`、`src/runtime_coordinator/mod.rs`
  - 多个 `ui/*.slint`
  - 新增 `assets/images/xt2uo.png`、`ui/GuardPhoneCommunication.slint`
- TODO：新服务器接手前建议先确认这些未提交改动是否已验证并提交。

## 7. 已完成改动

从当前代码和 `runtime-coordinator-roadmap.md` 可确认：

- 新增/保留根目录 `runtime-coordinator-roadmap.md` 作为架构整理路线。
- `RuntimeCoordinator` 已从 `main.rs` 抽出，负责云端 CallX、WebRTC 账号、AIOT 开锁等协调。
- `src/app_event.rs` 已建立 `AppEvent::Aiot / Indoor / Webrtc`。
- `src/runtime_coordinator/callx.rs` 已承载 CallX 会话状态。
- `src/runtime_coordinator/media_policy.rs` 已承载 IPC 监视与页面/呼叫互斥策略。
- `src/call.rs` 已提供 Call UI 状态 helper，如 `apply_call_idle`、`apply_monitor_active`。
- `native/shared/utils/event_queue.*` 已用于 AIOT/WebRTC 普通控制事件队列。
- `native/shared` 已承载公共 C utils/support 代码，`build.rs` 编译 `kr_native_utils`。
- `lib/codec` 已作为 AudioCore/VDec/Vo 头文件统一路径。
- `src/db.rs` 当前实现 KV 文件存储，不再是 SQLite。
- CCTV 配置存储拆为 `ipc_cameras.kv`，截图记录由 `src/capture_records.rs` 扫描文件系统。
- WebRTC 地址当前在 `src/webrtc.rs` 中固定为 `43.108.58.129`。
- AIOT 默认 `aiot.server_addr` 为 `43.108.59.118:30007`。
- 二次确认机 device id 常量为 `3:10.2.17.1`。
- `DoorCameraCall.slint` / `MwoLv` 已接入主动 monitor start/stop 和本地拒绝/挂断后回 `LCvek` 的逻辑。

## 8. 未完成事项

- TODO：确认当前未提交改动是否都需要保留并提交。
- TODO：补齐或移除 `.cargo/config.toml` 中引用的 `adb_run.sh`。
- TODO：更新旧文档中仍提到 `SQLite`、`native/call_adapter`、`native/d2d_bridge` 的内容；当前代码已不匹配。
- TODO：阶段 3b 是否将 IPC/WiFi 事件也纳入 `AppEvent`。
- TODO：阶段 5.2 是否迁移 IPC 事件队列，同时保持 `timeout_ms < 0` 和 monitor cleanup 语义。
- TODO：确认 AIOT 二次确认机代理登录/登出能力是否继续保留，或者彻底删除。
- TODO：确认 `webrtc.server_addr` 是否应继续代码固定，还是恢复为 KV/AIOT 下发可配置。
- TODO：补全自动化测试；当前测试覆盖很少。
- TODO：上板验证 MwoLv monitor stop 后多媒体释放是否完全稳定。

## 9. 高风险模块

- `src/main.rs`
  - 文件很大，仍包含大量 UI binding、事件分发、资源策略和业务协调。
  - 修改页面跳转和通话按钮时容易影响 AIOT/WebRTC/Indoor 多条链路。

- `src/runtime_coordinator/*`
  - 管理 CallX 与 WebRTC/AIOT 协调；错误状态清理会导致后续呼叫被 busy 或误路由。

- `native/indoor_bridge/kr_indoor_bridge.c`
  - 接入 F1 indoor runtime、media thread、monitor、二次确认机 SPE。
  - 多媒体释放、线程 stop/destroy 顺序风险高。

- `native/indoor_bridge/upstream/common`
  - F1 upstream 快照。除非明确移植补丁，否则不应随意改 upstream 逻辑。

- `native/webrtc_bridge/kr_webrtc_bridge.c`
  - 音频 ring buffer、视频解码显示、SDK callback 生命周期风险高。

- `native/aiot_bridge/kr_aiot_bridge.c`
  - AIOT login、CallX、proxy_sn unsupported、二次确认机代理登录/登出都集中在此。

- `native/ipc_bridge/kr_ipc_bridge.c`
  - ONVIF/RTSP/VideoDec/VO 显示链路，容易受解码尺寸、I 帧拆分、stop cleanup 影响。

- `src/db.rs`
  - KV 文件格式、默认值、兼容读取和原子写入都在单文件内。

## 10. 已知问题

- `.cargo/config.toml` 指定 `runner = "bash adb_run.sh"`，当前仓库未扫描到 `adb_run.sh`。
- `docs/current-features.md`、`docs/runtime-flows.md`、`docs/config-and-integration.md` 中部分内容与当前代码不一致，例如 SQLite、旧 call_adapter/d2d_bridge 描述。
- `src/main.rs::log_memory_usage()` 当前直接返回，不输出内存日志；调用点很多但实际无日志。
- `src/webrtc.rs` 中 `webrtc.server_addr` 被 `FIXED_WEBRTC_SERVER_ADDR` 覆盖，KV 中同名配置不会生效。
- `native/aiot_bridge/kr_aiot_bridge.c` 仍保留二次确认机代理登录/登出代码，proxy CallX 呼叫能力已清理；后续需要确认业务边界。
- 当前工作区有未提交改动，handoff 接手时不能假设 HEAD 是完整最新状态。
- TODO：当前板端运行问题、最新 `log/1.txt` 状态未在本次扫描中确认。

## 11. 环境变量说明

- `KR_ROOM_DB_PATH`
  - Rust：`src/db.rs` 使用它决定 KV 存储 root。
  - Python：`scripts/kv_tool.py` 使用同样规则。
  - 如果值带文件扩展名，代码取其父目录作为 root；否则直接作为 root 目录。

- `LD_LIBRARY_PATH`
  - `lib.sh` 会把仓库 `lib/` 下所有目录拼入 `LD_LIBRARY_PATH`。
  - 目标板运行动态库时通常需要确保 `lib/` 相关 `.so` 可被找到。

- Cargo/build 相关：
  - `.cargo/config.toml` 设置 `CC_riscv64gc_unknown_linux_gnu` 和 `AR_riscv64gc_unknown_linux_gnu`。
  - `build.rs` 读取 `CARGO_MANIFEST_DIR` 和 `TARGET`。

- TODO：运行时是否还有板端服务依赖的环境变量，当前扫描未确认。

## 12. 数据库/迁移说明

- 当前不是 SQLite。目标平台上 `src/db.rs` 使用 KV 文件：
  - `settings.kv`
  - `wifi_profiles.kv`
  - `ipc_cameras.kv`

- KV root 选择顺序：
  1. `KR_ROOM_DB_PATH` 非空时使用它。
  2. `/data` 存在时使用 `/data/kr_room`。
  3. 否则使用当前目录 `.`。

- `settings.kv` 格式：
  ```text
  key<TAB>type<TAB>base64(value)
  ```
  支持类型：`bool`、`int`、`float`、`string`。

- `wifi_profiles.kv` 格式：
  ```text
  base64(ssid)<TAB>base64(password)<TAB>priority<TAB>auto_connect<TAB>last_connected_at
  ```

- `ipc_cameras.kv` 格式：
  ```text
  index<TAB>enabled<TAB>prefer_substream<TAB>base64(name)<TAB>base64(ip)<TAB>base64(user)<TAB>base64(password)<TAB>base64(rtsp_url)<TAB>base64(sub_rtsp_url)
  ```

- 默认值来源：`src/db.rs::DEFAULT_SETTINGS`、`default_wifi_profiles()`、空 `ipc_cameras.kv`。

- 辅助工具：
  ```bash
  python3 scripts/kv_tool.py --help
  python3 scripts/kv_editor.py
  ```

- TODO：如果从旧 SQLite 版本升级，当前仓库未扫描到迁移脚本。

## 13. 后续开发建议

- 先处理工作区状态：确认未提交改动、上板结果和提交边界。
- 将 `docs/current-features.md`、`docs/runtime-flows.md`、`docs/config-and-integration.md` 更新到当前 `indoor_bridge + KV` 架构，避免误导。
- 保持架构边界：
  - Rust 主控和协调。
  - Native bridge 只做 SDK/硬件适配。
  - Native bridge 之间不要直接互调。
- 大改呼叫流程前先画清楚状态机：本地 Indoor、AIOT CallX、WebRTC 三者的 session 生命周期必须分开。
- 对 `main.rs` 继续做小步迁移，不要一次性重写：
  - 可优先迁移 IPC/WiFi 事件到 AppEvent。
  - 可继续把 UI 状态适配 helper 从 `main.rs` 拆出。
- 对 `native/indoor_bridge/upstream/common` 保持快照思路，必要改动应有明确注释和对比依据。
- 为高风险状态机补自动化测试，至少覆盖 CallX invited/confirmed/hungup、WebRTC end cleanup、本地 MwoLv monitor stop。
- 每轮修改后执行：
  ```bash
  git diff --check
  cargo build --target riscv64gc-unknown-linux-gnu --release
  ```

## 14. Code Review 重点

- 架构边界：
  - Rust 是否仍是主控层。
  - Native bridge 是否只封装 SDK/硬件，不引入跨 bridge 业务耦合。

- 呼叫与媒体：
  - CallX session 是否在 HUNGUP、WebRTC disconnect/destroy、本地挂断后清理。
  - MwoLv monitor stop 是否只请求媒体线程关闭，避免跨线程重复销毁 mtrans channel。
  - WebRTC callback 的 user 指针生命周期和 stop/destroy 顺序是否安全。

- AIOT：
  - `GOBLIN_AIOT_CALLX_*` 常量是否与 SDK 头文件一致。
  - `caller_device_kind` 路由是否正确进入 `Bu0fW`/`EJDbm`。
  - 带 `proxy_sn` 的 CallX 是否按当前策略 unsupported，不进入 Rust UI 链路。
  - AIOT 登录前置条件是否清楚：`aiot.enabled`、`aiot.model`、`aiot.server_addr`、WiFi/网络状态。

- WebRTC：
  - `FIXED_WEBRTC_SERVER_ADDR` 是否符合当前业务。
  - AIOT 下发账号与固定地址覆盖的关系是否被 Review 到。
  - 音频 ring buffer 和视频帧队列是否避免在 SDK callback 内阻塞。

- Indoor/F1：
  - `native/indoor_bridge/upstream/common` 是否被无意修改。
  - `simple_monitor_stop()` 与本地媒体关闭请求是否由正确线程执行实际 stop。
  - 二次确认机 device id/IP 固定值是否符合现场环境。

- IPC/CCTV：
  - 页面离开 `JVRSN` 是否停止 IPC monitor。
  - 云端 CallX 呼入时是否停止 IPC monitor。
  - IPC camera 保存是否保留已有列表，不误清空。

- KV 数据：
  - 写入是否使用现有 `Database` API 或 `scripts/kv_tool.py`，避免手写错误 base64/tab 格式。
  - 新增配置是否同步默认值、读取逻辑和文档。

- UI/Slint：
  - Node ID 页面跳转是否符合 `work.md`。
  - 按钮点击态和真实设备回调不要互相覆盖。
  - 修改 Slint 后至少跑 `slint-compiler -o /dev/null ui/app.slint`。

- 构建：
  - `build.rs` 是否只引用仓库内 `native/` 和 `lib/`，不要重新引入外部候选目录。
  - 新增 C 文件是否正确加入 `rerun-if-changed`。
  - riscv64 release 构建必须通过。
