# UGcOs CCTV Setting Verification

Date: 2026-05-07

## Files

- Slint page: `ui/CctvSetting.slint`
- Etc entry: `ui/EtcSetting.slint`
- Overlay routing: `ui/Overlays.slint`
- New overlays: `ui/CctvNumpadDialog.slint`, `ui/CctvTextInputDialog.slint`, `ui/CctvConnectionTestDialog.slint`
- Shared state: `ui/shared.slint`
- Backend persistence: `src/main.rs`, `src/db.rs`
- Exported assets: `assets/images/UBaad.png`, `assets/images/skpja.png`

## Verification Commands

- `slint-compiler -o /dev/null ui/app.slint` passed with 0 errors.
- `cargo check` passed. Remaining warnings are from existing external Slint/platform crates.
- `/usr/bin/pencil_to_slint ui/EtcSetting.slint slint-screenshots/EtcSetting-cctv.png 1024 600` passed.
- `/usr/bin/pencil_to_slint ui/CctvSetting.slint slint-screenshots/CctvSetting-UGcOs.png 1024 600` passed.
- `/usr/bin/pencil_to_slint ui/CctvConnectionTestDialog.slint slint-screenshots/CctvConnectionTestDialog.png 1024 600` passed.

## Visual Check

Available image inspection was manual screenshot review via exported PNGs; no dedicated `understand_image` tool is available in this environment.

- `EtcSetting-cctv.png`: matches Pencil `cBeFn` structure with five rows; CCTV entry appears at the bottom using exported CCTV icon.
- `CctvSetting-UGcOs.png`: matches the `UGcOs` base layout: header, ONVIF badge, four inputs, disabled connection test button.
- `CctvConnectionTestDialog.png`: matches `E0zNY` failure-state structure with dim overlay, status rows, error panel, close/retry buttons.

## Interaction Check

- `MpUw5` sets `previous-page` and navigates to `UGcOs`.
- `Ia1RF` opens `pJGE2`; `ddmM7` opens `YBsES`; `aSKvN` opens `KCRfp`; `EkZIL` opens `vfuIh`.
- Text keyboards support `lower`, `upper`, and `symbols` mode switching.
- Input submit writes back to the correct CCTV setting state and resets saved/tested state.
- Test button uses disabled `O4p9d` visuals until all required values are present; complete values use the `hohrT` active visual.
- Failure overlay `E0zNY` and success overlay `hMGBM` are routed through `OverlayLayer`.
- `save-cctv-setting()` persists `ipc.enabled`, `ipc.onvif_port`, and the first camera config, then enables the saved badge equivalent to `foYV2`.

## Result

PASS with one limitation: connection testing is represented as UI state only. No real ONVIF probe API exists in the current Rust IPC layer, so the retry path switches to the success dialog for UI verification and save flow coverage.
