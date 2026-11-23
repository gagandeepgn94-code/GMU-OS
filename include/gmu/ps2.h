#ifndef GMU_PS2_H
#define GMU_PS2_H

#include <stdint.h>
#include <stdbool.h>

void gmu_ps2_init(void);
void gmu_ps2_flush_mouse(void);
void gmu_ps2_flush_keyboard(void);

bool gmu_ps2_read_keyboard_byte(uint8_t *data);
bool gmu_ps2_read_mouse_byte(uint8_t *data);

bool gmu_ps2_mouse_command(uint8_t command);

#endif
