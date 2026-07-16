# 室内机 UI 交互流程说明

来源：
- `todo.md`：交互关系、跳转目标、按键效果目标。
- Pencil：`/home/dpower/data/KR_ROOM/室内机.pen`，用于确认界面名称、弹窗/键盘状态名称和部分按钮文字。

说明：
- 本文是后续 Slint 生成的交互逻辑依据。生成某个 Node ID 对应界面时，应优先查找本文中同名 Node ID 的章节。
- `跳转` 表示当前界面切换到目标界面。
- `效果` 表示按键点击/按下后的视觉状态或覆盖态。呼叫页按键释放后继续维持切换后的效果态，并锁定当前效果，不允许再次点击切换到其他效果。
- `弹窗` 表示在当前业务上下文上显示目标 Node ID 对应的弹窗/覆盖层样式。
- 未在 `todo.md` 或 Pencil 中确认的业务细节，不应臆造。

## 全局状态与建议回调

建议 Slint 根组件至少保留以下状态和回调入口：

- `in-out property <string> current-page`：当前页面 Node ID。
- `in-out property <string> active-overlay`：当前弹窗/蒙版/键盘 Node ID，空字符串表示无覆盖层。
- `in-out property <string> pressed-effect`：当前按键效果 Node ID，空字符串表示无按键效果。
- `in-out property <bool> call-effect-locked`：呼叫页效果是否已锁定；锁定后呼叫按键禁用，不再切换效果。
- `in-out property <bool> absent-mode-active`：外出模式黑屏蒙版是否显示。
- `in-out property <bool> password-visible`：密码是否明文显示。
- `in-out property <string> previous-page`：返回上一级页面时使用的来源页面 Node ID。
- `callback navigate(string target-node-id)`：页面跳转入口。
- `callback go-back()`：返回上一级；设置页、子设置页、弹窗和键盘均使用该入口返回。
- `callback show-overlay(string overlay-node-id)`：弹窗、键盘、成功/失败覆盖层入口。
- `callback close-overlay()`：关闭当前覆盖层入口。
- `callback show-pressed-effect(string effect-node-id)`：按键效果入口。
- `callback submit-password-change()`：提交密码修改，由业务侧返回成功或失败。
- `callback connect-wifi(string ssid, string password)`：WiFi 连接入口，由业务侧返回成功或失败。
- `callback reset-idle-timer()`：任意有效用户操作后重置 30 秒待机计时。

通用返回规则：
- 设置页、子设置页、弹窗、键盘的返回/关闭行为统一为返回上一级。
- 弹窗和键盘关闭时优先清空 `active-overlay`，回到打开它的页面。
- 子页面返回时使用 `previous-page` 或导航栈回到上一级页面。
- 呼叫页进入某个按键效果后，`pressed-effect` 保持目标效果 Node ID，`call-effect-locked = true`；同一呼叫页内其他呼叫按钮不可再次点击切换。

全局超时：
- 主界面 `LCvek / Main page` 无操作 30 秒后跳转 `VnIoT / Clock Mode - Background 1`。
- 其他界面是否也参与 30 秒待机，`todo.md` 未说明。

## 界面清单

| Node ID | Pencil 名称 | 用途 |
| --- | --- | --- |
| `LCvek` | Main page | 主界面 |
| `MwoLv` | Door camera call | 현관/门口呼叫 |
| `Bu0fW` | Lobby Phone call | Lobby 呼叫 |
| `EJDbm` | Guard Phone callig... | 경비실/警卫室呼叫 |
| `JVRSN` | CCTV Screen | CCTV |
| `hcEj1` | E/V Calling | 电梯呼叫 |
| `kKrEC` | Setting | 设置首页 |
| `NeeP0` | Capture image | 访客记录/抓拍图片 |
| `hEzDU` | Capture Full image | 抓拍大图 |
| `BqdKn` | General Setting | 通用设置 |
| `NKtaE` | Display Setting | 显示设置 |
| `HODTk` | Sound Setting | 声音设置 |
| `mcVw9` | Network Setting | 网络设置 |
| `FpPpi` | MODE Setting | 模式设置 |
| `gzCRD` | Etc. Setting | 其他设置 |
| `ITtGQ` | Password Change | 密码修改 |
| `VnIoT` | Clock Mode - Background 1 | 待机时钟页 |

## 主界面 `LCvek / Main page`

触发入口：
- `TxpXb`，按钮文字 `현관`：跳转 `MwoLv / Door camera call`。
- `HQQuJ`，按钮文字 `경비실`：跳转 `EJDbm / Guard Phone callig...`。
- `IbYr7`，按钮文字 `CCTV`：跳转 `JVRSN / CCTV Screen`。
- `vAbMH`，按钮文字 `외출모드`：进入外出模式，显示黑屏蒙版；再次点击或点击蒙版返回 `LCvek`。
- `6A3cC`，按钮文字 `엘리베이터`：跳转 `hcEj1 / E/V Calling`。
- `TmCEv`，按钮文字 `설정`：跳转 `kKrEC / Setting`。
- 无操作 30 秒：跳转 `VnIoT / Clock Mode - Background 1`。

Slint 状态建议：
- `absent-mode-active = true` 时显示全屏黑色蒙版。
- 外出模式返回方式在 todo 中描述为“点击返回主界面”，建议蒙版区域或外出模式按钮触发 `absent-mode-active = false` 并保持/返回 `LCvek`。

## CCTV 监视 `JVRSN / CCTV Screen`

入口：
- 从主界面 `IbYr7` 进入 `JVRSN`。
- 页面初始化调用 `cctv-open()` 启动当前摄像头监视。

IPC 来源：
- 进入 CCTV Screen、切换摄像头、截图时均从新的 `ipc_cameras.kv` 读取最新 IPC 信息，不使用应用启动时缓存的摄像头列表。
- 读取后刷新 `cctv-camera-options`，再按 `cctv-selected-camera` 选择对应摄像头启动 monitor。
- 如果没有选中索引或索引为 0，默认使用第一台可用 IPC。

## 门口呼叫 `MwoLv / Door camera call`

按键效果：
- `E3AYz`，按钮文字 `통화`：点击效果为 `7FHMR`。
- `T98lk`，按钮文字 `문열림`：点击效果为 `mqEIB`。
- `RtvoX`，按钮文字 `종료`：点击效果为 `D31Kv`。

Slint 状态建议：
- 使用 `pressed-effect` 保存 `7FHMR` / `mqEIB` / `D31Kv`。
- 点击后只切换到对应效果态，不触发通话、开门、挂断等设备命令。
- 效果态在释放后继续维持，且 `call-effect-locked = true` 禁止再次点击切换。

## Lobby 呼叫 `Bu0fW / Lobby Phone call`

按键效果：
- `7lxo1`：点击效果为 `vYZDx`。
- `8hxcK`：点击效果为 `jeBWl`。
- `7MfGl`：点击效果为 `q6PcB`。

Slint 状态建议：
- 使用 `pressed-effect` 保存目标效果 Node ID。
- 点击后只切换到对应效果态，不触发具体设备命令。
- 效果态在释放后继续维持，且 `call-effect-locked = true` 禁止再次点击切换。

## 警卫室呼叫 `EJDbm / Guard Phone callig...`

按键效果：
- `Aysc5`：点击效果为 `GuhoF`。
- `bqOrf`：点击效果为 `nRYdq`。

Slint 状态建议：
- 使用 `pressed-effect` 保存目标效果 Node ID。
- 点击后只切换到对应效果态，不触发具体设备命令。
- 效果态在释放后继续维持，且 `call-effect-locked = true` 禁止再次点击切换。

## 电梯呼叫 `hcEj1 / E/V Calling`

按键效果：
- `CsgUU`：点击效果为 `lkIXT`。
- `l7JTM`：点击效果为 `m9yPz`。

Slint 状态建议：
- 使用 `pressed-effect` 保存目标效果 Node ID。
- 点击后只切换到对应效果态，不触发具体设备命令。
- 效果态在释放后继续维持，且 `call-effect-locked = true` 禁止再次点击切换。

## 设置首页 `kKrEC / Setting`

菜单跳转：
- `RFJ0Z / VisitorRecordButton`：跳转 `NeeP0 / Capture image`。
- `uJKir`：跳转 `BqdKn / General Setting`。
- `4561U`：跳转 `NKtaE / Display Setting`。
- `APvob`：跳转 `HODTk / Sound Setting`。
- `nSeL5`：跳转 `mcVw9 / Network Setting`。
- `lgXpe`：跳转 `FpPpi / MODE Setting`。
- `Ia7D2`：跳转 `gzCRD / Etc. Setting`。

Slint 状态建议：
- 所有菜单项点击调用 `navigate(target-node-id)`。
- 返回按钮调用 `go-back()` 返回上一级。

## 抓拍记录 `NeeP0 / Capture image`

触发入口：
- `t0A5b / RecordCard`：跳转 `hEzDU / Capture Full image`。
- `Wfj5V`，按钮文字 `전체 삭제`：弹窗 `pHxTH / Cature image All Clear`。
- `OEFFi`，按钮文字 `개별 삭제`：弹窗 `1Negp / Cature image Select clear`。

Slint 状态建议：
- `active-overlay = "pHxTH"` 表示全部删除确认。
- `active-overlay = "1Negp"` 表示单条删除确认。
- 弹窗关闭或取消调用 `go-back()`，清空 `active-overlay` 后返回 `NeeP0`。
- 删除确认后的实际数据删除行为未说明。

## 网络设置 `mcVw9 / Network Setting`

WiFi 输入流程：
- WiFi 界面先使用模拟列表，后续再接入真实来源。
- 点击模拟 WiFi 列表项：弹出密码键盘，键盘样式为 `jbQzr / WIFI password_Caps off`。
- 在 `jbQzr` 中点击 `abMVY`，按钮文字 `!@#`：切换键盘样式为 `FmsBT / Password Keybord Change $%&#`。
- 在 `FmsBT` 中点击 `EI9Xp`，按钮文字 `ABC`：返回键盘样式 `jbQzr / WIFI password_Caps off`。
- 在 `jbQzr` 中点击 `FgtbO`，按钮文字 `Caps`：切换键盘样式为 `xMjyH / WIFI password_Caps ON`。
- `todo.md` 记录“点击 `xMjyH` 返回 `jbQzr`”，但未明确返回按钮 Node ID；生成时应保留从 `xMjyH` 返回 `jbQzr / WIFI password_Caps off` 的交互入口。
- WiFi 连接成功：弹窗样式 `de16y / WIFI Connect Success`。
- WiFi 连接失败：弹窗样式 `HZltp / WIFI Connect Fail`。

Slint 状态建议：
- `active-overlay` 保存键盘或连接结果弹窗 Node ID。
- `keyboard-mode` 可使用 `caps-off` / `caps-on` / `symbols`。
- WiFi 列表数据先使用本地模拟数据，保留后续替换为真实来源的接口。
- 连接结果由业务侧通过 `connect-wifi(...)` 后设置 `active-overlay = "de16y"` 或 `"HZltp"`。
- 键盘返回/关闭调用 `go-back()`，回到 `mcVw9`。

## WiFi 连接失败 `HZltp / WIFI Connect Fail`

触发入口：
- `gg7L1`，按钮文字 `다시 시도`：重新输入密码，返回 `jbQzr / WIFI password_Caps off`。
- `5wJ6J`，按钮文字 `취소`：返回 `mcVw9 / Network Setting`。

Slint 状态建议：
- `gg7L1` 设置 `active-overlay = "jbQzr"`。
- `5wJ6J` 清空 `active-overlay` 并显示/跳转 `mcVw9`。
- 失败弹窗关闭或返回调用 `go-back()`，返回上一级网络界面。

## 模式设置 `FpPpi / MODE Setting`

选择器：
- `FUUcG / TimeSelector`，当前值 `5분`：打开选择样式 `iOfBD / Absent Mode time`。
- `UoI1k / TimeSelector`，当前值 `3분`：打开选择样式 `qH43a / Communication time`。
- `y5miF / TimeSelector`，当前值 `3초`：打开选择样式 `tJvPk / 秒级选择器`。

取值参考：
- `iOfBD / Absent Mode time`：`1분`、`2분`、`3분`、`4분`、`5분`，Pencil 中 `3분` 为选中态。
- `qH43a / Communication time`：`1분`、`2분`、`3분`、`4분`、`5분`，Pencil 中 `3분` 为选中态。
- `tJvPk / 秒级选择器`：`1초`、`2초`、`3초`、`4초`、`5초`，Pencil 中 `3초` 为选中态。

Slint 状态建议：
- `active-overlay` 或 `active-picker` 保存当前展开的选择器。
- 每个选择器需要对应一个 `in-out` 属性保存当前值，例如 `absent-mode-time`、`communication-time`、`second-level-value`。
- 选择或返回/关闭后回到 `FpPpi`。

## 其他设置 `gzCRD / Etc. Setting`

触发入口：
- `86gws / SettingButton`：跳转 `ITtGQ / Password Change`。
- `UttiH / SettingButton`：弹窗 `uvgmF / System Infomation`。
- `qVOEf / SettingButton`：弹窗 `RFIGR / System Reset`。
- `MpUw5 / SettingRow`：跳转 `GUr03 / CCTV List`。

Slint 状态建议：
- `86gws` 调用 `navigate("ITtGQ")`。
- `UttiH` 设置 `active-overlay = "uvgmF"`。
- `qVOEf` 设置 `active-overlay = "RFIGR"`。
- `MpUw5` 设置 `previous-page = "gzCRD"` 并跳转 `GUr03`。
- 弹窗关闭或取消调用 `go-back()`，清空 `active-overlay` 后返回 `gzCRD`。
- 系统重置的确认/取消结果未在 todo 中说明。

## CCTV 列表 `GUr03 / CCTV List`

入口：
- 从 `MpUw5` 进入 `GUr03`，显示已注册 CCTV 列表。
- 点击列表成员：进入 `foYV2`，显示该 CCTV 的 IP、端口、账号、密码和已保存状态。
- 点击列表成员内 `CZq0T` 删除按钮：弹出 `Dr8p5` 删除确认弹窗。
- 点击 `zZElu`：进入正常添加界面 `UGcOs`。

删除弹窗：
- `Dr8p5`：显示摄像头删除确认。
- 取消关闭弹窗。
- 删除确认调用 `cctv-delete-setting-camera(index)` 删除当前待删除摄像头。

Slint 状态建议：
- `current-page = "GUr03" / "UGcOs" / "foYV2"` 表示列表、新增、已保存详情。
- `active-overlay = "Dr8p5"` 显示删除确认弹窗。
- `cctv-camera-options` 保存列表名称，`cctv-camera-details` 保存列表副标题。
- 默认 CCTV 名称使用 `CCTV1`、`CCTV2` 规则，不使用 `Camera 1`。
- 新增 CCTV 保存时追加到 `ipc_cameras.kv` 末尾，默认名称取最小未使用的 `CCTV{n}`，删除后再添加也不能与现有 CCTV 重名；从 `foYV2` 编辑已存在 CCTV 时只更新当前选中项，不清空其他 CCTV。
- `zZElu` 新增按钮按下效果使用 `Z76xmT`：背景与描边 `#155dfcff`，图标和文字为白色。
- `cctv-setting-delete-index` 保存待删除列表成员。

## CCTV 设置 `UGcOs / CCTV Setting`

输入入口：
- `Ia1RF`：IP 地址输入，弹窗 `pJGE2`。
  - IP 输入弹窗应按 `Q5kNvw` 效果提供 4 个可输入段区域，点号按键切换到下一段，当前段显示蓝色描边/底色。
- `ddmM7`：端口号保留界面显示，但 IPC 设置不需要端口号设置，禁用点击事件，不再弹出 `YBsES`。
- `aSKvN`：账号输入，弹窗 `KCRfp`，多键盘切换逻辑与 WiFi 密码输入一致。
- `EkZIL`：密码输入，弹窗 `vfuIh`，多键盘切换逻辑与 WiFi 密码输入一致。

连接测试：
- `O4p9d`：输入未完成时为禁用视觉。
- 当 IP、端口、账号、密码输入完成后，按钮切换为 `hohrT` 视觉。
- 点击 `hohrT`：弹窗显示连接状态，检测进行中的检查项效果按 `q5E1Bu`，检查项需要一项项推进，包含 IP 响应、ONVIF 服务、账号密码认证、摄像头信息、RTSP 流。
- `q5E1Bu` 弹窗地址显示只显示 IP，不显示端口号。
- 检测必须通过实际 IPC 监视验证：启动临时 IPC monitor，收到 `StreamReady` 才视为 ONVIF、账号密码和视频流全部检查通过，并切换为 `hMGBM`；收到 `Offline` / `Error` 或超时则保持失败/未通过样式 `E0zNY` 并显示错误信息。
- 保存成功后，`UGcOs` 页面显示为 `foYV2` 效果，即保留已输入内容并显示 `CCTV 저장됨`。

Slint 状态建议：
- `active-overlay = "pJGE2" / "YBsES" / "KCRfp" / "vfuIh" / "E0zNY" / "hMGBM"` 保存当前输入或测试弹窗。
- `cctv-setting-ip`、`cctv-setting-port`、`cctv-setting-account`、`cctv-setting-password` 保存当前设置值。
- `cctv-setting-input-target` 与 `cctv-setting-input-buffer` 保存当前弹窗输入上下文。
- `keyboard-mode` 沿用 WiFi 密码输入的 `lower` / `upper` / `symbols` 切换。
- `cctv-setting-tested` 表示连接测试是否通过；`cctv-setting-saved` 表示已保存并显示 `foYV2` 的保存状态。
- `cctv-setting-test-running` 表示正在验证；`cctv-setting-test-step` 表示当前检查项；`cctv-setting-test-error` 保存失败原因。
- 连接测试动作通过 `test-cctv-setting()` 交给业务侧启动 IPC monitor 验证。
- 保存动作通过 `save-cctv-setting()` 交给业务侧持久化。

## 密码修改 `ITtGQ / Password Change`

输入与提交：
- `zu9HV`：原密码输入框，弹窗显示键盘，效果为 `Q7BQ4 / Password Input`。
- `mSeYE`：新密码输入框，弹窗显示键盘，效果为 `Siq8B / Password Input`。
- `K7BNd`：切换密码隐藏模式与明文模式。
- `rexrj`，按钮文字 `비밀번호 변경`：提交修改。
- 提交成功：显示 `rIvrD / Password OK` 弹窗效果。
- 提交失败：显示 `CMf10 / Password NG` 弹窗效果。

Slint 状态建议：
- 使用 `active-overlay = "Q7BQ4"` 或 `"Siq8B"` 区分当前输入框键盘。
- `password-visible` 控制 `K7BNd` 对应的明文/隐藏显示。
- 先使用模拟校验：原密码固定为 `123456`。
- 点击 `rexrj` 时，如果 `zu9HV` 输入值不是 `123456`，设置失败并显示 `CMf10`。
- 如果原密码正确，则接受 `mSeYE` 中的新密码并显示 `rIvrD`；新密码的复杂度校验后续再补。
- 键盘、成功弹窗、失败弹窗关闭时调用 `go-back()`，返回上一级密码修改界面。

## 待确认项

- WiFi 真实数据来源、选中 SSID 显示方式和密码提交按钮 Node ID 后续补充；当前按模拟流程实现。
- 新密码复杂度、长度限制和保存方式后续补充；当前只校验原密码是否等于 `123456`。
- 系统重置、抓拍删除等确认后的实际数据操作后续补充；当前只保留弹窗和返回流程。
