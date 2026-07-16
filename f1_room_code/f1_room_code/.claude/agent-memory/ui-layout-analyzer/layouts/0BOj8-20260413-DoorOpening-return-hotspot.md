# UI Layout Analysis Report: 0BOj8 / DoorOpening return hotspot

- Pencil file: `/home/dpower/data/KR_DOOR/door_ui_2.pen`
- Node ID: `0BOj8`
- Name: `Container`
- Type: frame
- Size: 127.9327 x 127.9327
- Relative position in parent icon group: x=63.9582, y=517.0306
- Parent icon group position in generated Slint: x=272.0673, y=317.513
- Calculated page position: x=336.0255, y=834.5436
- Date: 2026-04-13

## Work.md Interaction Context

User correction: in `DoorOpening`, clicking the background must not return to the previous page. Only clicking Node ID `0BOj8` should return to the previous page.

## Visual Hierarchy Tree

```text
0BOj8 Container (frame, 127.9327x127.9327) @ parent (63.9582, 517.0306)
└── bBnLk Icon (frame/icon, 127.9327x127.9327, rotation=90) @ (0, 0)
    ├── cLdxm Vector (path, 106.6119x106.6119) @ (10.6611, 10.6611)
    └── BKBTL Vector (path, 106.6119x106.6119) @ (10.6611, 10.6611)
```

## Complete Element Coordinates Table

| # | ID | Name | Type | X rel | Y rel | W | H | Text |
|---|----|------|------|-------|-------|---|---|------|
| 1 | 0BOj8 | Container | frame | 63.9582 | 517.0306 | 127.9327 | 127.9327 | |
| 2 | bBnLk | Icon | frame/icon | 0 | 0 | 127.9327 | 127.9327 | |
| 3 | cLdxm | Vector | path | 10.6611 | 10.6611 | 106.6119 | 106.6119 | |
| 4 | BKBTL | Vector | path | 10.6611 | 10.6611 | 106.6119 | 106.6119 | |

## Detailed Properties

- `bBnLk` is already exported and used by `ui/pages/DoorOpening.slint` as `../../assets/images/icons/bBnLk.png`.
- Icon rotation in Pencil: 90deg.
- Vector strokes: `#a4f4cfff` and `#00bc7dff`, thickness 7.9959.

## Layout Observations

- Existing generated Slint wraps the full door-open graphic in a container at x=272.0673, y=317.513.
- The correct click hotspot should therefore be x=272.0673+63.9582=336.0255, y=317.513+517.0306=834.5436.
- The previous full-screen background TouchArea is incorrect and should be removed.
