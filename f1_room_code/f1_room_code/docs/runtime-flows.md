# KR_ROOM 功能执行流程

本文使用 Mermaid 描述当前项目的主要执行流程。入口逻辑集中在 `src/main.rs`，UI 状态集中在 `ui/shared.slint`。

## 应用启动总流程

```mermaid
flowchart TD
    A[main] --> B{target_arch == riscv64}
    B -->|是| C[打开 SQLite 数据库]
    B -->|否| D[使用默认配置和 stub 后端]
    C --> E[初始化 G2D Slint 平台]
    D --> F[创建 AppWindow]
    E --> F
    F --> G[刷新系统时钟]
    G --> H[启动系统时钟 Timer]
    H --> I[启动主界面待机 Timer]
    I --> J[读取 Call/D2D/AIOT/WebRTC/IPC/WiFi 配置]
    J --> K[绑定 Call 与 D2D]
    K --> L[绑定数据库设置回调]
    L --> M[绑定截图记录]
    M --> N[绑定 IPC]
    N --> O[创建 WebRTC Runtime]
    O --> P[创建 AIOT Runtime]
    P --> Q[绑定呼叫动作路由]
    Q --> R[绑定媒体设置实时路由]
    R --> S[启动 WiFi Service]
    S --> T[绑定系统信息]
    T --> U[app.run]
```

## 页面导航流程

```mermaid
flowchart TD
    Main[LCvek Main page] --> Door[MwoLv Door camera call]
    Main --> Guard[EJDbm Guard Phone call]
    Main --> CCTV[JVRSN CCTV Screen]
    Main --> Absent[外出模式黑屏蒙版]
    Main --> EV[hcEj1 E/V Calling]
    Main --> Setting[kKrEC Setting]
    Main --> Clock[VnIoT Clock Mode]

    Absent --> Main
    Clock --> Main

    Setting --> Capture[NeeP0 Capture image]
    Setting --> General[BqdKn General Setting]
    Setting --> Display[NKtaE Display Setting]
    Setting --> Sound[HODTk Sound Setting]
    Setting --> Network[mcVw9 Network Setting]
    Setting --> Mode[FpPpi MODE Setting]
    Setting --> Etc[gzCRD Etc Setting]

    Capture --> Full[hEzDU Capture Full image]
    Full --> Capture

    Etc --> Password[ITtGQ Password Change]
    Etc --> SystemInfo[uvgmF System Information overlay]
    Etc --> Reset[RFIGR System Reset overlay]
```

## 主界面待机时钟流程

```mermaid
flowchart TD
    A[每秒 Timer tick] --> B{current-page == LCvek}
    B -->|否| Reset[清空 main_page_since]
    B -->|是| C{absent-mode-active == false}
    C -->|否| Reset
    C -->|是| D{active-overlay 为空}
    D -->|否| Reset
    D -->|是| E{clock-screen-enabled == true}
    E -->|否| Reset
    E -->|是| F{是否已有开始时间}
    F -->|否| G[记录当前 Instant]
    F -->|是| H{elapsed >= idle-clock-timeout-sec.max 60}
    H -->|否| End[继续等待]
    H -->|是| I[current-page = VnIoT]
```

## WiFi 连接与平台懒启动流程

```mermaid
sequenceDiagram
    participant UI as Slint NetworkSetting
    participant State as Slint State
    participant Main as src/main.rs
    participant Wifi as WifiService
    participant DB as SQLite
    participant AIOT as AiotRuntime
    participant WebRTC as WebrtcRuntime

    UI->>State: refresh-wifi / connect-wifi / wifi-set-enabled
    State->>Main: Rust callback
    Main->>Wifi: refresh_scan / set_enabled / connect
    Wifi-->>Main: WifiEvent
    Main->>State: apply_event_to_state

    alt ConnectSucceeded
        Main->>State: wifi-status=connected, active-overlay=de16y
        Main->>DB: save connected_ssid and wifi profile
        Main->>AIOT: start
        Main->>WebRTC: start
    else ConnectFailed
        Main->>State: wifi-status=failed, active-overlay=HZltp
    else Snapshot connected
        Main->>AIOT: start
        Main->>WebRTC: start
    end
```

## AIOT 与 WebRTC 账号流程

```mermaid
sequenceDiagram
    participant Wifi as WiFi connected
    participant Main as src/main.rs
    participant AIOT as AiotRuntime/AiotService
    participant Bridge as kr_aiot_bridge
    participant WebRTC as WebrtcRuntime
    participant D2D as D2dController
    participant State as Slint State

    Wifi->>Main: ConnectSucceeded or connected Snapshot
    Main->>AIOT: start()
    AIOT->>Bridge: kr_aiot_create + kr_aiot_start
    Bridge-->>AIOT: AIOT events
    AIOT-->>Main: AiotEvent

    alt WebrtcAccount event
        Main->>WebRTC: set_account(server_addr, serno, initstring)
        WebRTC->>WebRTC: recreate service with account
        WebRTC->>WebRTC: start if desired_started
    else SyncTime event
        Main->>Main: parse YYYYMMDDhhmmss
        Main->>State: refresh system clock
    else OpenLock event
        Main->>D2D: unlock_default_door
    else Error or non-zero status
        Main->>State: call-last-error
    end
```

## 云端 CallX + WebRTC 呼叫流程

```mermaid
flowchart TD
    Start{呼叫方向} --> Incoming[云端来电: AIOT CALLX_INVITED]
    Start --> Outgoing[本地主呼: State.call-start target]

    Incoming --> I1{mode == webrtc 且 WebRTC ready 且非 busy}
    I1 -->|否| IReject[send_callx_invited_ack able=false]
    I1 -->|是| IAcceptAck[send_callx_invited_ack able=true]
    IAcceptAck --> StopIPC{IPC 正在监视?}
    StopIPC -->|是| StopMonitor[异步 stop IPC monitor]
    StopIPC -->|否| Ringing
    StopMonitor --> Ringing[UI call-session-state=ringing]
    Ringing --> UserAccept[用户接听 State.call-accept]
    UserAccept --> SendAccept[send_callx_accept video/video]
    SendAccept --> Confirmed[CALLX_CONFIRMED]
    Confirmed --> Active[UI active + audio/video active]

    Outgoing --> O1{WebRTC ready 且非 busy}
    O1 -->|否| OErr[call-last-error]
    O1 -->|是| Initiate[send_callx_initiate]
    Initiate --> InitiateAck[CALLX_INITIATE_ACK]
    InitiateAck --> Invite[send_callx_invite mode=webrtc]
    Invite --> Replied[CALLX_REPLIED]
    Replied --> Confirmed2[CALLX_CONFIRMED]
    Confirmed2 --> WebrtcCall[webrtc.call session_id/to]
    WebrtcCall --> Active

    Active --> Hangup[本地挂断或 CALLX_HUNGUP]
    Hangup --> Cleanup[关闭 WebRTC session, 清理 UI, 回主界面]
```

## 本地 Call 事件流程

```mermaid
sequenceDiagram
    participant Native as call_adapter
    participant Call as CallService worker
    participant Main as src/main.rs
    participant State as Slint State
    participant UI as 呼叫页面

    Native-->>Call: kr_call_event_t
    Call-->>Main: CallEvent
    Main->>State: call::apply_event_to_state

    alt Incoming
        State->>State: clear overlay/effects
        State->>State: call-session-state=ringing
        State->>State: current-page=incoming-call-page
    else StateChanged
        State->>State: update call-session-state
        State->>State: idle 时 cleanup_call_ui
    else MediaChanged
        State->>State: update audio/video active
    else RemoteHangup or Error
        State->>State: cleanup_call_ui and go LCvek
    end

    UI->>State: call-accept / call-reject / call-hangup
    State->>Main: callback
    Main->>Call: accept / reject / hangup
```

## D2D 开锁与密码修改流程

```mermaid
flowchart TD
    A{入口} --> B[用户点击开门 State.call-open-door]
    A --> C[AIOT OpenLock 事件]
    A --> D[WebRTC Unlock 事件]
    B --> E[D2D unlock_default_door]
    C --> E
    D --> E
    E --> F{D2D 事件}
    F -->|UnlockDone| G[记录成功日志]
    F -->|UnlockFail/Timeout| H[call-last-error=unlock_failed]

    P[密码修改提交] --> Q{旧密码和新密码均 6 位 且旧密码 hash 匹配}
    Q -->|否| R[active-overlay=CMf10 密码错误]
    Q -->|是| S[D2D set_unlock_password]
    S --> T{D2D SetPassword 事件}
    T -->|Done| U[保存明文和 hash 到 DB]
    U --> V[active-overlay=rIvrD 成功, 清空输入]
    T -->|Fail/Timeout| R
```

## IPC CCTV 监视与截图流程

```mermaid
sequenceDiagram
    participant UI as CCTVScreen
    participant State as Slint State
    participant Main as src/main.rs
    participant IPC as IpcController
    participant Native as ipc_bridge
    participant Records as CaptureRecords

    UI->>State: init => cctv-open
    State->>Main: on_cctv_open
    Main->>Main: resolve selected camera
    Main->>IPC: spawn start_monitor(camera)
    IPC->>Native: kr_ipc_start_monitor
    Native-->>IPC: MonitorStarted / StreamReady / Offline / Error
    IPC-->>Main: IpcEvent
    Main->>State: update online/error/current camera

    UI->>State: cctv-select-camera(index)
    State->>Main: on_cctv_select_camera
    Main->>IPC: if page is JVRSN, spawn start_monitor(new camera)

    UI->>State: cctv-capture
    State->>Main: on_cctv_capture
    Main->>Records: new_capture_output_path
    Main->>IPC: capture_jpeg(path)
    IPC->>Native: kr_ipc_capture_jpeg
    Native-->>IPC: CaptureSaved
    IPC-->>Main: IpcEvent
    Main->>State: cctv-last-capture-path

    UI->>State: cctv-stop / leave page
    State->>Main: on_cctv_stop
    Main->>IPC: spawn stop_monitor
```

## 截图记录流程

```mermaid
flowchart TD
    A[配置 capture_dir] --> B[CaptureGallery refresh]
    B --> C[扫描 jpg/jpeg/png]
    C --> D{文件名是否为 unread__/read__ 规则}
    D -->|是| E[解析 label/timestamp/unread]
    D -->|否| F[读取文件 metadata timestamp, label=CCTV]
    E --> G[按 timestamp 倒序]
    F --> G
    G --> H[同步每页 6 条到 Slint State]

    H --> I[打开记录]
    I --> J[设置 full_index]
    J --> K{unread?}
    K -->|是| L[rename unread__ -> read__]
    K -->|否| M[进入大图页]
    L --> M

    H --> N[选择记录]
    N --> O[删除选中]
    M --> P[上一张/下一张/删除当前]
    H --> Q[删除全部]
```

## 设置保存流程

```mermaid
sequenceDiagram
    participant UI as Setting Slint pages
    participant State as Slint State
    participant Main as src/main.rs
    participant DB as SQLite settings
    participant Call as CallController
    participant IPC as IpcController

    UI->>State: save-int/save-float/save-string/save-bool
    State->>Main: database callback
    Main->>DB: set_setting(key, value, value_type)

    UI->>State: float-setting-live-changed
    State->>Main: media live callback

    alt call volume key
        Main->>Call: set_playback_volume(value)
    else cctv.brightness
        Main->>IPC: set_preview_brightness(value)
    end

    UI->>State: set-system-time
    State->>Main: parse selected time/date
    Main->>Main: clock_settime
    Main->>State: refresh system clock
```

## 构建后端选择流程

```mermaid
flowchart TD
    A[build.rs] --> B{TARGET contains riscv64}
    B -->|否| StubAll[启用 D2D/AIOT/WebRTC/IPC/WiFi stub bridge]
    B -->|是| C[定位 lib 与 mtrans]
    C --> D{mtrans/call 运行库可用?}
    D -->|否| CallStub[Call/D2D/AIOT 走 stub, WebRTC/IPC 按库可用性选择]
    D -->|是| RealCall[编译真实 call_adapter 与 shared PLC runtime]
    RealCall --> E{D2D 库可用?}
    E -->|是| RealD2D[真实 D2D bridge]
    E -->|否| StubD2D[D2D stub]
    RealCall --> F{WiFi 运行库可用?}
    F -->|是| RealWifi[链接 dplib_wifi/dplib_net]
    F -->|否| StubWifi[WiFi stub]
    RealCall --> G{AIOT 运行库可用?}
    G -->|是| RealAIOT[真实 AIOT bridge]
    G -->|否| StubAIOT[AIOT stub]
    RealCall --> H{WebRTC 运行库可用?}
    H -->|是| RealWebRTC[真实 WebRTC bridge]
    H -->|否| StubWebRTC[WebRTC stub]
    RealCall --> I{IPC 运行库可用?}
    I -->|是| RealIPC[真实 IPC bridge]
    I -->|否| StubIPC[IPC stub]
```
