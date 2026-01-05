#include "ext4_blockdev_ide.h"
#include "../drivers/ide.h"
#include <stddef.h>
#include <stdint.h>
#include "../libc/string.h"

static int ide_to_ext4_error(int rc) {
    if (rc == IDE_OK) return 0;
    if (rc == IDE_ERR_DEVICE) return -EIO;
    if (rc == IDE_ERR_TIMEOUT) return -EIO;
    if (rc == IDE_ERR_INVALID) return -EINVAL;
    return -EIO;
}

static int ide_open(struct ext4_blockdev *bdev) {
    return 0;
}

static int ide_close(struct ext4_blockdev *bdev) { return 0; }
static int ide_lock(struct ext4_blockdev *bdev) { return 0; }
static int ide_unlock(struct ext4_blockdev *bdev) { return 0; }
static int ide_bread(struct ext4_blockdev *bdev, void *buf, uint64_t blk_id, uint32_t blk_cnt) {
    ide_disk_t *disk = (ide_disk_t *)bdev->bdif->p_user;
    int block_size = bdev->bdif->ph_bsize;
    size_t total = blk_cnt * block_size / disk->sector_size;
    uint64_t lba = blk_id * (block_size / disk->sector_size);
    return ide_to_ext4_error(ide_read_sectors(disk, lba, total, buf));
}
static int ide_bwrite(struct ext4_blockdev *bdev, const void *buf, uint64_t blk_id, uint32_t blk_cnt) {
    ide_disk_t *disk = (ide_disk_t *)bdev->bdif->p_user;
    int block_size = bdev->bdif->ph_bsize;
    size_t total = blk_cnt * block_size / disk->sector_size;
    uint64_t lba = blk_id * (block_size / disk->sector_size);
    return ide_to_ext4_error(ide_write_sectors(disk, lba, total, buf));
}