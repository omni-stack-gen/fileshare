# KR_ROOM 当前开发交接

本文记录当前工作状态、已完成事项、未完成事项和下一步验证标准，供后续继续开发使用。

## 当前工作状态

- 当前仓库工作区未提交，`git status --short` 显示多处修改和新增资源。
- 最近一次完整构建已通过：
  - `git diff --check`
  - `cargo check --target riscv64gc-unknown-linux-gnu`
  - `cargo build --target riscv64gc-unknown-linux-gnu --release`
- 当前重点开发线是：
  - 二次确认机 D2D 呼入转呼 APP。
  - AIOT callX 媒体协商。
  - WebRTC 桥接视频排查。
  - 主界面外出模式和状态图标。

## 已完成事项

- 主界面外出模式：
  - 外出模式不再显示全屏黑色遮罩。
  - 点击外出模式按钮后图标和文字变红，再次点击恢复。
  - 状态保存到数据库 `mode.absent_active`，断电重启后恢复。

- 主界面状态显示：
  - 主界面时间不再显示 AM/PM。
  - Wi-Fi 图标左侧增加服务器在线图标 `assets/images/JPh5M.png`。
  - 服务器图标由 AIOT 连接状态驱动。

- UI 图片替换：
  - 呼叫管理机界面中间图使用 `assets/images/Group_4.png`。
  - 呼叫电梯界面使用 `images/Group 2.png`。

- 测试模式网络：
  - 支持通过 `KR_ROOM_TEST_CLOUD_IFACE=eth1` 让 AIOT/WebRTC 走测试网卡。
  - 正常模式仍按数据库配置走原网卡。

- AIOT 代理二次确认机：
  - 二次确认机按小门口机处理，代理类型 `proxy_kind=entryDoor`。
  - 二次确认机在线/离线/详情/设置上报携带代理信息。

- AIOT 登录：
  - AIOT 登录失败后增加 30 秒重试逻辑。
  - `aiot.mqtt_server_addr` 可临时覆盖 MQTT 地址，空时回退到 `aiot.server_addr`。

- AIOT 呼叫管理室：
  - 主界面管理室呼叫走 AIOT callX + WebRTC。
  - 管理室主动呼叫临时切为音频模式，结束后恢复默认配置。

- 二次确认机转呼 APP：
  - 二次确认机 D2D 呼入时可按室内机房号转呼 APP。
  - 转呼只邀请 APP 类型 callee。
  - APP 接听后进入桥接态，目标是 APP 与二次确认机通话。
  - 当前预期 callXInitiate 媒体配置：
    - `supportedMedias=["video","audio"]`
    - `requiredMedias=["audio"]`
  - 为避免 SDK ABI 错位，`lib/goblin_aiot_callx.h` 的 `initiate` 结构保持当前 SDK 头文件布局，不再手动追加媒体字段。
  - 媒体配置通过 `goblin_aiot_set_callx_config()` 临时设置。

- WebRTC 桥接视频排查：
  - 支持 `KR_ROOM_DUMP_BRIDGE_H264=1` 保存桥接 H264 码流。
  - 默认保存两路：
    - D2D 输入码流。
    - 实际送 WebRTC SDK 的码流。
  - 支持 WebRTC 登录后通过 `KR_ROOM_WEBRTC_TEST_H264=1` 循环推送 `bridge_test.h264` 测试流。
  - 二次确认机转 APP 视频已改为收到 H264 后全量送 WebRTC，不再 15fps 限流。

- 声音：
  - 已加入触摸点击声和呼叫铃声。
  - 铃声连续播放，接听/挂断后停止。
  - 铃声文件路径和音量逻辑已接入现有设置。

- 调试控制：
  - `/data/debugflag` 存在时，呼叫无人接听和接听后的通话不走超时自动挂断。

## 已确认发现

- WebRTC 发送接口只声明 `WEBRTC_VIDEO_H264`，当前代码没有配置 H264 Baseline/Main/High profile 的接口。
- 二次确认机转 APP 的 H264 profile 由二次确认机输出的 SPS 决定，室内机当前不做解码后重编码。
- 如果要确认 H264 profile，应分析 dump 文件：
  - `ffprobe -show_streams bridge_xxx_webrtc.h264`
- `spi plc get free buffer fail index:-1` 是底层库打印，当前排查中忽略。
- AIOT `callXInitiate` 的 `supportedMedias/requiredMedias` 由 SDK 内部根据 `goblin_aiot_set_callx_config()` 生成，不应随意修改 `goblin_aiot_callx.h` 的结构体布局。

## 未完成/待验证事项

- 二次确认机转呼 APP 的最新 MQTT JSON 仍需板端验证：
  - 必须同时出现 `callKind:"visitor"`。
  - 必须出现 `calleeApartment:"101"` 或实际房号。
  - 必须出现 `supportedMedias:["video","audio"]`。
  - 必须出现 `requiredMedias:["audio"]`。

- APP 接听后二次确认机桥接仍需实测：
  - 二次确认机是否进入接听状态。
  - 室内机是否显示 `APP 接听中`。
  - APP 挂断是否联动挂断二次确认机。
  - 二次确认机和 APP 双向音频是否正常。
  - 桥接状态下室内机本机麦克风/喇叭是否不参与。

- WebRTC 视频卡顿仍需现场继续定位：
  - 对比 `_input.h264` 与 `_webrtc.h264` 是否完整。
  - 查看 SDK 是否出现 `queue_full`、`video_skip`、`que_count` 上升。
  - 如 APP 仍卡顿，应优先推动二次确认机输出低码率/低帧率或兼容 profile。

- WebRTC/SILK 音频重采样异常未彻底修复：
  - 已确认 `silk/resampler.c assertion failed: inLen >= S->Fs_in_kHz` 通常是送入 OPUS/SILK 的 PCM 帧长度不足。
  - 后续应在桥接音频路径增加固定帧长缓存后再送编码器。

- CPU/内存升高问题仍需继续排查：
  - 已建议忽略 PLC free buffer 底层打印。
  - 后续应结合进程 RSS、线程 CPU、事件队列长度、媒体队列长度和音视频回调频率定位。

## 下一步建议

1. 先在板端复测二次确认机呼入转 APP 的 `callXInitiate` JSON。
2. 如果 JSON 字段不符合预期，优先确认当前 `lib/goblin_aiot_callx.h` 是否与 `lib/libgoblin_aiot.so` 同版本匹配。
3. 如果 JSON 正确但 APP 桥接不通，优先查：
   - `src/runtime_coordinator/callx.rs`
   - `src/runtime_coordinator/mod.rs`
   - `native/indoor_bridge/kr_indoor_bridge.c`
   - `native/webrtc_bridge/kr_webrtc_bridge.c`
4. 如果视频仍慢，开启：
   - `KR_ROOM_DUMP_BRIDGE_H264=1`
   - `KR_ROOM_DUMP_BRIDGE_H264_DIR=/tmp/kr_room_bridge_dump`
5. 如果要测试固定码流，开启：
   - `KR_ROOM_WEBRTC_TEST_H264=1`
   - `KR_ROOM_WEBRTC_TEST_H264_PATH=./bridge_test.h264`

## 验证标准

- 构建检查：
  - `git diff --check`
  - `cargo check --target riscv64gc-unknown-linux-gnu`
  - `cargo build --target riscv64gc-unknown-linux-gnu --release`

- AIOT 转呼验证：
  - 二次确认机呼入后，室内机保留本地待接听界面。
  - AIOT 发出面向住户 APP 的 `callXInitiate`。
  - `calleeApartment` 使用室内机设置的 Room 号。
  - `supportedMedias=["video","audio"]`。
  - `requiredMedias=["audio"]`。

- 桥接验证：
  - APP 接听后二次确认机进入通话。
  - APP 可听到二次确认机音频。
  - 二次确认机可听到 APP 音频。
  - APP 可看到二次确认机视频。
  - 任一侧挂断后 D2D、AIOT、WebRTC、bridge 状态都清理。

## 当前未提交文件概览

- 主要修改：
  - `native/aiot_bridge/kr_aiot_bridge.c`
  - `native/webrtc_bridge/kr_webrtc_bridge.c`
  - `native/indoor_bridge/kr_indoor_bridge.c`
  - `src/main.rs`
  - `src/runtime_coordinator/*`
  - `src/aiot.rs`
  - `src/db.rs`
  - `ui/MainPage.slint`
  - `ui/app.slint`
  - `ui/shared.slint`
  - `ui/GuardPhoneCall.slint`
  - `ui/SystemInformation.slint`
  - `readme.txt`
  - `lib/goblin_aiot.h`
  - `lib/libgoblin_aiot.so`

- 新增资源：
  - `assets/images/JPh5M.png`
  - `assets/images/Group_4.png`
  - `assets/images/x1tVq_red.png`
  - `images/Group 2.png`
  - `images/image_1.png`

- 注意：
  - 当前工作区不是干净状态。
  - 提交前必须重新确认哪些改动属于本次提交范围。
