#ifndef __M0110A_H__
#define __M0110A_H__

#include <stdint.h>
#include <stdbool.h>

void M0110AInit(void);
void M0110AProcessPort(void);

// Queue a key transition from USB HID keycode.
// isBreak=false for key press, true for key release.
void M0110AEnqueueHidEvent(uint16_t hidcode, bool isBreak, bool isMedia);

#endif
