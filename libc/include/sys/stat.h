#ifndef LIBC_SYS_STAT_H
#define LIBC_SYS_STAT_H

#include <stdint.h>

#define S_IFMT   0170000
#define S_IFSOCK 0140000
#define S_IFLNK  0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000

#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

struct stat {
    uint64_t  st_dev;
    uint64_t  st_ino;
    uint32_t  st_mode;
    uint32_t  st_nlink;
    uint32_t  st_uid;
    uint32_t  st_gid;
    uint64_t  st_rdev;
    int64_t   st_size;
    int64_t   st_atime;
    int64_t   st_mtime;
    int64_t   st_ctime;
    int64_t   st_blocks;
    uint32_t  st_blksize;
};

#endif // LIBC_SYS_STAT_H