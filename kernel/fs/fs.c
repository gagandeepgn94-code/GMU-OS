#include <gmu/fs.h>

static gmu_fs_node_t nodes[GMU_FS_MAX_NODES];

static bool valid_name(const char *name) {
    if (!name || !name[0])
        return false;

    if (name[0] == '.' &&
        (name[1] == '\0' ||
         (name[1] == '.' && name[2] == '\0')))
        return false;

    for (uint32_t i = 0; i < GMU_FS_NAME_MAX; i++) {
        if (name[i] == '\0')
            return true;

        if (name[i] == '/' || name[i] == '\\' || name[i] == ':' || name[i] == '*')
            return false;
    }

    return false;
}

static bool valid_parent(uint16_t parent) {
    return parent < GMU_FS_MAX_NODES &&
           nodes[parent].used &&
           nodes[parent].type == GMU_FS_DIRECTORY;
}

static bool str_equals(const char *a, const char *b) {
    while (*a && *b) {
        char ca = *a;
        char cb = *b;
        /* Case-insensitive comparison for convenient navigation */
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return false;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static bool name_exists(uint16_t parent,
                        const char *name,
                        uint16_t ignore_id) {
    for (uint16_t i = 0; i < GMU_FS_MAX_NODES; i++) {
        if (i == ignore_id || !nodes[i].used || nodes[i].parent != parent)
            continue;

        if (str_equals(nodes[i].name, name))
            return true;
    }

    return false;
}

bool gmu_fs_init(void) {
    for (uint32_t i = 0; i < GMU_FS_MAX_NODES; i++) {
        nodes[i].used = false;
        nodes[i].size = 0;
        nodes[i].parent = 0;
        nodes[i].type = GMU_FS_FILE;

        for (uint32_t j = 0; j < GMU_FS_NAME_MAX; j++)
            nodes[i].name[j] = '\0';
    }

    nodes[GMU_FS_ROOT_ID].used = true;
    nodes[GMU_FS_ROOT_ID].type = GMU_FS_DIRECTORY;
    nodes[GMU_FS_ROOT_ID].parent = GMU_FS_ROOT_ID;
    nodes[GMU_FS_ROOT_ID].name[0] = '/';
    nodes[GMU_FS_ROOT_ID].name[1] = '\0';

    return true;
}

int32_t gmu_fs_create(uint16_t parent,
                      const char *name,
                      gmu_fs_type_t type) {
    if (!valid_parent(parent) || !valid_name(name))
        return -1;

    if (type != GMU_FS_FILE && type != GMU_FS_DIRECTORY)
        return -1;

    if (name_exists(parent, name, GMU_FS_MAX_NODES))
        return -1;

    for (uint16_t i = 1; i < GMU_FS_MAX_NODES; i++) {
        if (!nodes[i].used) {
            nodes[i].used = true;
            nodes[i].type = type;
            nodes[i].parent = parent;
            nodes[i].size = 0;

            uint32_t j = 0;
            while (j < GMU_FS_NAME_MAX - 1 && name[j]) {
                nodes[i].name[j] = name[j];
                j++;
            }
            nodes[i].name[j] = '\0';

            return (int32_t)i;
        }
    }

    return -1;
}

bool gmu_fs_write(uint16_t id,
                  const void *data,
                  uint32_t size) {
    if (id >= GMU_FS_MAX_NODES ||
        !nodes[id].used ||
        nodes[id].type != GMU_FS_FILE ||
        size > GMU_FS_FILE_MAX_SIZE ||
        (size > 0 && !data))
        return false;

    const uint8_t *src = (const uint8_t *)data;

    for (uint32_t i = 0; i < size; i++)
        nodes[id].data[i] = src[i];

    nodes[id].size = size;
    return true;
}

bool gmu_fs_append(uint16_t id,
                   const void *data,
                   uint32_t size) {
    if (id >= GMU_FS_MAX_NODES ||
        !nodes[id].used ||
        nodes[id].type != GMU_FS_FILE ||
        (size > 0 && !data))
        return false;

    if (nodes[id].size + size > GMU_FS_FILE_MAX_SIZE)
        return false;

    const uint8_t *src = (const uint8_t *)data;

    for (uint32_t i = 0; i < size; i++)
        nodes[id].data[nodes[id].size + i] = src[i];

    nodes[id].size += size;
    return true;
}

bool gmu_fs_truncate(uint16_t id,
                     uint32_t new_size) {
    if (id >= GMU_FS_MAX_NODES ||
        !nodes[id].used ||
        nodes[id].type != GMU_FS_FILE ||
        new_size > GMU_FS_FILE_MAX_SIZE)
        return false;

    if (new_size < nodes[id].size) {
        for (uint32_t i = new_size; i < nodes[id].size; i++)
            nodes[id].data[i] = 0;
    }

    nodes[id].size = new_size;
    return true;
}

uint32_t gmu_fs_read(uint16_t id,
                     void *buffer,
                     uint32_t offset,
                     uint32_t size) {
    if (id >= GMU_FS_MAX_NODES ||
        !nodes[id].used ||
        nodes[id].type != GMU_FS_FILE ||
        !buffer ||
        offset >= nodes[id].size)
        return 0;

    if (size > nodes[id].size - offset)
        size = nodes[id].size - offset;

    uint8_t *dst = (uint8_t *)buffer;

    for (uint32_t i = 0; i < size; i++)
        dst[i] = nodes[id].data[offset + i];

    return size;
}

bool gmu_fs_delete(uint16_t id) {
    if (id == GMU_FS_ROOT_ID ||
        id >= GMU_FS_MAX_NODES ||
        !nodes[id].used)
        return false;

    if (nodes[id].type == GMU_FS_DIRECTORY) {
        for (uint16_t i = 1; i < GMU_FS_MAX_NODES; i++) {
            if (nodes[i].used && nodes[i].parent == id)
                return false; /* Cannot delete non-empty directory */
        }
    }

    nodes[id].used = false;
    nodes[id].size = 0;
    nodes[id].name[0] = '\0';

    return true;
}

bool gmu_fs_rename(uint16_t id, const char *new_name) {
    if (id == GMU_FS_ROOT_ID ||
        id >= GMU_FS_MAX_NODES ||
        !nodes[id].used ||
        !valid_name(new_name))
        return false;

    if (name_exists(nodes[id].parent, new_name, id))
        return false;

    uint32_t i = 0;
    while (i < GMU_FS_NAME_MAX - 1 && new_name[i]) {
        nodes[id].name[i] = new_name[i];
        i++;
    }

    nodes[id].name[i] = '\0';
    return true;
}

const gmu_fs_node_t *gmu_fs_get(uint16_t id) {
    if (id >= GMU_FS_MAX_NODES || !nodes[id].used)
        return 0;

    return &nodes[id];
}

uint16_t gmu_fs_count(uint16_t parent) {
    if (!valid_parent(parent))
        return 0;

    uint16_t count = 0;
    for (uint16_t i = 1; i < GMU_FS_MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].parent == parent)
            count++;
    }

    return count;
}

static int str_case_cmp(const char *a, const char *b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    while (*a && *b) {
        char ca = *a;
        char cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) {
            return (int)((unsigned char)ca) - (int)((unsigned char)cb);
        }
        a++;
        b++;
    }
    if (*a) return 1;
    if (*b) return -1;
    return 0;
}

static int str_cmp(const char *a, const char *b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (int)((unsigned char)*a) - (int)((unsigned char)*b);
}

static int fs_node_compare(uint16_t id_a, uint16_t id_b) {
    if (id_a == id_b) return 0;
    const gmu_fs_node_t *a = &nodes[id_a];
    const gmu_fs_node_t *b = &nodes[id_b];

    /* Folders (directories) always come before regular files */
    if (a->type != b->type) {
        return (a->type == GMU_FS_DIRECTORY) ? -1 : 1;
    }

    /* Alphabetical order (case-insensitive) */
    int c = str_case_cmp(a->name, b->name);
    if (c != 0) return c;

    /* Tie-breaker: case-sensitive comparison */
    int sc = str_cmp(a->name, b->name);
    if (sc != 0) return sc;

    return (int)id_a - (int)id_b;
}

uint16_t gmu_fs_get_children(uint16_t parent, uint16_t *out_ids, uint16_t max_out) {
    if (!valid_parent(parent) || !out_ids || max_out == 0) return 0;

    uint16_t count = 0;
    for (uint16_t i = 1; i < GMU_FS_MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].parent == parent) {
            out_ids[count++] = i;
            if (count >= max_out) break;
        }
    }

    /* Sort entries: folders first, then alphabetical */
    for (uint16_t i = 1; i < count; i++) {
        uint16_t key = out_ids[i];
        int j = (int)i - 1;
        while (j >= 0 && fs_node_compare(out_ids[j], key) > 0) {
            out_ids[j + 1] = out_ids[j];
            j--;
        }
        out_ids[j + 1] = key;
    }

    return count;
}

int32_t gmu_fs_get_child(uint16_t parent, uint16_t index) {
    if (!valid_parent(parent)) return -1;

    uint16_t children[GMU_FS_MAX_NODES];
    uint16_t count = gmu_fs_get_children(parent, children, GMU_FS_MAX_NODES);
    if (index >= count) return -1;

    return (int32_t)children[index];
}

int32_t gmu_fs_lookup(uint16_t parent, const char *name) {
    if (!valid_parent(parent) || !name) return -1;

    for (uint16_t i = 0; i < GMU_FS_MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].parent == parent && str_equals(nodes[i].name, name)) {
            return (int32_t)i;
        }
    }

    return -1;
}

int32_t gmu_fs_resolve_path(uint16_t base_dir, const char *path) {
    if (!path || !path[0]) return (int32_t)base_dir;

    uint16_t current = base_dir;
    uint32_t idx = 0;

    /* Absolute path check */
    if (path[0] == '/' || path[0] == '\\') {
        current = GMU_FS_ROOT_ID;
        idx++;
    }

    char component[GMU_FS_NAME_MAX];

    while (path[idx] != '\0') {
        while (path[idx] == '/' || path[idx] == '\\') idx++;
        if (path[idx] == '\0') break;

        uint32_t c_idx = 0;
        while (path[idx] != '\0' && path[idx] != '/' && path[idx] != '\\') {
            if (c_idx < GMU_FS_NAME_MAX - 1) {
                component[c_idx++] = path[idx];
            }
            idx++;
        }
        component[c_idx] = '\0';

        if (str_equals(component, ".")) {
            continue;
        } else if (str_equals(component, "..")) {
            current = nodes[current].parent;
        } else {
            int32_t next = gmu_fs_lookup(current, component);
            if (next < 0) return -1; /* Not found */
            current = (uint16_t)next;
        }
    }

    return (int32_t)current;
}

bool gmu_fs_get_path(uint16_t id, char *buffer, uint32_t max_len) {
    if (!buffer || max_len < 2 || id >= GMU_FS_MAX_NODES || !nodes[id].used) {
        return false;
    }

    if (id == GMU_FS_ROOT_ID) {
        buffer[0] = '/';
        buffer[1] = '\0';
        return true;
    }

    /* Trace backwards to root */
    uint16_t chain[16];
    int depth = 0;
    uint16_t curr = id;

    while (curr != GMU_FS_ROOT_ID && depth < 16) {
        chain[depth++] = curr;
        curr = nodes[curr].parent;
    }

    uint32_t pos = 0;
    for (int i = depth - 1; i >= 0; i--) {
        if (pos + 1 >= max_len) break;
        buffer[pos++] = '/';

        const char *name = nodes[chain[i]].name;
        while (*name && pos + 1 < max_len) {
            buffer[pos++] = *name++;
        }
    }

    buffer[pos] = '\0';
    return true;
}

uint32_t gmu_fs_total_used_bytes(void) {
    uint32_t total = 0;
    for (uint16_t i = 0; i < GMU_FS_MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].type == GMU_FS_FILE) {
            total += nodes[i].size;
        }
    }
    return total;
}

uint16_t gmu_fs_total_nodes_used(void) {
    uint16_t count = 0;
    for (uint16_t i = 0; i < GMU_FS_MAX_NODES; i++) {
        if (nodes[i].used) count++;
    }
    return count;
}

static void create_file_with_content(uint16_t parent, const char *name, const char *content) {
    int32_t id = gmu_fs_create(parent, name, GMU_FS_FILE);
    if (id >= 0 && content) {
        uint32_t len = 0;
        while (content[len]) len++;
        gmu_fs_write((uint16_t)id, content, len);
    }
}

void gmu_fs_load_default_data(void) {
    /* Create standard directories in alphabetical order */
    int32_t config = gmu_fs_create(GMU_FS_ROOT_ID, "Config", GMU_FS_DIRECTORY);
    int32_t docs = gmu_fs_create(GMU_FS_ROOT_ID, "Documents", GMU_FS_DIRECTORY);
    int32_t games = gmu_fs_create(GMU_FS_ROOT_ID, "Games", GMU_FS_DIRECTORY);

    /* Root files */
    create_file_with_content(
        GMU_FS_ROOT_ID, "README.TXT",
        "=========================================\n"
        "       GMU OS 64 - MODERN EDITION        \n"
        "=========================================\n\n"
        "Welcome to GMU OS 64, a 64-bit operating\n"
        "system developed at GM University.\n\n"
        "Features in this build:\n"
        " - UEFI Boot via Limine Protocol v3\n"
        " - Double-Buffered 32bpp Framebuffer\n"
        " - Physical Memory Manager & Dynamic Heap\n"
        " - Full In-Memory Hierarchical Filesystem\n"
        " - Modern Graphical Desktop & Dock UI\n"
        " - Interactive Shell with 20+ Commands\n"
        " - Dual-Pane File Manager with Text Viewer\n"
        " - GMU Strike 2D Space Arcade Game\n\n"
        "Type 'HELP' in the shell or use the mouse\n"
        "on the desktop dock to explore the OS!\n"
    );

    create_file_with_content(
        GMU_FS_ROOT_ID, "SYSTEM.LOG",
        "[INIT] UEFI GOP Framebuffer initialized.\n"
        "[INIT] IDT loaded with 256 gates.\n"
        "[INIT] PIC & PIT 100Hz timer calibrated.\n"
        "[INIT] Physical Memory Manager (PMM) active.\n"
        "[INIT] Kernel Dynamic Heap initialized (2MB).\n"
        "[INIT] RAMFS initialized with hierarchical VFS.\n"
        "[INIT] Applications registered successfully.\n"
        "[STATUS] System operational. All tests passed.\n"
    );

    /* Documents directory files */
    if (docs >= 0) {
        create_file_with_content(
            (uint16_t)docs, "ABOUT.TXT",
            "GM UNIVERSITY, DAVANAGERE, KARNATAKA\n"
            "Department of Computer Science & Engineering\n"
            "Course: UE25CS2304 - Operating System with Practice\n\n"
            "Project: GMU OS 64\n"
            "Architecture: x86-64 Freestanding Long Mode\n"
            "Boot Method: UEFI (Limine Bootloader v3)\n"
            "Target: Educational OS Design & Practice\n"
        );

        create_file_with_content(
            (uint16_t)docs, "COMMANDS.TXT",
            "AVAILABLE SHELL COMMANDS:\n"
            "  HELP     - Show list of commands\n"
            "  LS / DIR - List files in current directory\n"
            "  CD <dir> - Change directory (.., /, path)\n"
            "  PWD      - Print current working directory\n"
            "  CAT <f>  - View text content of a file\n"
            "  TOUCH <f>- Create an empty file\n"
            "  WRITE    - Write text into a file\n"
            "  APPEND   - Append text to a file\n"
            "  MKDIR <d>- Create a new directory\n"
            "  RM <f>   - Delete a file or empty folder\n"
            "  STAT <f> - View file/folder metadata\n"
            "  MEM      - Physical & heap memory statistics\n"
            "  UPTIME   - Show system uptime\n"
            "  SYSINFO  - Display system banner & specs\n"
            "  CALC     - Expression arithmetic calculator\n"
            "  DESKTOP  - Launch Graphical Desktop\n"
            "  STRIKE   - Launch GMU Strike space game\n"
            "  CLEAR    - Clear screen\n"
            "  REBOOT   - Restart the computer\n"
        );
    }

    /* Games directory files */
    if (games >= 0) {
        create_file_with_content(
            (uint16_t)games, "STRIKE.TXT",
            "GMU STRIKE - 2D SPACE SHOOTER\n\n"
            "How to play:\n"
            "  LEFT ARROW  - Move ship left\n"
            "  RIGHT ARROW - Move ship right\n"
            "  SPACEBAR    - Fire photon blaster\n"
            "  ESC         - Exit game back to OS\n\n"
            "Defend the galaxy and achieve high scores!\n"
        );
    }

    /* Config directory files */
    if (config >= 0) {
        create_file_with_content(
            (uint16_t)config, "SETTINGS.TXT",
            "THEME=MODERN_DARK\n"
            "ACCENT=BLUE_CYAN\n"
            "TIMER_HZ=100\n"
            "DOUBLE_BUFFER=ENABLED\n"
            "CURSOR=HARDWARE_STYLE\n"
        );
    }
}
