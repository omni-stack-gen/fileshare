# KR_ROOM 当前功能总览

本文基于 `SESSION_HANDOFF.md`、`work.md`、`src/main.rs`、`src/*.rs` 与 `ui/*.slint` 整理当前项目已具备的功能。目标是描述“现在代码已经做了什么”，不包含未实现的规划功能。

## 项目定位

- 项目名：`kr_room_ui`
- 技术栈：Rust + Slint + Native C bridge
- 目标平台：`riscv64gc-unknown-linux-gnu`
- UI 尺寸：1024 x 600
- 应用入口：`src/main.rs`
- Slint 根界面：`ui/app.slint`
- 全局状态与回调：`ui/shared.slint`
- 交互依据：`work.md`

项目形态是室内机 UI，当前覆盖主界面、呼叫、CCTV 监视、截图记录、WiFi、系统/通用/显示/声音/模式/其他设置、密码修改、系统信息等页面。

## 页面功能

### 主界面

实现文件：`ui/MainPage.slint`

- 显示系统时间与日期，Rust 侧每秒刷新。
- 入口按钮：
  - 현관：进入门口呼叫页。
  - 경비실：进入警卫室呼叫页。
  - CCTV：进入 CCTV 监视页。
  - 외출모드：显示全屏黑色外出模式蒙版。
  - 엘리베이터：进入电梯呼叫页。
  - 설정：进入设置首页。
- 待机时钟：主界面无覆盖层、未处于外出模式且启用时钟屏时，空闲达到配置值后进入时钟页。

### 呼叫页面

实现文件：

- `ui/DoorCameraCall.slint`
- `ui/LobbyPhoneCall.slint`
- `ui/GuardPhoneCall.slint`
- `ui/EVCalling.slint`
- `src/call.rs`
- `src/main.rs`

当前呼叫能力分为两类：

- 本地 Call：通过 `native/call_adapter` 接入门口机/室内机呼叫，事件映射到 Slint 状态。
- 云端 CallX + WebRTC：通过 AIOT CallX 协商，WebRTC 负责云端音视频会话。

页面状态包含：

- `call-session-state`
- `call-peer-id`
- `call-video-active`
- `call-audio-active`
- `call-last-error`
- 呼叫按键效果状态，如 `door-call-effect-active`、`lobby-open-effect-active` 等。

当前动作入口：

- 接听：`State.call-accept()`
- 拒接：`State.call-reject()`
- 挂断：`State.call-hangup()`
- 发起云端呼叫：`State.call-start(target)`
- 开门：`State.call-open-door()`

按 `work.md`，部分呼叫页按键会先体现视觉按下效果；实际设备动作由 Rust 绑定决定。

### WiFi 与网络设置

实现文件：

- `ui/NetworkSetting.slint`
- `ui/WifiPasswordDialog.slint`
- `src/wifi.rs`
- `src/main.rs`

当前能力：

- WiFi 开关。
- 扫描 SSID 列表。
- 选择 SSID 后弹出密码键盘。
- 支持小写/大写/符号键盘模式。
- 发起连接并显示连接中锁定状态。
- 连接成功弹出成功覆盖层。
- 连接失败弹出失败覆盖层。
- 连接成功后保存 `network.connected_ssid`，并 upsert 到 `wifi_profiles`。
- 非目标平台或运行库缺失时使用 stub 后端，便于宿主机开发。

连接成功后的联动：

- 启动 AIOT runtime。
- 启动 WebRTC runtime。

### AIOT 平台

实现文件：

- `src/aiot.rs`
- `src/main.rs`
- `native/aiot_bridge/*`

当前能力：

- 按配置创建 AIOT 句柄。
- WiFi 已连接后懒启动 AIOT。
- 处理连接状态、开锁、同步时间、WebRTC 账号、CallX、错误等事件。
- `aiot.mqtt_server_addr` 为空时回退使用 `aiot.server_addr`。
- 收到云端开锁事件时转交 D2D 默认门开锁。
- 收到云端同步时间事件时设置系统时间并刷新 UI 时钟。
- 收到 WebRTC 账号事件时更新 WebRTC runtime 账号。

AIOT 启动前置条件：

- `aiot.enabled = 1`
- `aiot.model` 非空
- `aiot.server_addr` 非空
- WiFi 连接成功或已有 connected snapshot

### WebRTC 云端通话

实现文件：

- `src/webrtc.rs`
- `src/main.rs`
- `native/webrtc_bridge/*`

当前能力：

- WebRTC runtime 可先创建为未登录状态。
- AIOT 下发 `server_addr`、`serno`、`initstring` 后重建并启动 WebRTC service。
- 处理在线、离线、连接失败、呼叫失败、错误、远程开锁等事件。
- 云端主呼流程：CallX confirmed 后才调用 `webrtc_streamer_call`。
- 云端被呼流程：收到邀请后 ack，接听时发送 accept，不主动发起 WebRTC call。

### CCTV 监视与截图

实现文件：

- `ui/CCTVScreen.slint`
- `src/ipc.rs`
- `src/main.rs`
- `native/ipc_bridge/*`

当前能力：

- 从配置筛选可用 IPC 摄像头。
- CCTV 页面初始化时调用 `State.cctv-open()`。
- 异步启动监视，避免阻塞 UI。
- 退出页面时异步停流。
- 支持选择摄像头，若当前在 CCTV 页会切换监视源。
- 显示当前摄像头名称、在线/失败状态和错误文本。
- 支持预览亮度实时调节。
- 支持保存 JPEG 截图到 capture 目录。
- 云端来电时，如果 IPC 正在监视，会先异步停止 IPC 监视。

IPC 可用条件：

- `ipc.enabled = 1`
- 至少一个摄像头满足：
  - `ipc.cameraN.enabled = 1`
  - 且 `ip` / `rtsp_url` / `sub_rtsp_url` 至少一个非空

### 截图记录

实现文件：

- `ui/CaptureImage.slint`
- `ui/CaptureFullImage.slint`
- `src/capture_records.rs`

当前能力：

- 扫描 capture 目录中的 jpg/jpeg/png 文件。
- 以 6 条为一页展示截图记录。
- 展示日期、时间、来源标签、未读标记。
- 进入大图页查看。
- 大图上一张/下一张。
- 删除当前大图。
- 选择单条记录并删除选中。
- 删除全部记录。
- 打开未读大图时将文件名前缀从 `unread__` 改为 `read__`。

文件命名规则：

- 新截图：`unread__<camera_label>__<unix_timestamp>.jpg`
- 已读：`read__<camera_label>__<unix_timestamp>.jpg`

### 设置页面

实现文件：

- `ui/Setting.slint`
- `ui/GeneralSetting.slint`
- `ui/DisplaySetting.slint`
- `ui/SoundSetting.slint`
- `ui/NetworkSetting.slint`
- `ui/ModeSetting.slint`
- `ui/EtcSetting.slint`
- `ui/PasswordChange.slint`
- `ui/SettingsInfoPage.slint`
- `src/db.rs`
- `src/main.rs`

当前能力：

- 设置首页跳转到访客记录、通用、显示、声音、网络、模式、其他设置。
- 通用设置：
  - 语言选择。
  - 时间日期选择。
  - 调用系统时间设置。
- 显示设置：
  - 亮度、色彩、对比度、饱和度等值保存。
- 声音设置：
  - 门口/警卫室铃声选择。
  - 通话音量、麦克风等声音参数保存。
- 模式设置：
  - 外出模式时间。
  - 通话时间。
  - 门开时间。
  - 时钟屏开关等状态保存。
- 其他设置：
  - 密码修改。
  - 系统信息弹窗。
  - 系统重置弹窗。
  - 设置户号/房号页面。
- 密码修改：
  - 旧密码、新密码 6 位输入。
  - 密码显示/隐藏。
  - 目标平台上校验数据库中的密码 hash。
  - 通过 D2D 设置门锁密码，成功后更新本地数据库。
  - 失败显示密码错误弹窗。

### 系统信息

实现文件：

- `ui/SystemInformation.slint`
- `src/system_info.rs`

当前能力：

- 刷新设备系统信息。
- 显示 MAC、IP 等信息。
- 支持二维码图片状态字段。

### 本地数据与配置

实现文件：`src/db.rs`

当前能力：

- 目标平台上初始化 SQLite 数据库。
- 默认数据库路径：
  - 若 `KR_ROOM_DB_PATH` 存在，使用该路径。
  - 若 `/data` 存在，使用 `/data/kr_room/kr_room.db`。
  - 否则使用项目根目录 `kr_room.db`。
- 表结构：
  - `schema_meta`
  - `settings`
  - `wifi_profiles`
  - `visitor_records`
- 支持保存 bool/int/float/string 设置。
- 支持 WiFi profile 保存。
- 支持门锁密码明文与 SHA-256 hash 存储。

## Native Bridge

当前 Native bridge 分布：

- AIOT：`native/aiot_bridge`
- WebRTC：`native/webrtc_bridge`
- IPC：`native/ipc_bridge`
- 本地 Call：`native/call_adapter`
- D2D：`native/d2d_bridge`
- PLC 共享运行时：`native/shared`

构建脚本 `build.rs` 会根据目标平台和库文件可用性选择真实 bridge 或 stub bridge。

## 当前限制与风险

- 本地 Call 视频链路仍有历史 `Bus error` 风险，相关日志前缀为 `kr_call video:`。
- IPC 对 RTSP 地址依赖较高，部分摄像头无法通过 ONVIF 自动获得可用地址，需要显式配置 `rtsp_url` 或 `sub_rtsp_url`。
- 非 riscv64 平台默认使用 stub 后端，适合 UI/流程开发，不代表真实设备链路可用。
- 部分 `work.md` 中标注“未说明”的业务结果仍只保留 UI 入口或弹窗，不臆造实际设备行为。
