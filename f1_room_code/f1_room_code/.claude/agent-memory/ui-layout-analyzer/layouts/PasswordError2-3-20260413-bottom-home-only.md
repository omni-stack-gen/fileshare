# UI Layout / Interaction Report: PasswordError2 & PasswordError3 bottom-home-only fix

- Date: 2026-04-13
- Target files:
  - `ui/pages/PasswordError2.slint`
  - `ui/pages/PasswordError3.slint`
  - `src/main.rs`

## User Requirement

`PasswordError2` and `PasswordError3` background click return logic is wrong. Only clicking the bottom button should return to the home page.

## Fix

- Removed full-screen background `TouchArea` from both error pages.
- Added explicit bottom-button `TouchArea` at x=255.9573, y=1070.1293, width=288.0854, height=121.1118 to both pages.
- Changed Rust fallback behavior so `PasswordError2` now returns to `Home`, matching the bottom button text `홈으로`.
- `PasswordError3` continues returning to `Home` via fallback and also still has its countdown auto-return behavior.
