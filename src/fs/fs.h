// fs.h
#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stddef.h>
#include "../drivers/ide.h"

void fs_init(void);
ide_disk_t *get_primary_master_disk(void);

#endif // FS_H