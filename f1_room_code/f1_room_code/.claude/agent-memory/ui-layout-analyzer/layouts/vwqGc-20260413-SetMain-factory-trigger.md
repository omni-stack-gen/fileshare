# UI Layout Analysis Report: vwqGc / SetMain factory reset trigger

- Pencil file: `/home/dpower/data/KR_DOOR/door_ui_2.pen`
- Node ID: `vwqGc`
- Name: `Button`
- Type: frame
- Size: 177.7844 x 177.7844
- Position in SetMain: x=95, y=797.3525
- Date: 2026-04-13

## Work.md Interaction Context

User correction: `vwqGc` is the trigger for the factory reset confirmation popup. `fSFg6` should still navigate to `DataBackup`.

## Visual Hierarchy Tree

```text
vwqGc Button (frame, 177.7844x177.7844) @ (95, 797.3525)
└── IUhjz Frame 964 (frame, 90.7277x117.1900) @ (43.3609, 30.0239)
    ├── yWbga repeat-fill (frame/icon, 75.6064x75.6064) @ (7.5610, -0.0005)
    │   ├── qchFb Vector (path, 75.6064x75.6064) @ (-0.0005, -0.0005)
    │   └── da2yZ Vector (path, 61.4339x56.7085) @ (7.0857, 9.4474)
    └── ySn0Y 恢复出厂设置 (text, 137x33) @ (-22.6825, 83.1664)
```

## Complete Element Coordinates Table

| # | ID | Name | Type | X rel | Y rel | W | H | Text |
|---|----|------|------|-------|-------|---|---|------|
| 1 | vwqGc | Button | frame | 95 | 797.3525 | 177.7844 | 177.7844 | |
| 2 | IUhjz | Frame 964 | frame | 43.3609 | 30.0239 | 90.7277 | 117.1900 | |
| 3 | yWbga | repeat-fill | frame/icon | 7.5610 | -0.0005 | 75.6064 | 75.6064 | |
| 4 | qchFb | Vector | path | -0.0005 | -0.0005 | 75.6064 | 75.6064 | |
| 5 | da2yZ | Vector | path | 7.0857 | 9.4474 | 61.4339 | 56.7085 | |
| 6 | ySn0Y | 恢复出厂设置 | text | -22.6825 | 83.1664 | 137 | 33 | 恢复出厂设置 |

## Detailed Properties

- Button fill: linear gradient `#6f6bedff` -> `#5c57ffff`, rotation -180 in Pencil.
- Button corner radius: 24.2434.
- Button shadow: `#0000001a`, blur 5.3032 / 7.9549, y offsets 3.0304 / 6.0608.
- Label font: Source Han Sans CN, 22.6819px, normal/400, color `#ffffffff`, centered.
- Icon node is already exported and referenced in existing SetMain as `assets/images/icons/yWbga.png`.

## Layout Observations

- The existing visual button for `vwqGc` is already present in `ui/pages/SetMain.slint`; missing behavior was the TouchArea.
- New TouchArea should exactly cover x=95, y=797.3525, width=177.7844, height=177.7844 and set `factory_reset_dialog = true`.
- `fSFg6` is a separate bottom-middle button and should continue emitting `hotspot_clicked("fSFg6")` for DataBackup navigation.
