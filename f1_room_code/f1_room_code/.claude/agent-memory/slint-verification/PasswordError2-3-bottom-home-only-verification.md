# Slint Verification Report: PasswordError2 & PasswordError3 bottom-home-only fix

- Date: 2026-04-13
- Status: PASS
- Layout / interaction report: `.claude/agent-memory/ui-layout-analyzer/layouts/PasswordError2-3-20260413-bottom-home-only.md`
- Render PNGs:
  - `slint-screenshots/PasswordError2-bottom-home-only.png`
  - `slint-screenshots/PasswordError3-bottom-home-only.png`

## User Requirement

`PasswordError2` and `PasswordError3` background click return logic is wrong. Only clicking the bottom button should return to the home page.

## Fix Summary

- Removed full-screen background `TouchArea` from `PasswordError2.slint`.
- Removed full-screen background `TouchArea` from `PasswordError3.slint`.
- Added explicit bottom-button `TouchArea` to both pages at x=255.9573, y=1070.1293, width=288.0854, height=121.1118.
- Updated Rust fallback so `PasswordError2` returns to `Home` instead of `Container`.
- `PasswordError3` fallback remains `Home`; its countdown auto-return remains intact.

## Commands Run

```bash
slint-compiler -o /dev/null ui/pages/PasswordError2.slint
slint-compiler -o /dev/null ui/pages/PasswordError3.slint
slint-compiler -o /dev/null ui/workflow-generated.slint
cargo check
/usr/bin/pencil_to_slint ui/pages/PasswordError2.slint slint-screenshots/PasswordError2-bottom-home-only.png 800 1280
/usr/bin/pencil_to_slint ui/pages/PasswordError3.slint slint-screenshots/PasswordError3-bottom-home-only.png 800 1280
```

## Results

- `PasswordError2.slint`: PASS, 0 errors. Existing warning: exported Rectangle component does not inherit Window.
- `PasswordError3.slint`: PASS, 0 errors. Existing warning: exported Rectangle component does not inherit Window.
- `ui/workflow-generated.slint`: PASS, 0 errors.
- `cargo check`: PASS.
- Render checks: PASS.

## Source-Level Interaction Check

- No `clicked => { root.fallback_tapped(); }` full-screen background TouchArea remains in either page.
- The only Slint fallback trigger on each page is the bottom button TouchArea.

## Limitations

- The environment does not provide an `understand_image` verifier tool. I used `/usr/bin/pencil_to_slint` render output and source-level interaction verification as the available substitute.
