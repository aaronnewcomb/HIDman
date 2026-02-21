# M0110A Migration Plan (Phase 1)

Goal: add strict Apple M0110A keyboard protocol output mode while preserving existing HIDman behavior.

## Scope (Phase 1)
- Add new keyboard mode for M0110A output.
- Strict host-command behavior (Inquiry/Instant/Model/Test).
- Model response: `0x0B`.
- Reuse existing pinout on one current PS/2 port for jumper-wire testing on existing hardware.
- Do not redesign PCB/jacks yet.

## Non-goals (Phase 1)
- ADB protocol.
- Extended "extras" behavior.
- New hardware routing.

## Implementation approach
1. Add mode constants and settings/menu wiring.
2. Add new low-level protocol engine (keyboard-clock-driven state machine).
3. Add M0110A event queue + command handling.
4. Add USB-HID -> M0110A keycode mapping table.
5. Add compatibility defaults and BAT/startup behavior.
6. Build + bench test iteration.

## Key constraints
- Keep existing PS/2/XT/Amstrad modes untouched.
- Keep serial mouse path untouched for now.
- Preserve watchdog/timer behavior in ISR path.

## Test checklist (initial)
- `/start` / reset behavior on host side.
- Inquiry returns null when idle and key transitions when pending.
- Instant returns immediate key transition.
- Model returns `0x0B`.
- Press/release for alpha keys, modifiers, enter/backspace, arrows.
- No regressions in existing modes.
