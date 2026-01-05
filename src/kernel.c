/* kernel.c */
#include <stdint.h>
#include "fs/fs.h"
#include "keyboard/keyboard.h"
#include "portio/portio.h"
#include "idt.h"
#include "time/timer.h"
#include "time/clock/clock.h"
#include "syscall/syscall.h"
#include "malloc/malloc.h"
#include "libc/string.h"
#include "power/poweroff.h"
#include "power/reboot.h"
#include "multitask/multitask.h"
#include "tasks/tasks.h"
#include "ramdisk/ramdisk.h"
#include "user/terminal/main_bin.h"
#include "user/ls/main_bin.h"
#include "user/memstat/main_bin.h"
#include "user/mkdir/main_bin.h"
#include "user/rm/main_bin.h"
#include "user/pwd/main_bin.h"
#include "user/clear/main_bin.h"
#include "user/shutdown/main_bin.h"
#include "user/reboot/main_bin.h"
#include "user/help/main_bin.h"
#include "user/time/main_bin.h"
#include "default_files.h"
#include "graphics/formatting.h"
#include "graphics/graphics.h"
#include "graphics/font.h"
#include "drivers/ide.h"
#include "drivers/pci.h"
#include "fs/fs.h"
#include "fs/vfs.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "graphics/colors.h"
#include "limine.h"

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};
struct limine_framebuffer *fb;

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};
struct limine_memmap_response *memmap_res;

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};
struct limine_hhdm_response *hhdm_res;
uint64_t hhdm_offset;

__attribute__((used, section(".limine_requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};
struct limine_rsdp_response *rsdp_res;

__attribute__((used, section(".limine_requests")))
static volatile struct limine_bootloader_info_request bl_request = {
    .id = LIMINE_BOOTLOADER_INFO_REQUEST,
    .revision = 0
};
struct limine_bootloader_info_response *bl_res;

extern char _heap_start;

uint64_t g_saved_user_rsp = 0;

void load_app_to_fs(char *folder, char *name, char *ext, unsigned char *data, unsigned int data_len)
{
    if (!folder || !name || !ext || !data) {
        return;
    }

    char folder_path[256];
    if (folder[0] != '/') {
        strcpy(folder_path, "/");
        strcat(folder_path, folder);
    } else {
        strcpy(folder_path, folder);
    }

    vfs_stat_t stat_buf;
    if (vfs_stat(folder_path, &stat_buf) != 0) {
        if (vfs_mkdir(folder_path, 0755) != 0) {
            return;
        }
    } else {
        if (stat_buf.type != VFS_TYPE_DIR) {
            return;
        }
    }

    char file_path[256];
    strcpy(file_path, folder_path);
    if (file_path[strlen(file_path) - 1] != '/') {
        strcat(file_path, "/");
    }
    strcat(file_path, name);
    if (ext && strlen(ext) > 0) {
        strcat(file_path, ".");
        strcat(file_path, ext);
    }

    int fd = vfs_open(file_path, VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC);
    if (fd < 0) {
        return;
    }

    ssize_t written = vfs_write(fd, data, data_len);
    if (written != (ssize_t)data_len) {
    }

    vfs_close(fd);
}

int init_autorun(const char *autorun)
{
    if (!autorun) {
        return -1;
    }

    const char* boot_dir_path = "SYS:/boot.d";
    const char* autorun_file_path = "SYS:/boot.d/autorun.rc";

    vfs_stat_t stat_buf;
    if (vfs_stat(boot_dir_path, &stat_buf) != 0) {
        if (vfs_mkdir(boot_dir_path, 0755) != 0) {
            return -2;
        }
    } else {
        if (stat_buf.type != VFS_TYPE_DIR) {
            return -3;
        }
    }

    if (vfs_stat(autorun_file_path, &stat_buf) == 0) {
        return 0;
    }

    int fd = vfs_open(autorun_file_path, VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC);
    if (fd < 0) {
        return -4;
    }

    size_t autorun_len = strlen(autorun);
    ssize_t written = vfs_write(fd, autorun, autorun_len);
    
    vfs_close(fd);
    
    if (written != (ssize_t)autorun_len) {
        return -5;
    }

    return 0;
}

static void enable_sse(void)
{
    uint64_t cr0, cr4;

    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1UL << 2);  // Clear EM
    cr0 |= (1UL << 1);   // Set MP
    __asm__ volatile("mov %0, %%cr0" ::  "r"(cr0));

    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1UL << 9);   // Set OSFXSR
    cr4 |= (1UL << 10);  // Set OSXMMEXCPT
    __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
}

void kmain(void)
{
    if (LIMINE_BASE_REVISION_SUPPORTED == false)
    {
        for (;;) __asm__("hlt");
    }

    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1)
    {
        for (;;) __asm__("hlt");
    }

    if (rsdp_request.response == NULL || rsdp_request.response->address == NULL)
    {
        return;
    }

    if (bl_request.response == NULL || bl_request.response->name == NULL)
    {
        for (;;) asm ("hlt");
    }

    bl_res = bl_request.response;
    fb = framebuffer_request.response->framebuffers[0];
    memmap_res = memmap_request.response;
    hhdm_res = hhdm_request.response;
    hhdm_offset = hhdm_res->offset;
    rsdp_res = rsdp_request.response;

    idt_install();
    enable_sse();
    init_system_clock();
    init_timer(1000);
    outb(0x21, 0xFC);

    pmm_init();
    malloc_init();

    gfx_init(fb);
    pci_init();

    gfx_clear(0x000000);

    // gfx_draw_window(50, 50, 400, 200, "System Information", GFX_STATE_NORMAL);
    // gfx_draw_window(200, 250, 500, 300, "Application", GFX_STATE_NORMAL);

    gfx_update_screen();

#ifdef DEBUG

    uint64_t rsdp = get_rsdp_address();
    if (rsdp != 0)
    {
        kprint(0, "RSDP found at physical address: 0x%016lx\n", rsdp);
    }
    else
    {
        kprint(0, "RSDP not found in Multiboot2 tags\n");
    }

    int n = pci_get_device_count();
    kprint(0, "PCI devices: %d\n", n);
    for (int i = 0; i < n; ++i)
    {
        pci_device_t *d = pci_get_device(i);
        kprint(0, "dev %d: %02x:%02x.%x ven=0x%04x dev=0x%04x class=0x%02x sub=0x%02x\n",
               i, d->bus, d->device, d->function, d->vendor_id, d->device_id, d->class_code, d->subclass);
    }

    /* --- Инициализация IDE и проверка диска --- */
    ide_disk_t disk;
    int rc = ide_init(&disk, IDE_CHANNEL_PRIMARY, 0); // primary channel, master (0)

    if (rc == 0)
    {
        uint16_t ident[256];
        if (ide_identify(&disk, ident) == 0)
        {
            /* Считываем модель из слов 54..73 (20 слов = 40 байт) */
            char model[41];
            for (int i = 0; i < 40; i += 2)
            {
                uint16_t w = ident[54 + i / 2];
                model[i] = (char)((w >> 8) & 0xFF);
                model[i + 1] = (char)(w & 0xFF);
            }
            model[40] = '\0';
            /* Обрезаем пробелы справа */
            int mlen = strlen(model);
            while (mlen > 0 && model[mlen - 1] == ' ')
            {
                model[mlen - 1] = '\0';
                --mlen;
            }

            /* Выводим найденную модель и общее число секторов (форматированные вызовы) */
            kprint(0, "IDE: Found ATA Drive Primary Master: \"%s\"\n", model);

            char numbuf[32];
            /* Преобразуем число в строку */
            {
                uint64_t val = disk.total_sectors;
                int pos = 0;
                if (val == 0)
                {
                    numbuf[pos++] = '0';
                }
                else
                {
                    char rev[32];
                    int rpos = 0;
                    while (val && rpos < (int)sizeof(rev) - 1)
                    {
                        rev[rpos++] = '0' + (val % 10);
                        val /= 10;
                    }
                    while (rpos > 0)
                        numbuf[pos++] = rev[--rpos];
                }
                numbuf[pos] = '\0';
            }

            kprint(0, "IDE: Total sectors: %s\n", numbuf);
        }
        else
        {
            kprint(0, "IDE: IDENTIFY failed (device present but identify read failed)\n");
        }
    }
    else
    {
        kprint(0, "IDE: No device on Primary Master (ide_init failed)\n");
    }

#endif // DEBUG

    gfx_clear(COLOR_CYAN);
    gfx_draw_string(30, 30, 16, COLOR_GOLD, bl_res->name);
    gfx_update_screen();

    scheduler_init();
    tasks_init();

    fs_init();

    // init_autorun(autorun);

    // load_app_to_fs("bin", "terminal", "bin", terminal_bin, terminal_bin_len);
    // load_app_to_fs("bin", "memstat", "bin", memstat_bin, memstat_bin_len);
    // load_app_to_fs("bin", "clear", "bin", clear_bin, clear_bin_len);
    // load_app_to_fs("bin", "shutdown", "bin", shutdown_bin, shutdown_bin_len);
    // load_app_to_fs("bin", "reboot", "bin", reboot_bin, reboot_bin_len);
    // load_app_to_fs("bin", "help", "bin", help_bin, help_bin_len);
    // load_app_to_fs("bin", "time", "bin", time_bin, time_bin_len);
    // load_app_to_fs("bin", "ls", "bin", ls_bin, ls_bin_len);
    // load_app_to_fs("bin", "pwd", "bin", pwd_bin, pwd_bin_len);
    // load_app_to_fs("bin", "mkdir", "bin", mkdir_bin, mkdir_bin_len);
    // load_app_to_fs("bin", "rm", "bin", rm_bin, rm_bin_len);

    /* Разрешаем прерывания */
    asm volatile("sti");

    for (;;)
    {
        asm volatile("hlt");
    }
}