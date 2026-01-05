#ifndef MULTIthread_H
#define MULTIthread_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../mm/vmm.h"
#include "../mm/pmm.h"

typedef struct thread thread_t;
typedef struct process process_t;
typedef struct window window_t;

typedef enum {
    THREAD_RUNNING,
    THREAD_READY,
    THREAD_BLOCKED,
    THREAD_ZOMBIE
} thread_state_t;

typedef enum {
    PROCESS_RUNNING,
    PROCESS_ZOMBIE,
    PROCESS_STOPPED
} process_state_t;

#define KSTACK_SIZE      (8 * 1024)
#define USTACK_SIZE      (64 * 1024)
#define MAX_PROCESSES    1024
#define MAX_THREADS_PER_PROCESS 128
#define CWD_PATH_MAX     256

#include "ipc.h"
#define MAILBOX_SIZE 16

struct thread {
    int tid;
    thread_state_t state;

    uint64_t *regs;
    void *kstack;
    size_t kstack_size;

    void *user_stack;
    size_t user_stack_size;

    void *arg;

    thread_t *next;
    thread_t *proc_next;
    thread_t *znext;

    process_t *parent;

    window_t *windows;

    int exit_code;
    int errno;

    char cwd_path[CWD_PATH_MAX];

    ipc_msg_t mailbox[MAILBOX_SIZE];
    int mailbox_head;
    int mailbox_tail;
    int mailbox_count;

    int ipc_blocked_on_pid;
    ipc_msg_t *ipc_reply_msg;
};

struct process {
    int pid;
    process_state_t state;
    page_table_t *pml4;
    char cwd_path[CWD_PATH_MAX];
    int thread_count;
    thread_t *threads;
    thread_t *main_thread;
    process_t *next;
    process_t *znext;
    int exit_code;
    // TODO: file descriptors, environment, etc.
};

typedef struct thread_info {
    int tid;
    int state;
} thread_info_t;

typedef struct process_info {
    int pid;
    int state;
    int threads;
} process_info_t;

#define PROC_FLAG_FORCE              (1 << 0)
#define PROC_FLAG_INHERIT_ENV        (1 << 1)
#define PROC_FLAG_INHERIT_FD         (1 << 2)
#define PROC_FLAG_DETACH             (1 << 3)
#define PROC_FLAG_DO_NOT_ISOLATION   (1 << 4)
#define PROC_FLAG_MAX_PRIORITY       (1 << 5)

#define PROC_CODE_CRASH              -1
#define PROC_CODE_OK                 0
#define PROC_CODE_OOM                -20
#define PROC_CODE_CRITICAL_ERROR     -500
#define PROC_CODE_KERNEL_KILL        -600

void scheduler_init(void);
void schedule_from_isr(uint64_t *regs, uint64_t **out_regs_ptr);
void reap_zombie_threads(void);

process_t *process_create(uint64_t flags);
void process_exit(int exit_code);
int process_stop(int pid);
process_t *get_current_process(void);
process_t *find_process_by_pid(int pid);
int process_is_alive(int pid);
process_t *get_all_processes(void);

thread_t *thread_create(
    process_t *parent,
    void (*entry)(void *), void *arg,
    bool is_user,
    uint64_t flags
);
void thread_exit(int exit_code);
int thread_stop(int tid);
thread_t *get_current_thread(void);
thread_t *find_thread(int pid, int tid);
thread_t *get_all_threads(int pid);
int thread_is_alive(int tid);
thread_t *get_first_alive_thread(int pid);

int sys_chdir(const char *path);
int sys_getcwd(char *buf, size_t size);

void jump_to_user(uint64_t entry, uint64_t rsp, uint64_t pml4_phys);

static inline void switch_to_user(uint64_t entry, uint64_t pml4_phys)
{
    uint64_t user_stack_top = 0x00007FFFFFFFF000;
    uint64_t stack_phys = (uint64_t)alloc_pages(4);

    mmap((page_table_t*)virt(pml4_phys), user_stack_top - PAGE_SIZE, stack_phys,
        PTE_PRESENT | PTE_WRITABLE | PTE_USER
    );

    jump_to_user(entry, user_stack_top, pml4_phys);
}

#endif