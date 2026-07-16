# Verification Report: PasswordError3 failure count fix

- Date: 2026-04-13
- Status: PASS
- Rust file: `src/main.rs`
- Report: `.claude/agent-memory/ui-layout-analyzer/layouts/PasswordError3-20260413-failure-count-fix.md`

## User Requirement

Entering `PasswordError3` is logically wrong; check whether the failure count calculation has a problem.

## Root Cause

`handle_password_failure()` was incrementing failures correctly. The issue was that generic `navigate(Home)` reset `password_failures` to 0. Since `PasswordError2` now returns to Home only via the bottom button, returning to Home after each non-locking password error reset the count and prevented attempts from accumulating to 3.

## Fix Summary

- Removed `password_failures = 0` from the generic `navigate(Home)` branch.
- Kept reset on correct password entry.
- Added reset when `PasswordError3` countdown reaches 0.
- Added reset when the user exits `PasswordError3` via the bottom Home button.

## Verified Flow

- Wrong attempt 1 -> `PasswordError2`, count 1.
- Return Home -> count remains 1.
- Wrong attempt 2 -> `PasswordError2`, count 2.
- Return Home -> count remains 2.
- Wrong attempt 3 -> `PasswordError3`, count 3.
- `PasswordError3` countdown end or Home button -> count resets to 0.

## Commands Run

```bash
cargo check
```

## Result

- `cargo check`: PASS.

## Notes

No Slint file needed to change for this specific logic fix; the issue was in Rust state management.
