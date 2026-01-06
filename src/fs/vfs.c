#include "vfs.h"
#include "ext4/include/ext4.h"
#include "../malloc/malloc.h"
#include "../libc/string.h"
#include "ext4/include/ext4_fs.h"
#include "ext4/include/ext4_inode.h"

#define VFS_MAX_FD 256

static vfs_mount_t* g_mounts = NULL;
static vfs_file_t* fd_table[VFS_MAX_FD];
static int g_next_fd = 3;
static vfs_dev_t device_table[8];
static int num_devices = 0;

void vfs_init(void) {
    memset(fd_table, 0, sizeof(fd_table));
    g_mounts = NULL;
    g_next_fd = 3;
    num_devices = 0;
}

int vfs_register_device(const char* dev_name, struct ext4_blockdev* dev) {
    if (num_devices >= (int)(sizeof(device_table)/sizeof(device_table[0]))) return -1;
    strncpy(device_table[num_devices].name, dev_name, sizeof(device_table[0].name)-1);
    device_table[num_devices].blockdev = dev;
    num_devices++;
    return 0;
}

int vfs_mount(const char* dev_name, const char* mount_path, int read_only) {
    struct ext4_blockdev* dev = NULL;
    for (int i = 0; i < num_devices; ++i) {
        if (strcmp(device_table[i].name, dev_name) == 0) {
            dev = device_table[i].blockdev;
            break;
        }
    }
    if (!dev) return -1;

    if (ext4_device_register(dev, dev_name) != 0) return -1;
    if (ext4_mount(dev_name, mount_path, !!read_only) != 0) return -2;

    vfs_mount_t* m = malloc(sizeof(vfs_mount_t));
    strncpy(m->mount_path, mount_path, VFS_MOUNT_PATH_MAX-1);
    strncpy(m->dev_name, dev_name, sizeof(m->dev_name)-1);
    m->blockdev = dev;
    m->read_only = read_only;
    m->next = g_mounts;
    g_mounts = m;
    return 0;
}

int vfs_umount(const char* mount_path) {
    vfs_mount_t **pptr = &g_mounts, *to_free = NULL;

    while (*pptr) {
        if (strcmp((*pptr)->mount_path, mount_path) == 0) {
            to_free = *pptr;
            *pptr = (*pptr)->next;
            break;
        }
        pptr = &(*pptr)->next;
    }
    if (!to_free) return -1;

    ext4_umount(mount_path);
    free(to_free);
    return 0;
}

vfs_mount_t* vfs_resolve_path(const char* path, char* out_rel) {
    size_t best_len = 0;
    vfs_mount_t* best = NULL;
    for (vfs_mount_t* m = g_mounts; m; m = m->next) {
        size_t l = strlen(m->mount_path);
        if (strncmp(path, m->mount_path, l) == 0) {
            if (path[l] == '/' || path[l] == '\0') {
                if (l > best_len) { best = m; best_len = l; }
            }
        }
    }
    if (best && out_rel) {
        size_t len = strlen(best->mount_path);
        if (strncmp(path, best->mount_path, len) == 0) {
            strcpy(out_rel, path+len);
            if (out_rel[0] == '/') memmove(out_rel, out_rel+1, strlen(out_rel+1)+1);
        } else out_rel[0] = 0;
    }
    return best;
}

int vfs_seek(int fb, uint64_t offset, uint32_t origin)
{
    vfs_file_t *f = vfs_get_file(fb);
    if (!f || f->is_dir) return -1;
    ext4_file *ext_file = f->handle;
    int res = ext4_fseek(ext_file, offset, origin);
    if (res == 0)
    {
        f->offset = ext4_ftell(ext_file);
        return 0;
    }
    return res;
}

int vfs_find(const char* path)
{
    char rel[VFS_PATH_MAX];
    vfs_mount_t* m = vfs_resolve_path(path, rel);
    if (!m) return -1;
    int fb = vfs_alloc_fd();
    if (fb < 0) return -1;

    vfs_file_t* f = malloc(sizeof(vfs_file_t));
    if (!f) return -1;
    memset(f, 0, sizeof(vfs_file_t));
    f->mount = m;
    strncpy(f->canonical_path, path, VFS_PATH_MAX-1);

    ext4_dir* d = malloc(sizeof(ext4_dir));
    if (d) {
        if (ext4_dir_open(d, path) == 0) {
            f->is_dir = 1;
            f->handle = d;
            fd_table[fb] = f;
            return fb;
        }
        free(d);
    }

    ext4_file* ef = malloc(sizeof(ext4_file));
    if (ef) {
        if (ext4_fopen2(ef, path, 0) == 0) {
            f->is_dir = 0;
            f->handle = ef;
            f->offset = 0;
            f->size = ext4_fsize(ef);
            f->flags = 0;
            fd_table[fb] = f;
            return fb;
        }
        free(ef);
    }

    free(f);
    return -1;
}

size_t vfs_get_file_size(const char *path)
{
    ext4_file *ef = malloc(sizeof(ext4_file));
    if (ef)
    {
        if (ext4_fopen2(ef, path, 0))
        {
            return ext4_fsize(ef);
        }
    }
    return 0;
}

int vfs_alloc_fd(void) {
    for (int i = 3; i < VFS_MAX_FD; ++i) {
        if (!fd_table[i]) return i;
    }
    return -1;
}

void vfs_free_fd(int fd) {
    if (fd < 3 || fd >= VFS_MAX_FD) return;
    if (fd_table[fd]) {
        free(fd_table[fd]);
        fd_table[fd] = NULL;
    }
}

vfs_file_t* vfs_get_file(int fd) {
    if (fd < 3 || fd >= VFS_MAX_FD) return NULL;
    return fd_table[fd];
}

int vfs_open(const char* path, int flags) {
    char rel[VFS_PATH_MAX];
    vfs_mount_t* m = vfs_resolve_path(path, rel);
    if (!m) return -1;
    int fd = vfs_alloc_fd();
    if (fd < 0) return -1;

    vfs_file_t* f = malloc(sizeof(vfs_file_t));
    memset(f, 0, sizeof(vfs_file_t));
    f->mount = m;
    strncpy(f->canonical_path, path, VFS_PATH_MAX-1);

    ext4_file* ext_file = malloc(sizeof(ext4_file));
    if (ext4_fopen2(ext_file, path, flags) != 0) {
        free(ext_file); free(f); return -1;
    }
    f->is_dir = 0;
    f->handle = ext_file;
    f->offset = 0;
    f->size = ext4_fsize(ext_file);
    f->flags = flags;
    fd_table[fd] = f;
    return fd;
}

int vfs_opendir(const char* path) {
    char rel[VFS_PATH_MAX];
    vfs_mount_t* m = vfs_resolve_path(path, rel);
    if (!m) return -1;
    int fd = vfs_alloc_fd();
    if (fd < 0) return -1;

    vfs_file_t* f = malloc(sizeof(vfs_file_t));
    memset(f, 0, sizeof(vfs_file_t));
    f->mount = m;
    strncpy(f->canonical_path, path, VFS_PATH_MAX-1);

    ext4_dir* extdir = malloc(sizeof(ext4_dir));
    if (ext4_dir_open(extdir, path) != 0) {
        free(extdir); free(f); return -1;
    }
    f->is_dir = 1;
    f->handle = extdir;
    fd_table[fd] = f;
    return fd;
}

ssize_t vfs_read(int fd, void* buffer, size_t count) {
    vfs_file_t* f = vfs_get_file(fd);
    if (!f || f->is_dir) return -1;
    size_t rcnt = 0;
    ext4_file* ext_file = f->handle;
    int res = ext4_fread(ext_file, buffer, count, &rcnt);
    if (res == 0) {
        f->offset += rcnt;
        return rcnt;
    }
    return res;
}

ssize_t vfs_write(int fd, const void* buffer, size_t count) {
    vfs_file_t* f = vfs_get_file(fd);
    if (!f || f->is_dir) return -1;
    size_t wcnt = 0;
    ext4_file* ext_file = f->handle;
    int res = ext4_fwrite(ext_file, buffer, count, &wcnt);
    if (res == 0) {
        f->offset += wcnt;
        return wcnt;
    }
    return res;
}

int vfs_close(int fd) {
    vfs_file_t* f = vfs_get_file(fd);
    if (!f) return -1;

    if (f->is_dir) {
        ext4_dir* d = f->handle;
        ext4_dir_close(d); free(d);
    } else {
        ext4_file* extfile = f->handle;
        ext4_fclose(extfile); free(extfile);
    }

    vfs_free_fd(fd);
    return 0;
}

int vfs_stat(const char* path, vfs_stat_t* st) {
    char rel[VFS_PATH_MAX];
    vfs_mount_t* m = vfs_resolve_path(path, rel);
    if (!m) return -1;

    struct ext4_inode inode;
    if (ext4_raw_inode_fill(path, 0, &inode) != 0) return -1;
    st->size = ext4_inode_get_size(&m->blockdev->fs->sb, &inode);
    st->mode = ext4_inode_get_mode(&m->blockdev->fs->sb, &inode);
    uint32_t type = ext4_inode_type(&m->blockdev->fs->sb, &inode);
    st->type = (type == EXT4_INODE_MODE_DIRECTORY)? VFS_TYPE_DIR : VFS_TYPE_FILE;
    st->ctime = ext4_inode_get_change_inode_time(&inode);
    st->mtime = ext4_inode_get_modif_time(&inode);
    return 0;
}

int vfs_readdir(int fd, vfs_dirent_t* dirent) {
    vfs_file_t* f = vfs_get_file(fd);
    if (!f || !f->is_dir) return -1;
    ext4_dir* d = f->handle;
    const ext4_direntry* de = ext4_dir_entry_next(d);
    if (!de) return 0;
    strncpy(dirent->name, (const char*)de->name, de->name_length);
    dirent->name[de->name_length] = 0;
    dirent->size = 0;
    dirent->type = (de->inode_type == EXT4_DE_DIR)? VFS_TYPE_DIR : VFS_TYPE_FILE;
    return 1;
}

int vfs_mkdir(const char* path, uint32_t mode) {
    return ext4_dir_mk(path);
}

int vfs_rmdir(const char* path) {
    return ext4_dir_rm(path);
}

int vfs_unlink(const char* path) {
    return ext4_fremove(path);
}
