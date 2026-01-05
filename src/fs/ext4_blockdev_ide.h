#ifndef EXT4_BLOCKDEV_H
#define EXT4_BLOCKDEV_H

#include "ext4/include/ext4_blockdev.h"
#include "../drivers/ide.h"

#define EXT4_IDE_BLOCKDEV_INSTANCE(__name, __disk)               \
    static struct ext4_blockdev_iface __name##_iface = {         \
        .open = ide_open,                                        \
        .bread = ide_bread,                                      \
        .bwrite = ide_bwrite,                                    \
        .close = ide_close,                                      \
        .lock = ide_lock,                                        \
        .unlock = ide_unlock,                                    \
        .ph_bsize = (__disk)->sector_size,                       \
        .ph_bcnt  = (__disk)->total_sectors,                     \
        .ph_bbuf = NULL,                                         \
        .ph_refctr = 0,                                          \
        .p_user = (void *)(__disk),                              \
    };                                                           \
    struct ext4_blockdev __name = {                              \
        .bdif = &__name##_iface,                                 \
        .part_offset = 0,                                        \
        .part_size = (__disk)->total_sectors * (__disk)->sector_size, \
    }

#endif // EXT4_BLOCKDEV_H