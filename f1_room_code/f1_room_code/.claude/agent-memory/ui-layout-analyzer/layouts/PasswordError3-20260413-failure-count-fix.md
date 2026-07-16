# Interaction Report: PasswordError3 failure count fix

- Date: 2026-04-13
- Target file: `src/main.rs`

## User Requirement

Entering `PasswordError3` has incorrect logic. Check whether the password failure count calculation is wrong.

## Finding

`handle_password_failure()` increments `password_failures` correctly and enters `PasswordError3` when the count reaches 3.

The actual bug was in `navigate(Home)`: it reset `password_failures` every time the app returned to Home. After the recent fix requiring `PasswordError2` / `PasswordError3` to return home only via the bottom button, each `PasswordError2 -> Home` return cleared the failure count. That prevented failure attempts from accumulating correctly toward `PasswordError3`.

## Fix

- Removed automatic `password_failures = 0` from generic `navigate(Home)`.
- Kept failure reset when the password is entered correctly.
- Reset failure count when `PasswordError3` countdown finishes.
- Reset failure count when the user exits `PasswordError3` via the bottom Home button.

## Expected Flow

1. Wrong password attempt 1 -> `password_failures = 1` -> `PasswordError2`.
2. Return Home -> count stays 1.
3. Wrong password attempt 2 -> `password_failures = 2` -> `PasswordError2`.
4. Return Home -> count stays 2.
5. Wrong password attempt 3 -> `password_failures = 3` -> `PasswordError3`.
6. Lock countdown completes or user presses Home on `PasswordError3` -> count resets to 0.
