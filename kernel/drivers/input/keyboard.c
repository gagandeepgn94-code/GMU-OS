#include <stdint.h>
#include <stdbool.h>
#include <gmu/keyboard.h>
#include <gmu/ps2.h>

static const char normal_map[128] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3',
    [0x05] = '4', [0x06] = '5', [0x07] = '6',
    [0x08] = '7', [0x09] = '8', [0x0A] = '9',
    [0x0B] = '0', [0x0C] = '-', [0x0D] = '=',
    [0x0E] = '\b', [0x0F] = '\t',

    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e',
    [0x13] = 'r', [0x14] = 't', [0x15] = 'y',
    [0x16] = 'u', [0x17] = 'i', [0x18] = 'o',
    [0x19] = 'p', [0x1A] = '[', [0x1B] = ']',
    [0x1C] = '\n',

    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd',
    [0x21] = 'f', [0x22] = 'g', [0x23] = 'h',
    [0x24] = 'j', [0x25] = 'k', [0x26] = 'l',
    [0x27] = ';', [0x28] = '\'', [0x29] = '`',
    [0x2B] = '\\',

    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c',
    [0x2F] = 'v', [0x30] = 'b', [0x31] = 'n',
    [0x32] = 'm', [0x33] = ',', [0x34] = '.',
    [0x35] = '/', [0x37] = '*', [0x39] = ' ',

    /* Keypad without E0 prefix */
    [0x47] = '7', [0x48] = '8', [0x49] = '9', [0x4A] = '-',
    [0x4B] = '4', [0x4C] = '5', [0x4D] = '6', [0x4E] = '+',
    [0x4F] = '1', [0x50] = '2', [0x51] = '3',
    [0x52] = '0', [0x53] = '.',
};

static const char shift_map[128] = {
    [0x02] = '!', [0x03] = '@', [0x04] = '#',
    [0x05] = '$', [0x06] = '%', [0x07] = '^',
    [0x08] = '&', [0x09] = '*', [0x0A] = '(',
    [0x0B] = ')', [0x0C] = '_', [0x0D] = '+',
    [0x0E] = '\b', [0x0F] = '\t',

    [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E',
    [0x13] = 'R', [0x14] = 'T', [0x15] = 'Y',
    [0x16] = 'U', [0x17] = 'I', [0x18] = 'O',
    [0x19] = 'P', [0x1A] = '{', [0x1B] = '}',
    [0x1C] = '\n',

    [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D',
    [0x21] = 'F', [0x22] = 'G', [0x23] = 'H',
    [0x24] = 'J', [0x25] = 'K', [0x26] = 'L',
    [0x27] = ':', [0x28] = '"', [0x29] = '~',
    [0x2B] = '|',

    [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C',
    [0x2F] = 'V', [0x30] = 'B', [0x31] = 'N',
    [0x32] = 'M', [0x33] = '<', [0x34] = '>',
    [0x35] = '?', [0x37] = '*', [0x39] = ' ',

    [0x47] = '7', [0x48] = '8', [0x49] = '9', [0x4A] = '-',
    [0x4B] = '4', [0x4C] = '5', [0x4D] = '6', [0x4E] = '+',
    [0x4F] = '1', [0x50] = '2', [0x51] = '3',
    [0x52] = '0', [0x53] = '.',
};

static int extended_prefix = 0;
static bool shift_state = false;
static bool caps_lock = false;
static bool ctrl_state = false;
static bool alt_state = false;

void gmu_keyboard_init(void) {
    extended_prefix = 0;
    shift_state = false;
    caps_lock = false;
    ctrl_state = false;
    alt_state = false;
    gmu_ps2_flush_keyboard();
}

bool gmu_keyboard_shift_active(void) {
    return shift_state;
}

bool gmu_keyboard_caps_active(void) {
    return caps_lock;
}

uint16_t gmu_keyboard_read_key(void) {
    uint8_t code;

    if (!gmu_ps2_read_keyboard_byte(&code))
        return 0;

    if (code == 0xE0) {
        extended_prefix = 1;
        return 0;
    }

    /* Modifier Make and Break events */
    if (code == 0x2A || code == 0x36) {
        shift_state = true;
        return 0;
    }
    if (code == 0xAA || code == 0xB6) {
        shift_state = false;
        return 0;
    }
    if (code == 0x1D) {
        ctrl_state = true;
        return 0;
    }
    if (code == 0x9D) {
        ctrl_state = false;
        return 0;
    }
    if (code == 0x38) {
        alt_state = true;
        return 0;
    }
    if (code == 0xB8) {
        alt_state = false;
        return 0;
    }
    if (code == 0x3A) {
        caps_lock = !caps_lock;
        return 0;
    }

    /* Extended scancodes with 0xE0 prefix */
    if (extended_prefix) {
        extended_prefix = 0;

        /* Ignore break codes for extended keys */
        if (code & 0x80)
            return 0;

        switch (code) {
            case 0x48: return GMU_KEY_UP;
            case 0x50: return GMU_KEY_DOWN;
            case 0x4B: return GMU_KEY_LEFT;
            case 0x4D: return GMU_KEY_RIGHT;
            case 0x1C: return GMU_KEY_ENTER;
            case 0x53: return GMU_KEY_DELETE;
            case 0x47: return GMU_KEY_HOME;
            case 0x4F: return GMU_KEY_END;
            case 0x49: return GMU_KEY_PAGE_UP;
            case 0x51: return GMU_KEY_PAGE_DOWN;
            case 0x35: return '/';
            default:   return 0;
        }
    }

    /* Standard key release (break code) */
    if (code & 0x80) {
        return 0;
    }

    /* Function and control keys */
    if (code == 0x01) return GMU_KEY_ESCAPE;
    if (code == 0x0F) return GMU_KEY_TAB;
    if (code == 0x0E) return GMU_KEY_BACKSPACE;
    if (code == 0x1C) return GMU_KEY_ENTER;
    if (code == 0x3B) return GMU_KEY_F1;
    if (code == 0x3C) return GMU_KEY_F2;
    if (code == 0x3D) return GMU_KEY_F3;
    if (code == 0x3E) return GMU_KEY_F4;
    if (code == 0x3F) return GMU_KEY_F5;
    if (code == 0x40) return GMU_KEY_F6;
    if (code == 0x41) return GMU_KEY_F7;
    if (code == 0x42) return GMU_KEY_F8;
    if (code == 0x43) return GMU_KEY_F9;
    if (code == 0x44) return GMU_KEY_F10;
    if (code == 0x57) return GMU_KEY_F11;
    if (code == 0x58) return GMU_KEY_F12;

    if (code >= 128)
        return 0;

    char ch = normal_map[code];
    if (!ch)
        return 0;

    /* Casing logic: letters respond to shift XOR capslock */
    if (ch >= 'a' && ch <= 'z') {
        if (shift_state ^ caps_lock) {
            return (uint16_t)shift_map[code];
        }
        return (uint16_t)ch;
    }

    /* Symbols and numbers respond directly to Shift */
    if (shift_state) {
        char sch = shift_map[code];
        if (sch) return (uint16_t)sch;
    }

    return (uint16_t)ch;
}

char gmu_keyboard_read_char(void) {
    uint16_t key = gmu_keyboard_read_key();

    if (key == GMU_KEY_ENTER)
        return '\n';

    if (key == GMU_KEY_BACKSPACE)
        return '\b';

    if (key <= 0xFF)
        return (char)key;

    return 0;
}
