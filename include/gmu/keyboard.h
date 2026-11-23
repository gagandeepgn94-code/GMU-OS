#ifndef GMU_KEYBOARD_H
#define GMU_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

#define GMU_KEY_UP        0x100
#define GMU_KEY_DOWN      0x101
#define GMU_KEY_LEFT      0x102
#define GMU_KEY_RIGHT     0x103
#define GMU_KEY_ENTER     0x104
#define GMU_KEY_ESCAPE    0x105
#define GMU_KEY_F1        0x106
#define GMU_KEY_F2        0x107
#define GMU_KEY_F3        0x108
#define GMU_KEY_F4        0x109
#define GMU_KEY_TAB       0x10A
#define GMU_KEY_BACKSPACE 0x08
#define GMU_KEY_DELETE    0x10C
#define GMU_KEY_HOME      0x10D
#define GMU_KEY_END       0x10E
#define GMU_KEY_PAGE_UP   0x10F
#define GMU_KEY_PAGE_DOWN 0x110
#define GMU_KEY_F5        0x111
#define GMU_KEY_F6        0x112
#define GMU_KEY_F7        0x113
#define GMU_KEY_F8        0x114
#define GMU_KEY_F9        0x115
#define GMU_KEY_F10       0x116
#define GMU_KEY_F11       0x117
#define GMU_KEY_F12       0x118

void gmu_keyboard_init(void);
uint16_t gmu_keyboard_read_key(void);
char gmu_keyboard_read_char(void);
bool gmu_keyboard_shift_active(void);
bool gmu_keyboard_caps_active(void);

#endif
