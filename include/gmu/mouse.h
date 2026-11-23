#ifndef GMU_MOUSE_H
#define GMU_MOUSE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int32_t x;
    int32_t y;
    uint8_t buttons;
    bool initialized;
} gmu_mouse_t;

bool gmu_mouse_init(gmu_mouse_t *mouse,
                    uint64_t screen_width,
                    uint64_t screen_height);

bool gmu_mouse_poll(gmu_mouse_t *mouse);

#endif
