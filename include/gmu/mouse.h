#ifndef GMU_MOUSE_H
#define GMU_MOUSE_H

#include <stdint.h>
#include <stdbool.h>
#include <gmu/framebuffer.h>

#define GMU_CURSOR_W 16
#define GMU_CURSOR_H 16

typedef struct {
    int32_t x;
    int32_t y;
    uint8_t buttons;
    bool initialized;
} gmu_mouse_t;

/* Initialize or re-bind mouse device with screen dimensions */
bool gmu_mouse_init(gmu_mouse_t *mouse,
                    uint64_t screen_width,
                    uint64_t screen_height);

/* Poll hardware mouse packets and update coordinates */
bool gmu_mouse_poll(gmu_mouse_t *mouse);

/* Set screen boundaries without resetting hardware */
void gmu_mouse_set_bounds(uint64_t screen_width, uint64_t screen_height);

/* Direct access to current global mouse state */
void gmu_mouse_get_coords(int32_t *x, int32_t *y);
void gmu_mouse_set_coords(int32_t x, int32_t y);
uint8_t gmu_mouse_get_buttons(void);

/* Universal Cursor Rendering & Double-buffering */
void gmu_mouse_cursor_show(gmu_framebuffer_t *fb, int32_t x, int32_t y);
void gmu_mouse_cursor_hide(gmu_framebuffer_t *fb);
void gmu_mouse_cursor_update(gmu_framebuffer_t *fb, int32_t x, int32_t y);
bool gmu_mouse_cursor_is_visible(void);

#endif
