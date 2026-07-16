# UI Layout / Interaction Report: PasswordError3 progress fix

- Date: 2026-04-13
- Target file: `ui/pages/PasswordError3.slint`
- Supporting files: `ui/workflow-generated.slint`, `src/main.rs`

## User Requirement

`PasswordError3` progress bar displays abnormally.

## Finding

The page visually shows remaining time as `9초`, but the progress bar fill was fixed at `440.1047px` and Rust returned from `PasswordError3` after 5 seconds. This made the progress indicator a static design artifact rather than a real countdown indicator.

## Fix Plan

- Add a `lock_seconds_remaining` property to `PasswordError3`.
- Bind the displayed remaining seconds to that property.
- Bind the progress fill width to `lock_seconds_remaining / 9`.
- Add a rounded fill radius to avoid a squared internal fill edge.
- Expose `lock_seconds_remaining` through `workflow-generated.slint`.
- Update Rust `PasswordError3` timer from a 5-second one-shot to a 9-second repeated countdown.
