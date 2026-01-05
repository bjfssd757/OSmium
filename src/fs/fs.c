#include "fs.h"
#include "vfs.h"

static ide_disk_t g_primary_master_disk;
static int g_disk_initialized = 0;

static void ensure_disk_init(void) {
    if (!g_disk_initialized) {
        if (ide_init(&g_primary_master_disk, IDE_CHANNEL_PRIMARY, 0) == IDE_OK) {
            g_disk_initialized = 1;
        }
    }
}

void fs_init(void) {
    vfs_init();
    
    ensure_disk_init();
}

ide_disk_t *get_primary_master_disk(void) {
    ensure_disk_init();
    if (g_disk_initialized) {
        return &g_primary_master_disk;
    }
    return NULL;
}