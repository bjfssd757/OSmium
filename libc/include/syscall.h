#ifndef SYSCALL_H
#define SYSCALL_H

// Соглашение о вызовах для x86-64:
// rax: номер syscall
// rdi, rsi, rdx, r10, r8, r9: аргументы 1-6

// --- Обертки для системных вызовов ---

// Замечание: GCC inline-ассемблер - мощный, но сложный.
// "memory" в clobber-листе говорит компилятору, что ассемблерный код
// может читать или писать в произвольную память, что заставляет его
// сбросить кешированные значения из регистров в память перед вызовом
// и перезагрузить их после.

#include "sys/types.h"
#include <stdint.h>
#include <stddef.h>

// Системное
#define SYSCALL_GET_TIME 5
#define SYSCALL_GET_TIME_UP 7

// Память
#define SYSCALL_MALLOC 10
#define SYSCALL_REALLOC 11
#define SYSCALL_FREE 12
#define SYSCALL_KMALLOC_STATS 13

// Устройства ввода
#define SYSCALL_GETCHAR 30

// Управление питанием
#define SYSCALL_POWER_OFF 100
#define SYSCALL_REBOOT 101

// Управление задачами
#define SYSCALL_TASK_CREATE 200
#define SYSCALL_TASK_LIST 201
#define SYSCALL_TASK_STOP 202
#define SYSCALL_REAP_ZOMBIES 203
#define SYSCALL_TASK_EXIT 204
#define SYSCALL_TASK_IS_ALIVE 205
#define SYSCALL_TASK_GET_ERRNO_LOC 206
#define SYSCALL_GETPID 207

// Отладка/Исключения
#define THROW_AN_EXCEPTION 300

// Графика (примитивы)
#define SYSCALL_GFX_DRAW_POINT 400
#define SYSCALL_GFX_DRAW_LINE 401
#define SYSCALL_GFX_DRAW_CIRCLE 402
#define SYSCALL_GFX_FILL_CIRCLE 403
#define SYSCALL_GFX_DRAW_RECT 404
#define SYSCALL_GFX_FILL_RECT 405
#define SYSCALL_GFX_CLEAR 406

// НОВЫЕ графические сисколы
#define SYSCALL_GFX_UPDATE_SCREEN 407 // Обновить экран из бэк-буфера
#define SYSCALL_GFX_DRAW_STRING 408   // Нарисовать строку
#define SYSCALL_GFX_GET_TEXT_BOUNDS 409 // Получить размеры текста

// Файловая система
#define SYSCALL_CHDIR 500
#define SYSCALL_GETCWD 501
#define SYSCALL_GET_CWD_IDX 502
#define SYSCALL_FS_MKDIR 600
#define SYSCALL_FS_RMDIR 601
#define SYSCALL_FS_CREATE_FILE 602
#define SYSCALL_FS_REMOVE_ENTRY 603
#define SYSCALL_FS_FIND_IN_DIR 604
#define SYSCALL_FS_GET_ALL_IN_DIR 605
#define SYSCALL_FS_READ 606
#define SYSCALL_FS_WRITE 607
#define SYSCALL_FS_WRITE_FILE_IN_DIR 608
#define SYSCALL_FS_READ_FILE_IN_DIR 609
#define SYSCALL_FS_GET_PARENT_IDX 610
#define SYSCALL_FS_BUILD_PATH 611

#define SYSCALL_DISK_READ_SECTORS 700
#define SYSCALL_DISK_WRITE_SECTORS 701
#define SYSCALL_DISK_GET_SIZE 702

// IPC
#define SYSCALL_IPC_SEND    800
#define SYSCALL_IPC_RECEIVE 801
#define SYSCALL_IPC_CALL    802

typedef struct {
    int sender_pid;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
    uint64_t arg6;
} ipc_msg_t;

// IO ports
#define SYSCALL_IO_PORT_GRANT  810
#define SYSCALL_IO_PORT_REVOKE 811

// VFS
#define SYSCALL_VFS_REGISTER 820
#define SYSCALL_VFS_OPEN     821
#define SYSCALL_VFS_READ     822
#define SYSCALL_VFS_WRITE    823
#define SYSCALL_VFS_CLOSE    824
#define SYSCALL_VFS_STAT     825
#define SYSCALL_VFS_MKDIR    826
#define SYSCALL_VFS_RMDIR    827
#define SYSCALL_VFS_UNLINK   828

typedef struct {
    uint64_t size;
    uint32_t type;
    uint32_t mode;
    uint64_t ctime;
    uint64_t mtime;
} vfs_stat_t;

#define VFS_O_RDONLY    0x0000
#define VFS_O_WRONLY    0x0001
#define VFS_O_RDWR      0x0002
#define VFS_O_CREAT     0x0100
#define VFS_O_TRUNC     0x0200
#define VFS_O_APPEND    0x0400

static inline int syscall_io_port_grant(uint16_t port)
{
    register uint64_t rdi asm("rdi") = (uint64_t)port;
    register uint64_t rax asm("rax") = (uint64_t)SYSCALL_IO_PORT_GRANT;
    int result;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(rax), "D"(rdi)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int syscall_io_port_revoke(uint16_t port)
{
    register uint64_t rdi asm("rdi") = (uint64_t)port;
    register uint64_t rax asm("rax") = (uint64_t)SYSCALL_IO_PORT_REVOKE;
    int result;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(rax), "D"(rdi)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int* syscall_task_get_errno_loc(void) {
    int* result;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(SYSCALL_TASK_GET_ERRNO_LOC)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int syscall_fs_find(const char* path, void* entry_buf) {
    int res;
    // Предполагаем, что ядро ищет от корня, если `dir_path` и `dir_idx` NULL
    register uint64_t r10 asm("r10") = (uint64_t)entry_buf;
    asm volatile(
        "syscall"
        : "=a"(res)
        : "a"((uint64_t)SYSCALL_FS_FIND_IN_DIR), 
        "D"((uint64_t)path), 
        "S"((uint64_t)NULL), 
        "d"(-1), 
        "r"(r10)
        : "rcx", "r11", "memory"
    );
    return res;
}

static inline size_t syscall_fs_read(uint16_t cluster_idx, void* buf, size_t count) {
    size_t bytes_read;
    __asm__ volatile(
        "syscall"
        : "=a"(bytes_read)
        : "a"((uint64_t)SYSCALL_FS_READ), "D"((uint64_t)cluster_idx), "S"((uint64_t)buf), "d"((uint64_t)count)
        : "rcx", "r11", "memory"
    );
    return bytes_read;
}

static inline size_t syscall_fs_write(uint16_t cluster_idx, const void* buf, size_t count) {
    size_t bytes_written;
    __asm__ volatile(
        "syscall"
        : "=a"(bytes_written)
        : "a"((uint64_t)SYSCALL_FS_WRITE), "D"((uint64_t)cluster_idx), "S"((uint64_t)buf), "d"((uint64_t)count)
        : "rcx", "r11", "memory"
    );
    return bytes_written;
}

static inline void syscall_gfx_draw_string(int x, int y, int size, uint32_t color, const char* s) {
    register uint64_t r10 asm("r10") = (uint64_t)color;
    register uint64_t r8 asm("r8") = (uint64_t)s;
    __asm__ volatile(
        "syscall"
        :
        : "a"((uint64_t)SYSCALL_GFX_DRAW_STRING), "D"((uint64_t)x), "S"((uint64_t)y), "d"((uint64_t)size), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory"
    );
}

static inline void syscall_gfx_update_screen() {
    __asm__ volatile(
        "syscall"
        :
        : "a"((uint64_t)SYSCALL_GFX_UPDATE_SCREEN)
        : "rcx", "r11", "memory"
    );
}

static inline char *syscall_get_time(char *buf)
{
    char *result;
    __asm__ volatile(
        "movq %1, %%rax\n"
        "movq %2, %%rdi\n"
        "syscall\n"
        "movq %%rax, %0\n"
        : "=r"(result)
        : "i"((uint64_t)SYSCALL_GET_TIME), "r"((uint64_t)buf)
        : "rax", "rdi", "memory");
    return result;
}

static inline uint32_t syscall_get_time_up()
{
    uint32_t result;
    register uint64_t rax asm("rax") = (uint64_t)SYSCALL_GET_TIME_UP;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "0"(rax)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline void *syscall_malloc(size_t size)
{
    void *result;
    __asm__ volatile(
        "movq %1, %%rax\n"
        "movq %2, %%rdi\n"
        "syscall\n"
        "movq %%rax, %0\n"
        : "=r"(result)
        : "i"((uint64_t)SYSCALL_MALLOC), "r"((uint64_t)size)
        : "rax", "rdi", "memory");
    return result;
}

static inline void syscall_free(void *ptr)
{
    __asm__ volatile(
        "movq %0, %%rax\n"
        "movq %1, %%rdi\n"
        "syscall\n"
        :
        : "i"((uint64_t)SYSCALL_FREE), "r"((uint64_t)ptr)
        : "rax", "rdi", "memory");
}

static inline void *syscall_realloc(void *ptr, size_t size)
{
    void *result;
    __asm__ volatile(
        "movq %1, %%rax\n"
        "movq %2, %%rdi\n"
        "movq %3, %%rsi\n"
        "syscall\n"
        "movq %%rax, %0\n"
        : "=r"(result)
        : "i"((uint64_t)SYSCALL_REALLOC), "r"((uint64_t)ptr), "r"((uint64_t)size)
        : "rax", "rdi", "rsi", "memory");
    return result;
}

static inline void syscall_kmalloc_stats(void *stats)
{
    __asm__ volatile(
        "movq %0, %%rax\n"
        "movq %1, %%rdi\n"
        "syscall\n"
        :
        : "i"((uint64_t)SYSCALL_KMALLOC_STATS), "r"((uint64_t)stats)
        : "rax", "rdi", "memory");
}

static inline int syscall_getchar(void)
{
    int result;
    register uint64_t syscall_num asm("rax") = (uint64_t)SYSCALL_GETCHAR;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "0"(syscall_num)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline void syscall_power_off(void)
{
    __asm__ volatile(
        "movq %0, %%rax\n"
        "syscall\n"
        :
        : "i"((uint64_t)SYSCALL_POWER_OFF)
        : "rax", "memory");
}

static inline void syscall_reboot(void)
{
    __asm__ volatile(
        "movq %0, %%rax\n"
        "syscall\n"
        :
        : "i"((uint64_t)SYSCALL_REBOOT)
        : "rax", "memory");
}

static inline void syscall_task_create(void (*entry)(void), size_t stack_size)
{
    __asm__ volatile(
        "movq %0, %%rax\n"
        "movq %1, %%rdi\n"
        "movq %2, %%rsi\n"
        "syscall\n"
        :
        : "i"((uint64_t)SYSCALL_TASK_CREATE), "r"((uint64_t)entry), "r"((uint64_t)stack_size)
        : "rax", "rdi", "rsi", "memory");
}

static inline int syscall_task_list(void *buf, size_t max)
{
    int result;
    __asm__ volatile(
        "movq %1, %%rax\n"
        "movq %2, %%rdi\n"
        "movq %3, %%rsi\n"
        "syscall\n"
        "movq %%rax, %0\n"
        : "=r"(result)
        : "i"((uint64_t)SYSCALL_TASK_LIST), "r"((uint64_t)buf), "r"((uint64_t)max)
        : "rax", "rdi", "rsi", "memory");
    return result;
}

static inline int syscall_task_stop(int pid)
{
    int result;
    __asm__ volatile(
        "movq %1, %%rax\n"
        "movq %2, %%rdi\n"
        "syscall\n"
        "movq %%rax, %0\n"
        : "=r"(result)
        : "i"((uint64_t)SYSCALL_TASK_STOP), "r"((uint64_t)pid)
        : "rax", "rdi", "memory");
    return result;
}

static inline void syscall_reap_zombies(void)
{
    __asm__ volatile(
        "movq %0, %%rax\n"
        "syscall\n"
        :
        : "i"((uint64_t)SYSCALL_REAP_ZOMBIES)
        : "rax", "memory");
}

static inline void syscall_exit(int exit_code)
{
    __asm__ volatile(
        "movq %0, %%rax\n"
        "movq %1, %%rdi\n"
        "syscall\n"
        :
        : "i"((uint64_t)SYSCALL_TASK_EXIT), "r"((uint64_t)exit_code)
        : "rax", "rdi", "memory");
}

static inline int syscall_disk_read_sectors(uint64_t lba, uint32_t num_sectors, void* buffer) {
    int result;
    register uint64_t rdi asm("rdi") = lba;
    register uint64_t rsi asm("rsi") = num_sectors;
    register uint64_t rdx asm("rdx") = (uint64_t)buffer;
    register uint64_t rax asm("rax") = SYSCALL_DISK_READ_SECTORS;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(rax), "D"(rdi), "S"(rsi), "d"(rdx)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int syscall_disk_write_sectors(uint64_t lba, uint32_t num_sectors, const void* buffer) {
    int result;
    register uint64_t rdi asm("rdi") = lba;
    register uint64_t rsi asm("rsi") = num_sectors;
    register uint64_t rdx asm("rdx") = (uint64_t)buffer;
    register uint64_t rax asm("rax") = SYSCALL_DISK_WRITE_SECTORS;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(rax), "D"(rdi), "S"(rsi), "d"(rdx)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline off_t syscall_disk_get_size(void) {
    off_t result;
    register uint64_t rax asm("rax") = SYSCALL_DISK_GET_SIZE;
     __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(rax)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int syscall_getpid(void) {
    int result;
     __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(SYSCALL_GETPID)
        : "rcx", "r11", "memory"
    );
    return result;
}

// static inline int syscall_disk_read(uint64_t lba, uint32_t count, void* buffer) {
//     int result;
//     register uint64_t r10 asm("r10") = (uint64_t)buffer;
//     __asm__ volatile(
//         "syscall"
//         : "=a"(result)
//         : "a"(SYSCALL_DISK_READ), "D"(lba), "S"((uint64_t)count), "r"(r10)
//         : "rcx", "r11", "memory"
//     );
//     return result;
// }

// static inline int syscall_disk_write(uint64_t lba, uint32_t count, const void* buffer) {
//     int result;
//     register uint64_t r10 asm("r10") = (uint64_t)buffer;
//     __asm__ volatile(
//         "syscall"
//         : "=a"(result)
//         : "a"(SYSCALL_DISK_WRITE), "D"(lba), "S"((uint64_t)count), "r"(r10)
//         : "rcx", "r11", "memory"
//     );
//     return result;
// }

static inline int syscall_ipc_send(int target_pid, ipc_msg_t* msg) {
    int result;
    register uint64_t rdi asm("rdi") = (uint64_t)target_pid;
    register uint64_t rsi asm("rsi") = (uint64_t)msg;
    register uint64_t rax asm("rax") = SYSCALL_IPC_SEND;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(rax), "D"(rdi), "S"(rsi)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int syscall_ipc_receive(int filter_pid, ipc_msg_t* msg_buf) {
    int result;
    register uint64_t rdi asm("rdi") = (uint64_t)filter_pid;
    register uint64_t rsi asm("rsi") = (uint64_t)msg_buf;
    register uint64_t rax asm("rax") = SYSCALL_IPC_RECEIVE;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(rax), "D"(rdi), "S"(rsi)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int syscall_vfs_register(const char* mount_point) {
    int result;
    register uint64_t rdi asm("rdi") = (uint64_t)mount_point;
    register uint64_t rax asm("rax") = SYSCALL_VFS_REGISTER;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(rax), "D"(rdi)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int syscall_vfs_open(const char* path, int flags) {
    int result;
    register uint64_t rdi asm("rdi") = (uint64_t)path;
    register uint64_t rsi asm("rsi") = (uint64_t)flags;
    register uint64_t rax asm("rax") = SYSCALL_VFS_OPEN;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(rax), "D"(rdi), "S"(rsi)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline ssize_t syscall_vfs_read(int fd, void* buffer, size_t count) {
    ssize_t result;
    register uint64_t rdi asm("rdi") = (uint64_t)fd;
    register uint64_t rsi asm("rsi") = (uint64_t)buffer;
    register uint64_t rdx asm("rdx") = (uint64_t)count;
    register uint64_t rax asm("rax") = SYSCALL_VFS_READ;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(rax), "D"(rdi), "S"(rsi), "d"(rdx)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline ssize_t syscall_vfs_write(int fd, const void* buffer, size_t count) {
    ssize_t result;
    register uint64_t rdi asm("rdi") = (uint64_t)fd;
    register uint64_t rsi asm("rsi") = (uint64_t)buffer;
    register uint64_t rdx asm("rdx") = (uint64_t)count;
    register uint64_t rax asm("rax") = SYSCALL_VFS_WRITE;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(rax), "D"(rdi), "S"(rsi), "d"(rdx)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int syscall_vfs_close(int fd) {
    int result;
    register uint64_t rdi asm("rdi") = (uint64_t)fd;
    register uint64_t rax asm("rax") = SYSCALL_VFS_CLOSE;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(rax), "D"(rdi)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int syscall_vfs_stat(const char* path, vfs_stat_t* stat_buf) {
    int result;
    register uint64_t rdi asm("rdi") = (uint64_t)path;
    register uint64_t rsi asm("rsi") = (uint64_t)stat_buf;
    register uint64_t rax asm("rax") = SYSCALL_VFS_STAT;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(rax), "D"(rdi), "S"(rsi)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int syscall_vfs_mkdir(const char* path, uint32_t mode) {
    int result;
    register uint64_t rdi asm("rdi") = (uint64_t)path;
    register uint64_t rsi asm("rsi") = (uint64_t)mode;
    register uint64_t rax asm("rax") = SYSCALL_VFS_MKDIR;
    __asm__ volatile(
        "syscall"
        : "=a"(result)
        : "a"(rax), "D"(rdi), "S"(rsi)
        : "rcx", "r11", "memory"
    );
    return result;
}

#endif /* SYSCALL_H */