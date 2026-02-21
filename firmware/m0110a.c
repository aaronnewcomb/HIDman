#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "m0110a.h"
#include "ps2.h"
#include "ch559.h"

static uint8_t HidToM0110AScan(uint8_t hid)
{
    switch (hid)
    {
        // Letters
        case 0x04: return 0x00; case 0x05: return 0x0B; case 0x06: return 0x08; case 0x07: return 0x02;
        case 0x08: return 0x0E; case 0x09: return 0x03; case 0x0A: return 0x05; case 0x0B: return 0x04;
        case 0x0C: return 0x22; case 0x0D: return 0x26; case 0x0E: return 0x28; case 0x0F: return 0x25;
        case 0x10: return 0x2E; case 0x11: return 0x2D; case 0x12: return 0x1F; case 0x13: return 0x23;
        case 0x14: return 0x0C; case 0x15: return 0x0F; case 0x16: return 0x01; case 0x17: return 0x11;
        case 0x18: return 0x20; case 0x19: return 0x09; case 0x1A: return 0x0D; case 0x1B: return 0x07;
        case 0x1C: return 0x10; case 0x1D: return 0x06;

        // Number row
        case 0x1E: return 0x12; case 0x1F: return 0x13; case 0x20: return 0x14; case 0x21: return 0x15;
        case 0x22: return 0x17; case 0x23: return 0x16; case 0x24: return 0x1A; case 0x25: return 0x1C;
        case 0x26: return 0x19; case 0x27: return 0x1D;

        // Editing/navigation
        case 0x28: return 0x24; // Enter
        case 0x29: return 0x35; // Esc
        case 0x2A: return 0x33; // Backspace
        case 0x2B: return 0x30; // Tab
        case 0x2C: return 0x31; // Space

        // Symbols
        case 0x2D: return 0x1B; case 0x2E: return 0x18; case 0x2F: return 0x21; case 0x30: return 0x1E; case 0x31: return 0x2A;
        case 0x33: return 0x27; case 0x34: return 0x29; case 0x35: return 0x2B; case 0x36: return 0x2F; case 0x37: return 0x2C; case 0x38: return 0x32;

        // Locks / function keys (subset)
        case 0x39: return 0x39; // CapsLock
        case 0x3A: return 0x7A; case 0x3B: return 0x78; case 0x3C: return 0x63; case 0x3D: return 0x76; case 0x3E: return 0x60; case 0x3F: return 0x61;
        case 0x40: return 0x62; case 0x41: return 0x64; case 0x42: return 0x65; case 0x43: return 0x6D; case 0x44: return 0x67; case 0x45: return 0x6F;

        // Arrows
        case 0x4F: return 0x42; case 0x50: return 0x48; case 0x51: return 0x4D; case 0x52: return 0x46;

        // Modifiers
        case 0xE0: return 0x36; case 0xE1: return 0x38; case 0xE2: return 0x3A; case 0xE3: return 0x37;
        case 0xE4: return 0x7B; case 0xE5: return 0x38; case 0xE6: return 0x7C; case 0xE7: return 0x37;

        default:
            return 0xFF;
    }
}

#define M0110A_QUEUE_SIZE 32
static __xdata uint8_t q[M0110A_QUEUE_SIZE];
static __xdata uint8_t qHead = 0;
static __xdata uint8_t qTail = 0;

static void enqueueRaw(uint8_t raw)
{
    uint8_t next = (qHead + 1) & (M0110A_QUEUE_SIZE - 1);
    if (next == qTail) return; // drop on overflow for now
    q[qHead] = raw;
    qHead = next;
}

void M0110AInit(void)
{
    qHead = 0;
    qTail = 0;
}

void M0110AEnqueueHidEvent(uint16_t hidcode, bool isBreak, bool isMedia)
{
    if (isMedia) return; // strict mode v1: ignore media page for now
    if (hidcode > 255) return;

    uint8_t scan = HidToM0110AScan((uint8_t)(hidcode & 0xFF));
    if (scan == 0xFF) return;

    // Raw code format: bit0 always 1, bit7=release, bit6..1=scan
    uint8_t raw = (uint8_t)((scan << 1) | 0x01);
    if (isBreak) raw |= 0x80;

    enqueueRaw(raw);
}

void M0110AProcessPort(void)
{
    // Protocol engine TODO:
    // - Host-initiated command receive (Inquiry/Instant/Model/Test)
    // - Keyboard-driven clock timing per M0110A spec
    // - Drain q[] and reply to Inquiry/Instant
    //
    // For this commit we only stage mode plumbing + event queue.
}
