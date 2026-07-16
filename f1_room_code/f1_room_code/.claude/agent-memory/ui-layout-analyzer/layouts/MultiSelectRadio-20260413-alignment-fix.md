# UI Layout / Interaction Report: Multi-select radio alignment fix

- Date: 2026-04-13
- Target files:
  - `ui/pages/LanguageSet.slint`
  - `ui/pages/UnlockSet.slint`
  - `ui/pages/DoorMagneticSet.slint`
  - `ui/pages/PwdSet.slint`
  - `ui/pages/ServerSet.slint`

## User Requirement

All mutually-exclusive option controls have the leading radio circle lower than the following text by about half a text height. Align the leading circle with the option text.

## Finding

The radio icons are 46px tall, while nearby text uses 26px font size with no explicit text box height. Existing generated coordinates put the icon at roughly text-y + 5px, making the icon center appear about 13px lower than the text visual center.

## Fix

Moved only radio option icons up by 13px:

- LanguageSet: option icon `y: 27px` -> `y: 14px`.
- UnlockSet: option icon `y: 27px` -> `y: 14px`.
- DoorMagneticSet: option icon `y: 27px` -> `y: 14px`.
- PwdSet: option icon `y: 96px` -> `y: 83px`.
- ServerSet: option icon `y: 125px` -> `y: 112px`.

## Scope Guard

Did not change slider thumbs, toggle switches, password dots, or other non-radio circular controls.
