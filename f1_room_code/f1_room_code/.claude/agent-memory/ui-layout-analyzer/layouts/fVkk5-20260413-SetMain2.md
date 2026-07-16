# UI Layout Analysis Report: fVkk5 / SetMain2

- Pencil file: `/home/dpower/data/KR_DOOR/door_ui_2.pen`
- Root Node ID: `fVkk5`
- Name: `SetMain2`
- Type: frame
- Size: 800 x 1280
- Canvas position: x=6567, y=-169 (canvas offset only, not copied into Slint)
- Generated for: SetMain factory reset popup behavior
- Date: 2026-04-13

## Work.md Interaction Context

`work.md` maps SetMain hotspot `fSFg6` to `恢复出厂设置 (43FBY)`. The current generated SetMain grid contains a bottom-middle visual label `数据同步备份`, but Pencil node `fVkk5` provides the popup visual state for `恢复出厂设置`. This implementation uses the `fSFg6` click area to show a local confirmation dialog instead of navigating away.

## Visual Hierarchy Tree

```text
fVkk5 SetMain2 (frame, 800x1280) @ canvas (6567, -169)
├── Ayh3h Button (frame, 177.7844x177.7844) @ (527.3945, 377.7842)
│   └── aQ5Fn Frame 960 (frame, 90.7277x117.1900) @ (43.8726, 30.4881)
│       ├── zIv3w lock-key-fill (frame/icon, 75.6064x75.6064) @ (7.5598, -0.0002)
│       └── JrIZv 开锁设置 (text) @ (-0.0016, 83.1667)
├── SFYrq Button (frame, 177.7844x177.7844) @ (95, 168) text=系统信息
├── NCPfO Button (frame, 177.7844x177.7844) @ (311.6104, 168) text=MAC信息
├── 5lElV Button (frame, 177.7844x177.7844) @ (527.3945, 168) text=地址设置
├── D9UOV Button (frame, 177.7844x177.7844) @ (95, 377.7842) text=音量设置
├── mAOGy Button (frame, 177.7844x177.7844) @ (311.6104, 377.7842) text=语言设置
├── rQXq1 Button (frame, 177.7844x177.7844) @ (95, 587.5684) text=门磁设置
├── if4UM Button (frame, 177.7844x177.7844) @ (311.6104, 587.5684) text=密码设置
├── GhjD5 Button (frame, 177.7844x177.7844) @ (527.3945, 587.5684) text=服务器设置
├── yZoDi Button (frame, 177.7844x177.7844) @ (95, 797.3525) text=恢复出厂设置
├── GAPVI Button (frame, 177.7844x177.7844) @ (311.6104, 797.3525) text=人脸识别
├── hUrCf Button (frame, 177.7844x177.7844) @ (527.3945, 797.3525) text=门禁卡设置
├── leUwt Button (frame, 178.6099x177.7984) @ (95, 1007.1367) text=硬件测试
├── O24yz Button (frame, 175.5795x177.7984) @ (312.6104, 1007.1367) text=数据同步备份
├── ewukS Button (frame, 175.5795x177.7984) @ (529.3945, 1007.1367) text=高级设置
├── UVg1a 系统设置 (text, 200x72) @ (300.2178, 51)
└── jFhZB calling (frame overlay, 800x1280) @ (0, 0)
    └── L37k4 Container (frame, 517x249) @ (141, 516)
        ├── FRno6 恢复出厂设置 (text, 157x31) @ (180.9245, 15)
        ├── RGrPS 否 (text, 27x38) @ (122, 179)
        ├── ZLPuu 是 (text, 27x38) @ (368.7157, 179)
        ├── Ye6Ow Line 22 (line, 64x0, rotation=90deg) @ (258, 233)
        ├── kQAih Line 22 (line, 430x0) @ (43, 159)
        └── uZW1b 是否恢复到出厂设置？ (text, 267x35) @ (125, 81)
```

## Complete Element Coordinates Table

| # | ID | Name | Type | X rel | Y rel | W | H | Text |
|---|----|------|------|-------|-------|---|---|------|
| 1 | fVkk5 | SetMain2 | frame | 6567 | -169 | 800 | 1280 | |
| 2 | jFhZB | calling | frame | 0 | 0 | 800 | 1280 | overlay |
| 3 | L37k4 | Container | frame | 141 | 516 | 517 | 249 | |
| 4 | FRno6 | 恢复出厂设置 | text | 180.9245 | 15 | 157 | 31 | 恢复出厂设置 |
| 5 | uZW1b | 是否恢复到出厂设置？ | text | 125 | 81 | 267 | 35 | 是否恢复到出厂设置？ |
| 6 | kQAih | Line 22 | line | 43 | 159 | 430 | 0 | |
| 7 | Ye6Ow | Line 22 | line | 258 | 233 | 64 | 0 | rotated vertical divider |
| 8 | RGrPS | 否 | text | 122 | 179 | 27 | 38 | 否 |
| 9 | ZLPuu | 是 | text | 368.7157 | 179 | 27 | 38 | 是 |
| 10 | O24yz | Button | frame | 312.6104 | 1007.1367 | 175.5795 | 177.7984 | 数据同步备份 |
| 11 | yZoDi | Button | frame | 95 | 797.3525 | 177.7844 | 177.7844 | 恢复出厂设置 |
| 12 | UVg1a | 系统设置 | text | 300.2178 | 51 | 200 | 72 | 系统设置 |

## Detailed Properties

- Root background: gradient, colors `#1e2c4dff` at 0%, `#2b374aff` at 50%, `#221e48ff` at 100%, plus base `#0f172bff`.
- Overlay `jFhZB`: fill `#0000004f`, size 800x1280.
- Dialog container `L37k4`: fill `#314158ff`, corner radius 21.3224, stroke thickness 1.3326.
- Dialog title `FRno6`: Source Han Sans CN, 26px, normal/400, fill `#ffffffff`, centered.
- Dialog message `uZW1b`: Source Han Sans CN, 26.653px, normal/400, fill `#ffffffff`, centered.
- Divider lines `kQAih` and `Ye6Ow`: stroke/fill `#dadadaff`, thickness 1.3326.
- Button labels `RGrPS`, `ZLPuu`: Source Han Sans CN, 26px, normal/400, fill `#ffffffff`, centered.

## Color Palette

- Background gradient: `#1e2c4dff`, `#2b374aff`, `#221e48ff`
- Button gradient: `#6f6bedff`, `#5c57ffff`
- Overlay: `#0000004f`
- Dialog fill: `#314158ff`
- Text: `#ffffffff`
- Divider: `#dadadaff`

## Typography Scale

- Page title: Source Han Sans CN, 50px, weight 500
- Grid item labels: Source Han Sans CN, 22.6819px, weight 400
- Dialog title/buttons: Source Han Sans CN, 26px, weight 400
- Dialog message: Source Han Sans CN, 26.653px, weight 400

## Layout Observations

- Root node canvas x/y is a design-canvas offset and must not be copied into Slint root.
- Popup overlay is topmost and covers the full page.
- Dialog is horizontally centered approximately at x=141 for width 517.
- No new icon export is required for the popup itself; existing SetMain icons are already referenced from `assets/images/icons/`.
- The vertical divider is represented as a 1.3326px wide Rectangle in Slint to avoid line rotation differences.
