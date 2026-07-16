# Slint Verification Report: 0BOj8 / DoorOpening return hotspot

- Date: 2026-04-13
- Slint file: `ui/pages/DoorOpening.slint`
- Rust file: `src/main.rs`
- Layout report: `.claude/agent-memory/ui-layout-analyzer/layouts/0BOj8-20260413-DoorOpening-return-hotspot.md`
- Render PNG: `slint-screenshots/DoorOpening-0BOj8-hotspot.png`
- Status: PASS

## User Requirement

In `DoorOpening`, clicking the background should not return to the previous page. Only clicking Node ID `0BOj8` should return to the previous page.

## Commands Run

```bash
slint-compiler -o /dev/null ui/pages/DoorOpening.slint
slint-compiler -o /dev/null ui/workflow-generated.slint
cargo check
/usr/bin/pencil_to_slint ui/pages/DoorOpening.slint slint-screenshots/DoorOpening-0BOj8-hotspot.png 800 1280
```

## Compile Result

- `ui/pages/DoorOpening.slint`: PASS, 0 errors. Existing warning: exported Rectangle component does not inherit Window.
- `ui/workflow-generated.slint`: PASS, 0 errors.
- `cargo check`: PASS.

## Interaction Logic Check

- Removed the full-screen background TouchArea from `DoorOpening.slint`.
- Added a TouchArea only over `0BOj8` at x=336.0255, y=834.5436, width=127.9327, height=127.9327.
- Updated `src/main.rs` fallback handling: `DoorOpening` now calls `go_back_or_home()` instead of always navigating to `Home`.
- No additional background fallback path remains in `DoorOpening.slint`.

## Visual Check

- Rendered successfully with `/usr/bin/pencil_to_slint` at 800x1280.
- Existing icon image `bBnLk.png` remains unchanged and no new icon placeholder was introduced.

## Limitations

- The environment does not provide an `understand_image` verifier tool. I used `/usr/bin/pencil_to_slint` render output plus coordinate comparison from Pencil MCP as the available substitute.
