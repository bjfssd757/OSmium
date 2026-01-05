#ifndef RAMDISK_H
#define RAMDISK_H

#include <stdint.h>

#define RAMDISK_SIZE (64 * 1024 * 1024)

uint8_t *ramdisk_base(void);

#endif // RAMDISK_H