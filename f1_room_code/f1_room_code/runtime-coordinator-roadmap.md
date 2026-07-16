# KR_ROOM Runtime Coordinator 与事件队列整理路线

## Summary

本路线保留当前 Rust 主控 + Slint UI + Native bridge 适配层骨架，不推翻现有架构。目标是把 `src/main.rs` 中持续膨胀的跨模块联动收敛到 Rust coordinator，同时后续逐步整理 native 控制事件队列。

核心方向：

- Rust 继续作为主控层。
- Native bridge 只做 SDK/硬件适配，不互相直接调用。
- 跨模块业务联动统一放到 Rust coordinator。
- `main.rs` 后续只保留启动、服务装配、UI binding 等入口职责。

## 阶段 1：根目录计划文档

- 在项目根目录维护本文档：`runtime-coordinator-roadmap.md`。
- 文档用于记录 runtime coordinator、事件流、native 控制事件队列的整理路线。
- 后续每一阶段开始前，先根据实际代码状态更新本文档中的范围、验收点和风险。

## 阶段 2：抽出 Runtime Coordinator，不改变行为

新增 RuntimeCoordinator，把现有跨模块联动从 `main.rs` 迁出：

- AIOT WebRTC 账号下发 -> WebRTC 更新账号。
- AIOT 开锁 -> D2D 开锁。
- AIOT CallX 状态 -> 云端呼叫状态机。
- Indoor 二次确认机 online/offline -> AIOT proxy login/logout。
- Indoor 二次确认机呼入/挂断 -> AIOT proxy CallX。
- AIOT cloud hangup -> 本地 Indoor hangup。

实施要求：

- 把 `CloudCallController`、云端呼叫方向、proxy CallX 会话状态从 `main.rs` 移到 coordinator 相关模块。
- 第一轮不引入 Tokio。
- 第一轮不改变现有 dispatcher + `slint::invoke_from_event_loop` 事件处理方式。
- 第一轮只做结构迁移，保持用户可见行为不变。

## 阶段 3：建立 Rust AppEvent 单入口

新增统一事件类型：

- `AppEvent::Indoor`
- `AppEvent::Aiot`
- `AppEvent::Webrtc`
- `AppEvent::Ipc`
- `AppEvent::Wifi`

实施要求：

- 各 service dispatcher 只负责把事件转换为 `AppEvent` 并交给 coordinator。
- v1 使用单消费者 RuntimeCoordinator。
- 暂不做 Many-to-Many pub-sub。
- 后续如需多消费者，优先在 coordinator 内部 fan-out。

当前进度：

- 2026-05-28：阶段 3 第一轮已完成跨模块优先收敛。
- 本轮实际范围：AIOT、Indoor、WebRTC 事件已进入 `AppEvent` 单入口。
- IPC/WiFi 暂未迁移，延后为阶段 3b，避免扩大 CCTV/WiFi UI 改动面。
- 已验证：`git diff --check` 通过；`cargo build --target riscv64gc-unknown-linux-gnu --release` 通过。
- 上板已验证：AIOT 登录后 WebRTC 账号下发正常；二次确认机 online/offline 联动正常；二次确认机呼入触发 proxy CallX 正常。
- 上板待验证：云端挂断二次确认机 proxy CallX 带动本地 hangup；WebRTC/AIOT 开锁走 D2D unlock。
- 下一步：评估是否继续做阶段 3b，将 IPC/WiFi 纯 UI 事件也包装进 `AppEvent`，或直接进入阶段 4 拆分业务状态机。

## 阶段 4：拆分业务状态机

按业务拆分 coordinator 内部职责：

- 云端 CallX / WebRTC 呼叫。
- 二次确认机 proxy login 与 proxy CallX。
- IPC 监视与呼叫互斥策略。
- Slint UI 状态适配。

二次确认机 proxy CallX 使用明确状态机：

- `Idle`
- `LocalRinging`
- `CloudInitiating`
- `CloudRinging`
- `CloudConfirmed`
- `Terminating`

实施要求：

- UI callback 只表达用户动作，如 accept/reject/hangup。
- UI callback 不直接决定 AIOT/Indoor/WebRTC 联动细节。
- 复杂业务转移通过状态机表达，避免散落 bool 和临时 if。

当前进度：

- 2026-05-28：阶段 4 第一轮已完成 CallX 整体状态机拆分。
- 本轮实际范围：普通云端 CallX 与二次确认机 proxy CallX 会话状态已拆到 `runtime_coordinator/callx.rs`。
- 保持不变：RuntimeCoordinator 对外 public 方法、AIOT/Indoor/WebRTC 行为、native bridge、FFI、DB。
- 已验证：`rg -n "CloudCallDirection|CloudCallSessionState" src` 无结果；`git diff --check` 通过；`cargo build --target riscv64gc-unknown-linux-gnu --release` 通过。
- 上板已验证：二次确认机呼入触发 proxy CallX 正常；AIOT 登录后 WebRTC 账号下发正常；二次确认机 online/offline 联动正常。
- 上板待验证：云端挂断二次确认机 proxy CallX 带动本地 hangup；WebRTC/AIOT 开锁走 D2D unlock。
- 2026-05-28：阶段 4 第二轮已完成 IPC 监视与呼叫互斥策略拆分。
- 本轮实际范围：`callx-invited` 和 `page-change` 两处 IPC stop 策略已收敛到 `runtime_coordinator/media_policy.rs`。
- 保持不变：`spawn_ipc_stop_monitor` 执行方式、Slint UI 状态适配、native bridge、FFI、DB。
- 已验证：`rg -n "pub\\(crate\\) fn should_stop_ipc" src/runtime_coordinator` 有结果；`git diff --check` 通过；`cargo build --target riscv64gc-unknown-linux-gnu --release` 通过。
- 2026-05-28：阶段 4 第三轮已完成 Call UI 状态适配 helper 收敛。
- 本轮实际范围：CallX 事件处理和呼叫按钮回调中的 ringing、media active、idle、error、peer id 写入已改为调用 `call.rs` helper。
- 保持不变：协议字段、UI 文案、native bridge、FFI、DB；WebRTC/AIOT unlock、sync time、WiFi/Indoor 启动错误等非本轮状态写入暂未迁移。
- 已验证：`rg -n "apply_outgoing_ringing|apply_call_error|apply_call_idle|apply_cloud_ringing|apply_cloud_media_active" src/call.rs src/main.rs src/runtime_coordinator` 有结果；`git diff --check` 通过；`cargo build --target riscv64gc-unknown-linux-gnu --release` 通过。
- 上板已验证：二次确认机呼入、拒绝/挂断回 idle、proxy CallX confirmed、AIOT WebRTC 账号下发、二次确认机 online/offline 均正常。
- 下一步：评估是否继续做更广义 UI 状态适配，或进入阶段 5 native 控制事件队列整理。

## 阶段 5：整理 native 控制事件队列

在 `native/shared` 中新增轻量 control event queue helper。

优先迁移：

- AIOT 普通控制事件队列。
- IPC 普通控制事件队列。
- WebRTC 普通控制事件队列。

暂不迁移：

- WebRTC 音频 ring buffer。
- WebRTC 视频帧队列。
- Indoor 现有固定容量 ring queue。

实施要求：

- 普通控制事件队列保持现有 `wait_event` 语义。
- Indoor 暂时保留现有 drop-old ring queue，避免改变二次确认机和本地呼叫事件行为。
- 只有 helper 支持等价固定容量/drop-old 策略后，才考虑迁移 Indoor。

当前进度：

- 2026-05-28：阶段 5 第一轮已完成 AIOT/WebRTC 普通事件队列 helper 迁移。
- 本轮实际范围：新增 `native/shared/utils/control_event_queue.*`，AIOT/WebRTC 普通控制事件队列改用 shared helper。
- helper 约束：使用 `utils/list.h` 内核链表；使用 `pthread_condattr_setclock(CLOCK_MONOTONIC)` 和单调时钟超时；符号名不使用 `kr_control_` 前缀。
- 保持不变：IPC 普通事件队列、Indoor 固定容量 drop-old 队列、WebRTC 音频 ring buffer、WebRTC 视频帧队列。
- 已验证：`rg -n "kr_control_|kr_aiot_event_node|kr_webrtc_event_node|cancel_wait|kr_aiot_wait_locked|kr_webrtc_wait_locked|CLOCK_REALTIME" native/shared/utils/control_event_queue.* native/aiot_bridge/kr_aiot_bridge.c native/webrtc_bridge/kr_webrtc_bridge.c build.rs` 无结果；`git diff --check` 通过；`cargo build --target riscv64gc-unknown-linux-gnu --release` 通过。
- 下一步：阶段 5.2 单独评估 IPC 事件队列迁移，重点保持 `timeout_ms < 0` 无限等待和 `ERROR/OFFLINE` 后 monitor cleanup 语义。

## Test Plan

每个阶段完成后执行：

```bash
git diff --check
cargo build --target riscv64gc-unknown-linux-gnu --release
```

行为验收：

- AIOT 登录、WebRTC 账号下发、CallX 呼入/呼出保持不变。
- 二次确认机 online/offline 仍驱动 AIOT proxy login/logout。
- 二次确认机呼入仍触发 proxy CallX，`resident_locationid=201` 不变。
- proxy CallX confirmed 后不启动 WebRTC。
- 本地挂断、云端挂断、远端挂断都能正确清理会话。

## Assumptions

- 不推翻现有 Rust + Slint + Native bridge 骨架。
- 不在第一轮引入 Tokio、复杂 bus 框架或 C 侧 zbus。
- 跨模块业务耦合集中在 Rust coordinator，不下沉到 native bridge。
- 多消费者需求当前不是刚需；如果后续需要广播语义，优先在 Rust coordinator 内做 fan-out。
