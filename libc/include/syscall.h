#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>
#include "sys/types.h"

// State
#define SYSCALL_GET_TIME            5
#define SYSCALL_GET_TIME_UP         7

// Memory management
#define SYSCALL_MALLOC              10
#define SYSCALL_REALLOC             11
#define SYSCALL_FREE                12
#define SYSCALL_KMALLOC_STATS       13

// IO devices
#define SYSCALL_GETCHAR             30

// Power management
#define SYSCALL_POWER_OFF           100
#define SYSCALL_REBOOT              101

// Process management (process creation is for system/privileged code; userland can only create threads)
#define SYSCALL_PROCESS_CREATE      200
#define SYSCALL_PROCESS_LIST        201
#define SYSCALL_PROCESS_STOP        202
#define SYSCALL_PROCESS_EXIT        203
#define SYSCALL_PROCESS_IS_ALIVE    204
#define SYSCALL_GETPID              205

// Thread management
#define SYSCALL_THREAD_CREATE       250
#define SYSCALL_THREAD_LIST         251
#define SYSCALL_THREAD_STOP         252
#define SYSCALL_REAP_ZOMBIES        253
#define SYSCALL_THREAD_EXIT         254
#define SYSCALL_THREAD_IS_ALIVE     255
#define SYSCALL_THREAD_GET_ERRNO_LOC 256
#define SYSCALL_GETTID              257

// Exception/debug
#define THROW_AN_EXCEPTION          300

// Graphics (framebuffer primitives)
#define SYSCALL_GFX_DRAW_POINT      400
#define SYSCALL_GFX_DRAW_LINE       401
#define SYSCALL_GFX_DRAW_CIRCLE     402
#define SYSCALL_GFX_FILL_CIRCLE     403
#define SYSCALL_GFX_DRAW_RECT       404
#define SYSCALL_GFX_FILL_RECT       405
#define SYSCALL_GFX_CLEAR           406
#define SYSCALL_GFX_UPDATE_SCREEN   407
#define SYSCALL_GFX_DRAW_STRING     408
#define SYSCALL_GFX_GET_TEXT_BOUNDS 409

// Event system (read event queue from kernel)
#define SYSCALL_GET_EVENTS          900

// File system
#define SYSCALL_CHDIR               500
#define SYSCALL_GETCWD              501
#define SYSCALL_GET_CWD_IDX         502
#define SYSCALL_FS_MKDIR            600
#define SYSCALL_FS_RMDIR            601
#define SYSCALL_FS_CREATE_FILE      602
#define SYSCALL_FS_REMOVE_ENTRY     603
#define SYSCALL_FS_FIND_IN_DIR      604
#define SYSCALL_FS_GET_ALL_IN_DIR   605
#define SYSCALL_FS_READ             606
#define SYSCALL_FS_WRITE            607
#define SYSCALL_FS_WRITE_FILE_IN_DIR 608
#define SYSCALL_FS_READ_FILE_IN_DIR 609
#define SYSCALL_FS_GET_PARENT_IDX   610
#define SYSCALL_FS_BUILD_PATH       611

#define SYSCALL_DISK_READ_SECTORS   700
#define SYSCALL_DISK_WRITE_SECTORS  701
#define SYSCALL_DISK_GET_SIZE       702

// IPC
#define SYSCALL_IPC_SEND            800
#define SYSCALL_IPC_RECEIVE         801
#define SYSCALL_IPC_CALL            802

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
#define SYSCALL_IO_PORT_GRANT       810
#define SYSCALL_IO_PORT_REVOKE      811

// VFS
#define SYSCALL_VFS_REGISTER        820
#define SYSCALL_VFS_OPEN            821
#define SYSCALL_VFS_READ            822
#define SYSCALL_VFS_WRITE           823
#define SYSCALL_VFS_CLOSE           824
#define SYSCALL_VFS_STAT            825
#define SYSCALL_VFS_MKDIR           826
#define SYSCALL_VFS_RMDIR           827
#define SYSCALL_VFS_UNLINK          828
#define SYSCALL_VFS_FIND            829
#define SYSCALL_VFS_SEEK            830
#define SYSCALL_VFS_FILE_SIZE       831

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

typedef enum {
    EVENT_MOUSE,
    EVENT_KEY,
    EVENT_WINDOW
} event_type_t;

typedef struct {
    event_type_t type;
    union {
        struct { int x, y, button, state; } mouse;
        struct { int key, pressed; } key;
    } data;
    int target_wid;
    int pid;
    int uid;
} event_t;

#define _SYSCALL_RET(type, res) \
    if (!__builtin_types_compatible_p(type, void)) return (type)res;

#define _DO_SYSCALL(name, type, ...) do {                                 \
    register uint64_t rax_ __asm__("rax") = SYSCALL_##name;               \
    uint64_t res_;                                                         \
    __asm__ volatile("syscall"                                            \
                     : "=a"(res_)                                          \
                     : "a"(rax_) , ##__VA_ARGS__                           \
                     : "rcx", "r11", "memory");                            \
    _SYSCALL_RET(type, res_);                                              \
} while (0)

#define _SC0_A(type, name, alias) \
    static inline type name(void) { _DO_SYSCALL(name, type); } \
    static inline type PP_CAT(syscall_, alias)(void) { return name(); }

#define _SC1_A(type, name, alias, t1, a1) \
    static inline type name(t1 a1) { \
        register uint64_t rdi_ __asm__("rdi") = (uint64_t)a1; \
        _DO_SYSCALL(name, type, "D"(rdi_)); \
    } \
    static inline type PP_CAT(syscall_, alias)(t1 a1) { return name(a1); }

#define _SC2_A(type, name, alias, t1, a1, t2, a2) \
    static inline type name(t1 a1, t2 a2) { \
        register uint64_t rdi_ __asm__("rdi") = (uint64_t)a1; \
        register uint64_t rsi_ __asm__("rsi") = (uint64_t)a2; \
        _DO_SYSCALL(name, type, "D"(rdi_), "S"(rsi_)); \
    } \
    static inline type PP_CAT(syscall_, alias)(t1 a1, t2 a2) { return name(a1, a2); }

#define _SC3_A(type, name, alias, t1, a1, t2, a2, t3, a3) \
    static inline type name(t1 a1, t2 a2, t3 a3) { \
        register uint64_t rdi_ __asm__("rdi") = (uint64_t)a1; \
        register uint64_t rsi_ __asm__("rsi") = (uint64_t)a2; \
        register uint64_t rdx_ __asm__("rdx") = (uint64_t)a3; \
        _DO_SYSCALL(name, type, "D"(rdi_), "S"(rsi_), "d"(rdx_)); \
    } \
    static inline type PP_CAT(syscall_, alias)(t1 a1, t2 a2, t3 a3) { return name(a1, a2, a3); }

#define _SC4_A(type, name, alias, t1, a1, t2, a2, t3, a3, t4, a4) \
    static inline type name(t1 a1, t2 a2, t3 a3, t4 a4) { \
        register uint64_t rdi_ __asm__("rdi") = (uint64_t)a1; \
        register uint64_t rsi_ __asm__("rsi") = (uint64_t)a2; \
        register uint64_t rdx_ __asm__("rdx") = (uint64_t)a3; \
        register uint64_t r10_ __asm__("r10") = (uint64_t)a4; \
        _DO_SYSCALL(name, type, "D"(rdi_), "S"(rsi_), "d"(rdx_), "r"(r10_)); \
    } \
    static inline type PP_CAT(syscall_, alias)(t1 a1, t2 a2, t3 a3, t4 a4) { return name(a1, a2, a3, a4); }

#define _SC5_A(type, name, alias, t1, a1, t2, a2, t3, a3, t4, a4, t5, a5) \
    static inline type name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5) { \
        register uint64_t rdi_ __asm__("rdi") = (uint64_t)a1; \
        register uint64_t rsi_ __asm__("rsi") = (uint64_t)a2; \
        register uint64_t rdx_ __asm__("rdx") = (uint64_t)a3; \
        register uint64_t r10_ __asm__("r10") = (uint64_t)a4; \
        register uint64_t r8_  __asm__("r8")  = (uint64_t)a5; \
        _DO_SYSCALL(name, type, "D"(rdi_), "S"(rsi_), "d"(rdx_), "r"(r10_), "r"(r8_)); \
    } \
    static inline type PP_CAT(syscall_, alias)(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5) { return name(a1, a2, a3, a4, a5); }

#define _SC6_A(type, name, alias, t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, t6, a6) \
    static inline type name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6) { \
        register uint64_t rdi_ __asm__("rdi") = (uint64_t)a1; \
        register uint64_t rsi_ __asm__("rsi") = (uint64_t)a2; \
        register uint64_t rdx_ __asm__("rdx") = (uint64_t)a3; \
        register uint64_t r10_ __asm__("r10") = (uint64_t)a4; \
        register uint64_t r8_  __asm__("r8")  = (uint64_t)a5; \
        register uint64_t r9_  __asm__("r9")  = (uint64_t)a6; \
        _DO_SYSCALL(name, type, "D"(rdi_), "S"(rsi_), "d"(rdx_), "r"(r10_), "r"(r8_), "r"(r9_)); \
    } \
    static inline type PP_CAT(syscall_, alias)(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6) { return name(a1, a2, a3, a4, a5, a6); }

#define PP_CAT(a,b) PP_CAT_I(a,b)
#define PP_CAT_I(a,b) a##b

#define PP_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,N,...) N
#define PP_RSEQ_N() 16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_NARG(...) PP_NARG_(__VA_ARGS__, PP_RSEQ_N())

#define SELECT_SC_BY_COUNT_3  _SC0_A
#define SELECT_SC_BY_COUNT_5  _SC1_A
#define SELECT_SC_BY_COUNT_7  _SC2_A
#define SELECT_SC_BY_COUNT_9  _SC3_A
#define SELECT_SC_BY_COUNT_11 _SC4_A
#define SELECT_SC_BY_COUNT_13 _SC5_A
#define SELECT_SC_BY_COUNT_15 _SC6_A

#define SELECT_SC_BY_COUNT(N) PP_CAT(SELECT_SC_BY_COUNT_, N)

#define syscall(...) SELECT_SC_BY_COUNT(PP_NARG(__VA_ARGS__))(__VA_ARGS__)


typedef void (*thread_entry_t)(void*);

syscall(uint32_t, GET_TIME_UP, get_time_up)

syscall(void*, MALLOC, malloc, size_t, size)
syscall(void, FREE, free, void*, ptr)
syscall(void*, REALLOC, realloc, void*, ptr, size_t, size)
syscall(void, KMALLOC_STATS, kmalloc_stats, void*, stats)

syscall(int, GETCHAR, getchar)
syscall(void, POWER_OFF, power_off)
syscall(void, REBOOT, reboot)

syscall(int, THREAD_CREATE, thread_create, thread_entry_t, entry, size_t, stack_size)
syscall(int, THREAD_LIST, thread_list, void*, buf, size_t, max)
syscall(int, THREAD_STOP, thread_stop, int, tid)
syscall(void, REAP_ZOMBIES, reap_zombies)
syscall(void, THREAD_EXIT, thread_exit, int, exit_code)
syscall(int, THREAD_IS_ALIVE, thread_is_alive, int, tid)
syscall(int, THREAD_GET_ERRNO_LOC, thread_get_errno_loc)
syscall(int, GETTID, gettid)

syscall(void, PROCESS_EXIT, process_exit, int, exit_code)

syscall(void, GFX_DRAW_POINT, gfx_draw_point, uint32_t, x, uint32_t, y, uint32_t, color)
syscall(void, GFX_DRAW_LINE, gfx_draw_line, int32_t, x0, int32_t, y0, int32_t, x1, int32_t, y1, uint32_t, color)
syscall(void, GFX_DRAW_CIRCLE, gfx_draw_circle, int32_t, xc, int32_t, yc, int32_t, radius, uint32_t, color)
syscall(void, GFX_FILL_CIRCLE, gfx_fill_circle, int32_t, xc, int32_t, yc, int32_t, radius, uint32_t, color)
syscall(void, GFX_DRAW_RECT, gfx_draw_rect, int32_t, x0, int32_t, y0, int32_t, x1, int32_t, y1, uint32_t, color)
syscall(void, GFX_FILL_RECT, gfx_fill_rest, int32_t, x0, int32_t, y0, int32_t, x1, int32_t, y1, uint32_t, color)
syscall(void, GFX_CLEAR, gfx_clear, uint32_t, color)
syscall(void, GFX_UPDATE_SCREEN, gfx_update_screen)
syscall(void, GFX_DRAW_STRING, gfx_draw_string, int, x, int, y, int, size, uint32_t, color, const char*, s)
syscall(void, GFX_GET_TEXT_BOUNDS, gfx_get_text_bounds, int, size, const char*, s, int*, w, int*, h, int*, left)

syscall(ssize_t, GET_EVENTS, get_events, event_t*, buf, size_t, max_count)

syscall(int, IPC_SEND, ipc_send, int, target_pid, ipc_msg_t*, msg)
syscall(int, IPC_RECEIVE, ipc_receive, int, filter_pid, ipc_msg_t*, msg_buf)
syscall(int, IPC_CALL, ipc_call, int, target_pid, int, target_tid, ipc_msg_t*, msg, ipc_msg_t*, reply_buf)

syscall(int, VFS_OPEN, vfs_open, const char*, path, int, flags)
syscall(ssize_t, VFS_READ, vfs_read, int, fd, void*, buffer, size_t, count)
syscall(ssize_t, VFS_WRITE, vfs_write, int, fd, const void*, buffer, size_t, count)
syscall(int, VFS_CLOSE, vfs_close, int, fd)
syscall(int, VFS_STAT, vfs_stat, const char*, path, vfs_stat_t*, stat_buf)
syscall(int, VFS_MKDIR, vfs_mkdir, const char*, path, uint32_t, mode)
syscall(int, VFS_RMDIR, vfs_rmdir, const char*, path)
syscall(int, VFS_UNLINK, vfs_unlink, const char*, path)
syscall(int, VFS_FIND, vfs_find, const char*, path)
syscall(int, VFS_SEEK, vfs_seek, int, fd, long, offset, int, whence)
syscall(size_t, VFS_FILE_SIZE, vfs_file_size, const char*, path)

syscall(int, CHDIR, chdir, const char*, path)
syscall(int, GETCWD, getcwd, char*, buf, size_t, size)

syscall(int, DISK_READ_SECTORS, disk_read_sectors, uint64_t, lba, uint32_t, num_sectors, void*, buffer)
syscall(int, DISK_WRITE_SECTORS, disk_write_sectors, uint64_t, lba, uint32_t, num_sectors, const void*, buffer)
syscall(off_t, DISK_GET_SIZE, disk_get_size)

syscall(int, IO_PORT_GRANT, io_port_grant, uint16_t, port)
syscall(int, IO_PORT_REVOKE, io_port_revoke, uint16_t, port)

#endif /* SYSCALL_H */