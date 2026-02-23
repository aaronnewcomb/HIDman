# HIDman M0110A Stable Release — v1.1.7-15

This release marks the M0110A mode as stable after iterative testing.

## Release artifacts
- `firmware/build/hidman-v1.1.7-15.hex`
- `firmware/build/hidman-v1.1.7-15.bin`

## Implemented behavior

### Protocol and timing reliability
- Corrected Macintosh command byte decoding:
  - Inquiry `0x10`
  - Instant `0x14`
  - Model `0x16`
  - Test `0x36`
- Added host-ready handshake step before transmit (wait for DATA high after command).
- Added timeout guard in `M_TX_WAIT_READY` to avoid rare long stalls.
- Removed non-spec host-attention pulse behavior.

### Typing correctness and stability
- Eliminated unintended repeat behavior in M0110A path by keeping typematic host-driven.
- Fixed punctuation mapping issues (`\`/`~`, `,`/`<`, `.`/`>`, `/`/`?`, `;`/`:` and `'`/`"` swaps).
- Added/validated key mappings for standard alphanumeric block.

### Modifier mapping for classic Mac use
- USB Ctrl (L/R) → Mac Command (L/R)
- USB Alt (L/R) → Mac Option (L/R)
- USB GUI/Win (L/R) → Control fallback

### Keypad behavior
- Enabled keypad support with practical mapping for digits/operators and Enter.
- Added explicit handling for KP `*` and KP `+` via synthesized shifted sequences to avoid ambiguous transition/keycode behavior.

### Intentional key disables for keyboard authenticity
Disabled in M0110A mode:
- F1–F12
- Print Screen
- Scroll Lock
- Pause/Break
- Insert
- Home
- End
- Page Up
- Page Down
- Forward Delete (USB Delete key in nav cluster)

### Caps Lock behavior
- Implemented latching-style Caps Lock behavior to emulate original physical lock key semantics:
  - press toggles latch state
  - release is ignored

### Cursor keys
- Mapped USB arrow keys to classic Mac cursor transitions (Up/Down/Left/Right).

## Files changed for release
- `firmware/m0110a.c`
- `firmware/processreport.c`

## Validation status
- Confirmed stable by manual testing in conversation.
