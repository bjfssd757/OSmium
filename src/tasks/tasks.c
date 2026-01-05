#include "tasks.h"
#include "../multitask/multitask.h"
#include "../syscall/syscall.h"
#include "../fs/vfs.h"
#include "../libc/string.h"
#include "../graphics/graphics.h"
#include "../malloc/malloc.h"
#include <stdint.h>
#include "../graphics/formatting.h"
#include "../graphics/colors.h"
#include "../app_manager/elf.h"

extern bool screen_refresh_status;

void zombie_reaper_thread(void *_arg)
{
    (void)_arg;
    for (;;)
    {
        reap_zombie_threads();
        asm volatile("hlt");
    }
}

void screen_refresh_thread(void *_arg)
{
    (void)_arg;
    for (;;)
    {
        if (screen_refresh_status)
        {
            gfx_update_screen();
            screen_refresh_status = false;
        }
        asm volatile("hlt");
    }
}

static void trim_string(char *str)
{
    if (! str)
        return;

    char *start = str;
    while (*start && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r'))
        start++;

    char *end = start + strlen(start) - 1;
    while (end >= start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
        end--;

    if (start != str)
        memmove(str, start, end - start + 2);

    str[end - start + 1] = '\0';
}

static int load_program_from_file(const char* file_path)
{
    // TODO: boot from ELF

    return 0;
}

void load_and_run_from_autorun(void)
{
    const char* autorun_path = "SYS:/autorun.rc";

    vfs_stat_t stat;
    if (vfs_stat(autorun_path, &stat) != 0) {
        return;
    }

    if (stat.type != VFS_TYPE_FILE || stat.size == 0) {
        return;
    }

    char *buffer = (char *)malloc(stat.size + 1);
    if (!buffer) {
        return;
    }

    int fd = vfs_open(autorun_path, VFS_O_RDONLY);
    if (fd < 0) {
        free(buffer);
        return;
    }

    ssize_t bytes_read = vfs_read(fd, buffer, stat. size);
    vfs_close(fd);

    if (bytes_read != (ssize_t)stat.size) {
        free(buffer);
        return;
    }

    buffer[stat.size] = '\0';

    char *saveptr = NULL;
    char *token = strtok_r(buffer, ";", &saveptr);

    while (token) {
        trim_string(token);

        if (*token != '\0') {
            load_program_from_file(token);
        }

        token = strtok_r(NULL, ";", &saveptr);
    }

    free(buffer);
}

void delayed_app_loader_thread(void *_arg)
{
    (void)_arg;
    
    vfs_stat_t stat;
    int attempts = 0;
    const int max_attempts = 100;
    
    while (attempts < max_attempts) {
        if (vfs_stat("/", &stat) == 0) {
            break;
        }
        
        attempts++;
        for (int i = 0; i < 10000; i++) {
            asm volatile("nop");
        }
        asm volatile("hlt");
    }
    
    if (attempts >= max_attempts) {
        thread_exit(1);
        return;
    }

    load_and_run_from_autorun();

    thread_exit(0);
}

void tasks_init(void)
{
    thread_create(
        get_current_process(),
        zombie_reaper_thread,
        NULL, 
        false, 
        0
    );
    thread_create(
        get_current_process(),
        screen_refresh_thread,
        NULL, 
        false, 
        0
    );
    // thread_create(
    //     get_current_process(),
    //     delayed_app_loader,
    //     NULL, 
    //     false, 
    //     0
    // );
}