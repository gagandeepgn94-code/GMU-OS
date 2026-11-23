#ifndef GMU_FS_H
#define GMU_FS_H

#include <stdint.h>
#include <stdbool.h>

#define GMU_FS_MAX_NODES       128
#define GMU_FS_NAME_MAX        32
#define GMU_FS_FILE_MAX_SIZE   8192
#define GMU_FS_ROOT_ID         0

typedef enum {
    GMU_FS_FILE = 0,
    GMU_FS_DIRECTORY
} gmu_fs_type_t;

typedef struct {
    bool used;
    gmu_fs_type_t type;
    uint16_t parent;
    char name[GMU_FS_NAME_MAX];
    uint32_t size;
    uint8_t data[GMU_FS_FILE_MAX_SIZE];
} gmu_fs_node_t;

bool gmu_fs_init(void);

int32_t gmu_fs_create(
    uint16_t parent,
    const char *name,
    gmu_fs_type_t type
);

bool gmu_fs_write(
    uint16_t id,
    const void *data,
    uint32_t size
);

bool gmu_fs_append(
    uint16_t id,
    const void *data,
    uint32_t size
);

bool gmu_fs_truncate(
    uint16_t id,
    uint32_t new_size
);

uint32_t gmu_fs_read(
    uint16_t id,
    void *buffer,
    uint32_t offset,
    uint32_t size
);

bool gmu_fs_delete(uint16_t id);
bool gmu_fs_rename(uint16_t id, const char *new_name);

const gmu_fs_node_t *gmu_fs_get(uint16_t id);
uint16_t gmu_fs_count(uint16_t parent);
int32_t gmu_fs_get_child(uint16_t parent, uint16_t index);
uint16_t gmu_fs_get_children(uint16_t parent, uint16_t *out_ids, uint16_t max_out);
int32_t gmu_fs_lookup(uint16_t parent, const char *name);
int32_t gmu_fs_resolve_path(uint16_t base_dir, const char *path);
bool gmu_fs_get_path(uint16_t id, char *buffer, uint32_t max_len);

uint32_t gmu_fs_total_used_bytes(void);
uint16_t gmu_fs_total_nodes_used(void);
void gmu_fs_load_default_data(void);

#endif
