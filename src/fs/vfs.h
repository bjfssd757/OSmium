// vfs.h
#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

#define ssize_t long

#define VFS_PATH_MAX 512
#define VFS_MOUNT_PATH_MAX 64

#define VFS_O_RDONLY    0x0000
#define VFS_O_WRONLY    0x0001
#define VFS_O_RDWR      0x0002
#define VFS_O_CREAT     0x0100
#define VFS_O_TRUNC     0x0200
#define VFS_O_APPEND    0x0400

#define VFS_TYPE_FILE   0
#define VFS_TYPE_DIR    1

typedef struct vfs_dev {
    char name[32];
    void* blockdev;
} vfs_dev_t;

typedef struct vfs_mount {
    char mount_path[VFS_MOUNT_PATH_MAX];
    char dev_name[32];
    struct ext4_blockdev* blockdev;
    int read_only;
    struct vfs_mount* next;
} vfs_mount_t;

typedef struct vfs_file {
    int is_dir;
    void* handle;
    vfs_mount_t* mount;
    int flags;
    uint64_t size;
    uint64_t offset;
    struct vfs_file* next;
    char canonical_path[VFS_PATH_MAX];
} vfs_file_t;

typedef struct {
    uint64_t size;
    uint32_t type;
    uint32_t mode;
    uint64_t ctime, mtime;
} vfs_stat_t;

typedef struct {
    char name[256];
    uint32_t type;
    uint64_t size;
} vfs_dirent_t;

int vfs_register_device(const char* dev_name, struct ext4_blockdev* dev);
int vfs_mount(const char* dev_name, const char* mount_path, int read_only);
int vfs_umount(const char* mount_path);
vfs_mount_t* vfs_resolve_path(const char* path, char* out_rel);

int vfs_open(const char* path, int flags);
int vfs_opendir(const char* path);
ssize_t vfs_read(int fd, void* buffer, size_t count);
ssize_t vfs_write(int fd, const void* buffer, size_t count);
int vfs_close(int fd);
int vfs_stat(const char* path, vfs_stat_t* stat_buf);

int vfs_mkdir(const char* path, uint32_t mode);
int vfs_rmdir(const char* path);
int vfs_unlink(const char* path);

int vfs_readdir(int fd, vfs_dirent_t* dirent);

vfs_file_t* vfs_get_file(int fd);
int vfs_alloc_fd(void);
void vfs_free_fd(int fd);
void vfs_init(void);

#endif