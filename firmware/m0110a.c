#include <stdint.h>
#include <stdbool.h>
#include "m0110a.h"
#include "ps2.h"

// 60kHz timer tick ~= 16.67us
#define M0110A_TICKS_40US   2
#define M0110A_TICKS_80US   5
#define M0110A_TICKS_160US 10
#define M0110A_TICKS_170US 10
#define M0110A_TICKS_180US 11
#define M0110A_TICKS_220US 13

#define M0110A_CMD_INQUIRY 0x10
#define M0110A_CMD_INSTANT 0x12
#define M0110A_CMD_MODEL   0x14
#define M0110A_CMD_TEST    0x16

#define M0110A_RESP_NULL      0x7B
#define M0110A_RESP_TEST_ACK  0x7D
#define M0110A_RESP_TEST_NAK  0x77
#define M0110A_MODEL_M0110A   0x0B

#define M0110A_QUEUE_SIZE 32

static __xdata uint8_t q[M0110A_QUEUE_SIZE];
static __xdata uint8_t qHead = 0;
static __xdata uint8_t qTail = 0;

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

static void enqueueRaw(uint8_t raw)
{
    uint8_t next = (qHead + 1) & (M0110A_QUEUE_SIZE - 1);
    if (next == qTail) return;
    q[qHead] = raw;
    qHead = next;
}

static bool dequeueRaw(uint8_t *out)
{
    if (qHead == qTail) return false;
    *out = q[qTail];
    qTail = (qTail + 1) & (M0110A_QUEUE_SIZE - 1);
    return true;
}

void M0110AInit(void)
{
    qHead = 0;
    qTail = 0;

    // Idle: both lines high
    WritePS2Clock(PORT_KEY, 1);
    WritePS2Data(PORT_KEY, 1);
}

void M0110AEnqueueHidEvent(uint16_t hidcode, bool isBreak, bool isMedia)
{
    if (isMedia) return;
    if (hidcode > 255) return;

    {
        uint8_t scan = HidToM0110AScan((uint8_t)(hidcode & 0xFF));
        uint8_t raw;
        if (scan == 0xFF) return;

        // bit0=1, bit7=break, bits6..1=scan
        raw = (uint8_t)((scan << 1) | 0x01);
        if (isBreak) raw |= 0x80;
        enqueueRaw(raw);
    }
}

typedef enum {
    M_IDLE = 0,
    M_RX_LOW,
    M_RX_HIGH,
    M_TX_PREP,
    M_TX_LOW,
    M_TX_HIGH,
} m0110_state_t;

static __xdata m0110_state_t mState = M_IDLE;
static __xdata uint8_t mTicks = 0;
static __xdata uint8_t mBit = 0;
static __xdata uint8_t mRx = 0;
static __xdata uint8_t mTx = 0;
static __xdata uint8_t mReqLowTicks = 0;

static void queueResponseForCommand(uint8_t cmd)
{
    uint8_t ev;
    switch (cmd)
    {
        case M0110A_CMD_INQUIRY:
        case M0110A_CMD_INSTANT:
            if (dequeueRaw(&ev)) mTx = ev;
            else mTx = M0110A_RESP_NULL;
            break;

        case M0110A_CMD_MODEL:
            mTx = M0110A_MODEL_M0110A;
            break;

        case M0110A_CMD_TEST:
            mTx = M0110A_RESP_TEST_ACK;
            break;

        default:
            mTx = M0110A_RESP_TEST_NAK;
            break;
    }
}

void M0110AProcessPort(void)
{
    switch (mState)
    {
        case M_IDLE:
            // Idle: both lines released high
            WritePS2Clock(PORT_KEY, 1);
            WritePS2Data(PORT_KEY, 1);

            // Host initiates by pulling DATA low while clock remains high.
            if (ReadPS2Clock(PORT_KEY) && !ReadPS2Data(PORT_KEY)) {
                if (mReqLowTicks < 255) mReqLowTicks++;
                if (mReqLowTicks >= M0110A_TICKS_160US) {
                    mReqLowTicks = 0;
                    mBit = 0;
                    mRx = 0;
                    mTicks = 0;
                    mState = M_RX_LOW;
                }
            } else {
                mReqLowTicks = 0;
            }
            break;

        // Receive host->keyboard command byte.
        case M_RX_LOW:
            WritePS2Clock(PORT_KEY, 0);
            if (++mTicks >= M0110A_TICKS_180US) {
                mTicks = 0;
                mState = M_RX_HIGH;
            }
            break;

        case M_RX_HIGH:
            WritePS2Clock(PORT_KEY, 1);
            mTicks++;

            // Sample 80us after rising edge.
            if (mTicks == M0110A_TICKS_80US) {
                mRx <<= 1;
                if (ReadPS2Data(PORT_KEY)) mRx |= 1;
            }

            if (mTicks >= M0110A_TICKS_220US) {
                mTicks = 0;
                mBit++;
                if (mBit >= 8) {
                    queueResponseForCommand(mRx);
                    mBit = 0;
                    mState = M_TX_PREP;
                } else {
                    mState = M_RX_LOW;
                }
            }
            break;

        // Send keyboard->host response byte, MSB first.
        case M_TX_PREP:
        {
            uint8_t bitVal = (uint8_t)((mTx & 0x80) ? 1 : 0);
            WritePS2Clock(PORT_KEY, 1);
            WritePS2Data(PORT_KEY, bitVal);
            if (++mTicks >= M0110A_TICKS_40US) {
                mTicks = 0;
                mState = M_TX_LOW;
            }
            break;
        }

        case M_TX_LOW:
            WritePS2Clock(PORT_KEY, 0);
            if (++mTicks >= M0110A_TICKS_160US) {
                mTicks = 0;
                mState = M_TX_HIGH;
            }
            break;

        case M_TX_HIGH:
            WritePS2Clock(PORT_KEY, 1);
            if (++mTicks >= M0110A_TICKS_170US) {
                mTicks = 0;
                mBit++;
                mTx <<= 1;

                if (mBit >= 8) {
                    mBit = 0;
                    WritePS2Data(PORT_KEY, 1);
                    mState = M_IDLE;
                } else {
                    mState = M_TX_PREP;
                }
            }
            break;

        default:
            mState = M_IDLE;
            break;
    }
}
