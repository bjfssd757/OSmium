#include "syscall.h"
#include "../graphics/graphics.h"
#include "../keyboard/keyboard.h"
#include "../time/clock/clock.h"
#include "../malloc/malloc.h"
#include "../power/poweroff.h"
#include "../power/reboot.h"
#include "../multitask/multitask.h"
#include "../fs/fs.h"
#include <stdint.h>
#include <stddef.h>
#include "../drivers/ide.h"
#include "../tss/tss.h"
#include "../fs/fs.h"
#include "../multitask/ipc.h"
#include "../fs/vfs.h"
#include "../multitask/eventbuf.h"

extern uint32_t seconds;

uintptr_t syscall_handler(const struct syscall_regs *regs)
{
    switch ((uint32_t)regs->rax)
    {        
        // --- Time ---
        case SYSCALL_GET_TIME:
            if (regs->rdi && regs->rsi >= sizeof(ClockTime))
            {
                ClockTime* time_buf = (ClockTime*)(uintptr_t)regs->rdi;
                time_buf->hh = system_clock.hh;
                time_buf->mm = system_clock.mm;
                time_buf->ss = system_clock.ss;
            }
            return 0;

        case SYSCALL_GET_TIME_UP:
            return (uintptr_t)seconds;

        // --- Memory ---
        case SYSCALL_MALLOC:
            return (uintptr_t)malloc((size_t)regs->rdi);

        case SYSCALL_FREE:
            free((void *)(uintptr_t)regs->rdi);
            return 0;

        case SYSCALL_REALLOC:
            return (uintptr_t)realloc((void *)(uintptr_t)regs->rdi, (size_t)regs->rsi);

        case SYSCALL_KMALLOC_STATS:
             if (regs->rdi)
                get_kmalloc_stats((void *)(uintptr_t)regs->rdi);
            return 0;

        // --- IO ---
        case SYSCALL_GETCHAR:
        {
            int c = kbd_getchar();
            return (uintptr_t)(c == -1 ? 0 : c);
        }

        // --- Power Management ---
        case SYSCALL_POWER_OFF:
            power_off();
            return 0;

        case SYSCALL_REBOOT:
            reboot_system();
            return 0;
        
        // --- Thread Management ---
        case SYSCALL_THREAD_CREATE:
            return (uintptr_t)thread_create(
                (void*)(uintptr_t)regs->rdi,
                (void(*)(void*))(uintptr_t)regs->rsi,
                (void*)(uintptr_t)regs->rdx,
                (bool)regs->r10,
                (uint64_t)regs->r8
            );

        case SYSCALL_THREAD_LIST:
           return (uintptr_t)get_all_threads((int)regs->rdi);
        
        case SYSCALL_THREAD_STOP:
            return (uintptr_t)thread_stop((int)regs->rdi);
        
        case SYSCALL_REAP_ZOMBIES:
            reap_zombie_threads();
            return 0;
        
        case SYSCALL_THREAD_EXIT:
            thread_exit((int)regs->rdi);
            return 0;
        
        case SYSCALL_THREAD_IS_ALIVE:
            return (uintptr_t)thread_is_alive((int)regs->rdi);

        case SYSCALL_THREAD_GET_ERRNO_LOC:
        {
            thread_t *current = get_current_thread();
            if (current)
            {
                return (uintptr_t)&current->errno;
            }
            return 0;
        }

        case SYSCALL_GETTID:
        {
            thread_t *current = get_current_thread();
            if (current)
            {
                return (uintptr_t)current->tid;
            }
            return (uintptr_t)-1;
        }

        // --- Process Management ---
        case SYSCALL_PROCESS_CREATE:
            return (uintptr_t)process_create((uint64_t)regs->rdi);

        case SYSCALL_PROCESS_EXIT:
            process_exit((int)regs->rdi);
            return 0;

        case SYSCALL_PROCESS_IS_ALIVE:
            return (uintptr_t)process_is_alive((int)regs->rdi);

        case SYSCALL_PROCESS_STOP:
            return (uintptr_t)process_stop((int)regs->rdi);

        case SYSCALL_PROCESS_LIST:
            return (uintptr_t)get_all_processes();

        // --- Graphics ---
        case SYSCALL_GFX_DRAW_POINT:
            gfx_draw_point((uint32_t)regs->rdi, (uint32_t)regs->rsi, (uint32_t)regs->rdx);
            return 0;

        case SYSCALL_GFX_DRAW_LINE:
            gfx_draw_line((int32_t)regs->rdi, (int32_t)regs->rsi, (int32_t)regs->rdx,
                          (int32_t)regs->r10, (uint32_t)regs->r8);
            return 0;

        case SYSCALL_GFX_DRAW_CIRCLE:
            gfx_draw_circle((int32_t)regs->rdi, (int32_t)regs->rsi,
                            (int32_t)regs->rdx, (uint32_t)regs->r10);
            return 0;

        case SYSCALL_GFX_FILL_CIRCLE:
            gfx_fill_circle((int32_t)regs->rdi, (int32_t)regs->rsi,
                            (int32_t)regs->rdx, (uint32_t)regs->r10);
            return 0;

        case SYSCALL_GFX_DRAW_RECT:
            gfx_draw_rect((int32_t)regs->rdi, (int32_t)regs->rsi, (int32_t)regs->rdx,
                          (int32_t)regs->r10, (uint32_t)regs->r8);
            return 0;

        case SYSCALL_GFX_FILL_RECT:
            gfx_fill_rect((int32_t)regs->rdi, (int32_t)regs->rsi, (int32_t)regs->rdx,
                          (int32_t)regs->r10, (uint32_t)regs->r8);
            return 0;

        case SYSCALL_GFX_CLEAR:
            gfx_clear((uint32_t)regs->rdi);
            return 0;

        case SYSCALL_GFX_UPDATE_SCREEN:
            gfx_update_screen();
            return 0;

        case SYSCALL_GFX_DRAW_STRING:
            // rdi: x, rsi: y, rdx: size, r10: color, r8: const char* s
            gfx_draw_string((int)regs->rdi, (int)regs->rsi, (int)regs->rdx,
                            (uint32_t)regs->r10, (const char *)(uintptr_t)regs->r8);
            return 0;

        case SYSCALL_GFX_GET_TEXT_BOUNDS:
            // rdi: size, rsi: const char* s, rdx: int* w, r10: int* h
            gfx_get_text_bounds((int)regs->rdi, (const char *)(uintptr_t)regs->rsi,
                                (int *)(uintptr_t)regs->rdx, (int *)(uintptr_t)regs->r10);
            return 0;

        // --- File System ---
        case SYSCALL_DISK_READ_SECTORS:
            return (uintptr_t)ide_read_sectors(
                get_primary_master_disk(),
                (uint64_t)regs->rdi,
                (uint32_t)regs->rsi,
                (void *)(uintptr_t)regs->rdx
            );
        
        case SYSCALL_DISK_WRITE_SECTORS:
            return (uintptr_t)ide_write_sectors(
                get_primary_master_disk(),
                (uint64_t)regs->rdi,
                (uint32_t)regs->rsi,
                (const void *)(uintptr_t)regs->rdx
            );

        case SYSCALL_CHDIR:
            return (uintptr_t)sys_chdir((const char *)(uintptr_t)regs->rdi);

        case SYSCALL_GETCWD:
            return (uintptr_t)sys_getcwd((char *)(uintptr_t)regs->rdi, (size_t)regs->rsi);

        case SYSCALL_DISK_GET_SIZE:
            return get_primary_master_disk()->total_sectors * get_primary_master_disk()->sector_size;
        
        // IO

        case SYSCALL_IO_PORT_GRANT:
            return (uintptr_t)sys_io_port_grant(regs->rdi);
        
        case SYSCALL_IO_PORT_REVOKE:
            return (uintptr_t)sys_port_revoke(regs->rdi);
        
        // VFS

        case SYSCALL_VFS_OPEN: 
            return (uintptr_t)vfs_open((const char*)(uintptr_t)regs->rdi, (int)regs->rsi);

        case SYSCALL_VFS_READ: 
            return (uintptr_t)vfs_read((int)regs->rdi, (void*)(uintptr_t)regs->rsi, (size_t)regs->rdx);

        case SYSCALL_VFS_WRITE:
            return (uintptr_t)vfs_write((int)regs->rdi, (const void*)(uintptr_t)regs->rsi, (size_t)regs->rdx);

        case SYSCALL_VFS_CLOSE:
            return (uintptr_t)vfs_close((int)regs->rdi);

        case SYSCALL_VFS_STAT: 
            return (uintptr_t)vfs_stat((const char*)(uintptr_t)regs->rdi, (vfs_stat_t*)(uintptr_t)regs->rsi);

        case SYSCALL_VFS_MKDIR:
            return (uintptr_t)vfs_mkdir((const char*)(uintptr_t)regs->rdi, (uint32_t)regs->rsi);

        case SYSCALL_VFS_RMDIR:
            return (uintptr_t)vfs_rmdir((const char*)(uintptr_t)regs->rdi);

        case SYSCALL_VFS_UNLINK:
            return (uintptr_t)vfs_unlink((const char*)(uintptr_t)regs->rdi);

        case SYSCALL_VFS_FIND:
            return (uintptr_t)vfs_find((const char*)(uintptr_t)regs->rdi);

        case SYSCALL_VFS_SEEK:
            return (uintptr_t)vfs_seek((int)regs->rdi, (uint64_t)regs->rsi, (uint32_t)regs->rdx);

        case SYSCALL_VFS_FILE_SIZE:
            return (uintptr_t)vfs_get_file_size((const char*)(uintptr_t)regs->rdi);

        // IPC

        case SYSCALL_IPC_SEND:
            return (uintptr_t)sys_ipc_send((int)regs->rdi, (int)regs->rsi, (ipc_msg_t *)(uintptr_t)regs->rdx);

        case SYSCALL_IPC_RECEIVE:
            return (uintptr_t)sys_ipc_receive((int)regs->rdi, (int)regs->rsi, (ipc_msg_t *)(uintptr_t)regs->rdx);

        case SYSCALL_IPC_CALL:
            return (uintptr_t)sys_ipc_call((int)regs->rdi, (int)regs->rsi, (ipc_msg_t *)(uintptr_t)regs->rdx, (ipc_msg_t *)(uintptr_t)regs->r10);

        // Events

        case SYSCALL_GET_EVENTS:
            return (uintptr_t)sys_get_events((event_t*)(uintptr_t)regs->rdi, (size_t)regs->rsi);

        default:
            return (uintptr_t)-1;
    }
}