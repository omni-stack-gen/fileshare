# UI Layout / Interaction Report: Container background hotspot fix

- Date: 2026-04-13
- Slint file: `ui/pages/Container.slint`
- Target page: `Container`

## User Requirement

Clicking the `Container` background must not enter password check/failure logic.

## Finding

`Container.slint` had a full-screen TouchArea at x=0, y=0, width=parent.width, height=parent.height. It emitted `root.fallback_tapped()`.

`src/main.rs` handled `Container` fallback by calling `handle_password_failure()`, which treated the background click as a password failure.

## Fix Plan

- Remove the full-screen background TouchArea from `Container.slint`.
- Keep all explicit keyboard TouchAreas: `password_digit_0..9`, `password_clear`, `password_backspace`, and `password_home`.
- Make `Container` fallback a no-op in Rust as a defensive guard.
