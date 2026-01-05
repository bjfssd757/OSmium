#include "multitask.h"
#include "../malloc/malloc.h"
#include "../libc/string.h"
#include "../syscall/syscall.h"
#include "../fs/vfs.h"
#include <stdint.h>
#include <stddef.h>
#include "../tss/tss.h"
#include "../mm/vmm.h"

static inline uint64_t read_pml4(void);

#define USER_CS ((uint64_t)0x18 | 3) /* 0x1B */
#define USER_SS ((uint64_t)0x20 | 3) /* 0x23 */

extern char _heap_start;
extern char _heap_end;

uint64_t g_syscall_kstack_top = 0;

#define INIT_KSTACK_SIZE (16 * 1024)

static uint8_t init_kstack[INIT_KSTACK_SIZE];
static thread_t *thread_ring = NULL;
static thread_t *current_thread = NULL;
static thread_t *zombie_threads = NULL;

static process_t *process_table[MAX_PROCESSES] = { 0 };
static process_t *current_process = NULL;
static int next_pid = 1;
static int next_tid = 1;

/* CLI/STI */
static inline void cli(void) { __asm__ volatile("cli" : :: "memory"); }
static inline void sti(void) { __asm__ volatile("sti" ::: "memory"); }

static int alloc_pid(void) { return next_pid++; }
static int alloc_tid(void) { return next_tid++; }

/* prepare_initial_stack:  остается без изменений */
static uint64_t *prepare_initial_stack(void (*entry)(void*),
                                       void *kstack_top,
                                       void *user_stack_top,
                                       int argc,
                                       uintptr_t argv_ptr,
                                       int user_mode)
{
    const int FRAME_QWORDS = 22;
    uint64_t *sp = (uint64_t *)kstack_top;
    sp = (uint64_t *)(((uintptr_t)sp) & ~0xFULL);
    sp -= FRAME_QWORDS;

    sp[0] = 32;
    sp[1] = 0;
    sp[2] = 0;   /* r15 */
    sp[3] = 0;   /* r14 */
    sp[4] = 0;   /* r13 */
    sp[5] = 0;   /* r12 */
    sp[6] = 0;   /* r11 */
    sp[7] = 0;   /* r10 */
    sp[8] = 0;   /* r9  */
    sp[9] = 0;   /* r8  */
    sp[10] = (uint64_t)argc;     /* rdi */
    sp[11] = (uint64_t)argv_ptr; /* rsi */
    sp[12] = 0;  /* rbp */
    sp[13] = 0;  /* rbx */
    sp[14] = 0;  /* rdx */
    sp[15] = 0;  /* rcx */
    sp[16] = 0;  /* rax */
    sp[17] = (uint64_t)entry;    /* rip */
    sp[19] = 0x202;              /* rflags (IF=1) */

    if (user_mode)
    {
        sp[18] = USER_CS;
        sp[20] = (uint64_t)user_stack_top;
        sp[21] = USER_SS;
    }
    else
    {
        sp[18] = 0x08;
        sp[20] = (uint64_t)kstack_top;
        sp[21] = 0x10;
    }

    return sp;
}

void scheduler_init(void) 
{
    process_t *kernel_proc = malloc(sizeof(process_t));
    memset(kernel_proc, 0, sizeof(*kernel_proc));
    kernel_proc->pid = 0;
    kernel_proc->state = PROCESS_RUNNING;
    kernel_proc->pml4 = (page_table_t*)read_pml4();
    strcpy(kernel_proc->cwd_path, "/");
    process_table[0] = kernel_proc;
    current_process = kernel_proc;

    thread_t *thr = malloc(sizeof(thread_t));
    memset(thr, 0, sizeof(*thr));
    thr->tid = alloc_tid();
    thr->parent = kernel_proc;
    thr->state = THREAD_RUNNING;
    thr->kstack = init_kstack;
    thr->kstack_size = sizeof(init_kstack);
    thr->next = thr;
    strcpy(thr->cwd_path, "/");

    kernel_proc->threads = thr;
    kernel_proc->thread_count = 1;
    kernel_proc->main_thread = thr;

    thread_ring = thr;
    current_thread = thr;

    tss_init();
}

void add_to_thread_ring(thread_t *thr) {
    if (!thread_ring) {
        thread_ring = thr->next = thr;
    } else {
        thr->next = thread_ring->next;
        thread_ring->next = thr;
        thread_ring = thr;
    }
}

process_t *process_create(uint64_t flags)
{
    cli();
    int pid = alloc_pid();
    process_t *p = malloc(sizeof(process_t));
    memset(p, 0, sizeof(process_t));

    p->pid = pid;
    p->state = PROCESS_RUNNING;
    p->pml4 = create_address_space();
    strcpy(p->cwd_path, "SYS:/");

    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (process_table[i] == NULL)
        {
            process_table[i] = p;
            break;
        }
    }
    sti();
    return p;
}

thread_t *thread_create(process_t *parent, void(*entry)(void*), void *arg, bool is_user, uint64_t flags) {
    thread_t *thr = malloc(sizeof(thread_t));
    memset(thr, 0, sizeof(*thr));
    thr->tid = alloc_tid();
    thr->state = THREAD_READY;
    thr->parent = parent;
    thr->kstack_size = KSTACK_SIZE;
    thr->kstack = malloc(KSTACK_SIZE);
    thr->arg = arg;
    thr->errno = 0;
    strcpy(thr->cwd_path, parent->cwd_path);

    void *kstack_top = (char*)thr->kstack + thr->kstack_size;
    thr->regs = prepare_initial_stack(entry, kstack_top, NULL, (uint64_t)arg, 0, is_user);

    thr->proc_next = parent->threads;
    parent->threads = thr;
    parent->thread_count++;

    cli();
    add_to_thread_ring(thr);
    sti();

    return thr;
}

static thread_t *pick_next_thread(void)
{
    if (!thread_ring) return NULL;
    thread_t *start = current_thread ? current_thread->next : thread_ring->next;
    thread_t *it = start;
    do {
        if (it->state == THREAD_READY || it->state == THREAD_RUNNING) {
            if (it->parent->pml4 != (page_table_t *)read_pml4())
                vmm_switch_pml4(it->parent->pml4);
            return it;
        }
        it = it->next;
    } while (it != start);
    return NULL;
}

static process_t *pick_next_process(void)
{
    if (!current_process) return NULL;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        int idx = (current_process->pid + i + 1) % MAX_PROCESSES;
        if (process_table[idx] && process_table[idx]->state == PROCESS_RUNNING) {
            return process_table[idx];
        }
    }
    return NULL;
}

void schedule_from_isr(uint64_t *regs, uint64_t **out_regs_ptr)
{
    if (current_thread) {
        current_thread->regs = regs;
        if (current_thread->state == THREAD_RUNNING)
            current_thread->state = THREAD_READY;
    }

    thread_t *next = pick_next_thread();
    if (!next) {
        if (current_thread) current_thread->state = THREAD_RUNNING;
        *out_regs_ptr = regs;
        return;
    }

    current_thread = next;
    current_thread->state = THREAD_RUNNING;
    *out_regs_ptr = current_thread->regs;
    g_syscall_kstack_top = (uint64_t)current_thread->kstack + current_thread->kstack_size;
    tss_update_rsp0(g_syscall_kstack_top);
}

thread_t *get_current_thread(void) { return current_thread; }
process_t *get_current_process(void) { return current_process; }

static void add_to_zombie_threads(thread_t *thr) {
    thr->znext = zombie_threads;
    zombie_threads = thr;
}

static void remove_from_thread_ring(thread_t *thr) {
    if (!thread_ring) return;
    thread_t *prev = thread_ring, *it = thread_ring->next;
    do {
        if (it == thr) {
            prev->next = it->next;
            if (thread_ring == it) thread_ring = prev;
            break;
        }
        prev = it;
        it = it->next;
    } while (it != thread_ring->next);
}

static void free_thread_resources(thread_t *t)
{
    if (!t)
        return;

    if (t->kstack)
        free(t->kstack);

    if (t->user_stack)
    {
        free(t->user_stack);
        t->user_stack = NULL;
        t->user_stack_size = 0;
    }

    free(t);
}

void reap_zombie_processes(void)
{
    cli();
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        process_t *p = process_table[i];
        if (p && p->state == PROCESS_ZOMBIE)
        {
            thread_t *thr = p->threads;
            while (thr)
            {
                thread_t *next = thr->proc_next;
                free_thread_resources(thr);
                thr = next; 
            }

            if (p->pml4)
            {
                
            }
        }
    }
}

void reap_zombie_threads(void) {
    cli();
    thread_t *z = zombie_threads;
    zombie_threads = NULL;
    sti();
    while (z)
    {
        thread_t *next = z->znext;
        remove_from_thread_ring(z);
        free_thread_resources(z);
        z = next;
    }
}

int process_stop(int pid)
{
    if (pid == 0)
        return -1;

    process_t *p = find_process_by_pid(pid);
    if (!p) return -1;

    thread_t *it = p->threads;
    do
    {
        thread_stop(it->tid);
        it = it->next;
    } while (it);

    return 0;
}

int thread_stop(int tid)
{
    if (tid == 0)
        return -1;

    reap_zombie_threads();

    cli();
    if (! thread_ring)
    {
        sti();
        return -1;
    }

    thread_t *it = thread_ring->next;
    thread_t *found = NULL;
    do
    {
        if (it->tid == tid)
        {
            found = it;
            break;
        }
        it = it->next;
    } while (it != thread_ring->next);

    if (! found)
    {
        sti();
        return -1;
    }

    if (found == current_thread)
    {
        current_thread->state = THREAD_ZOMBIE;
        add_to_zombie_threads(current_thread);
        remove_from_thread_ring(current_thread);
        sti();
        for (;;)
        {
            sti();
            __asm__ volatile("hlt");
        }
    }

    remove_from_thread_ring(found);
    sti();
    free_thread_resources(found);
    return 0;
}

void thread_exit(int exit_code) {
    cli();
    current_thread->exit_code = exit_code;
    current_thread->state = THREAD_ZOMBIE;
    add_to_zombie_threads(current_thread);
    remove_from_thread_ring(current_thread);
    sti();
    for (;;)
        __asm__ volatile("hlt");
}

void process_exit(int exit_code)
{
    cli();
    current_process->exit_code = exit_code;
    current_process->state = PROCESS_ZOMBIE;
    
    thread_t *thr = current_process->threads;
    while (thr)
    {
        thr->state = THREAD_ZOMBIE;
        remove_from_thread_ring(thr);
        add_to_zombie_threads(thr);
        thr = thr->proc_next;
    }

    if (current_process->pml4)
    {
        free(current_process->pml4);
        current_process->pml4 = NULL;
    }

    current_process = NULL;
    sti();

    while (1)
        asm("hlt");
}

int thread_is_alive(int tid)
{
    if (tid < 0)
        return 0;

    if (tid == 0)
        return 1;

    reap_zombie_threads();

    cli();
    if (!thread_ring)
    {
        sti();
        return 0;
    }

    thread_t *it = thread_ring->next;
    do
    {
        if (it->tid == tid)
        {
            int alive = (it->state != THREAD_ZOMBIE);
            sti();
            return alive;
        }
        it = it->next;
    } while (it != thread_ring->next);

    sti();
    return 0;
}

int sys_chdir(const char *path)
{
    if (!path || !current_thread || path[0] == '\0')
        return -1;

    bool is_correct_style = (path[0] != '\0' && path[1] == ':');

    if (is_correct_style || path[0] == '/') {
        if (strlen(path) >= CWD_PATH_MAX)
            return -1;

        strcpy(current_thread->parent->cwd_path, path);
        strcpy(current_thread->cwd_path, path);
        return 0;
    }

    // TODO: relative paths
    
    return 0;
}

int sys_getcwd(char *buf, size_t size)
{
    if (!buf || !current_thread)
        return -1;
    
    if (strlen(current_thread->cwd_path) >= size)
        return -1;
    
    strcpy(buf, current_thread->cwd_path);
    return (int)strlen(buf);
}

static inline uint64_t read_pml4(void) {
    uint64_t val;
    __asm__ volatile("mov %%cr3, %0" : "=r"(val));
    return val + hhdm_offset;
}

process_t *find_process_by_pid(int pid) {
    for (int i = 0; i < MAX_PROCESSES; ++i)
        if (process_table[i] && process_table[i]->pid == pid)
            return process_table[i];
    return NULL;
}

thread_t *find_thread(int pid, int tid) {
    process_t *p = find_process_by_pid(pid);
    if (!p) return NULL;
    for (thread_t *t = p->threads; t; t = t->proc_next)
        if (t->tid == tid) return t;
    return NULL;
}

thread_t *get_all_threads(int pid) {
    process_t *proc = find_process_by_pid(pid);
    if (!proc)
        return NULL;
    return proc->threads;
}

process_t *get_all_processes(void)
{
    return process_table[0]->next;
}

int process_is_alive(int pid)
{
    if (pid < 0 || pid >= MAX_PROCESSES || !process_table[pid]) {
        return 0;
    }

    return process_table[pid]->state == PROCESS_RUNNING;
}

thread_t *get_first_alive_thread(int pid)
{
    process_t *proc = find_process_by_pid(pid);
    if (!proc || proc->state != PROCESS_RUNNING) {
        return NULL;
    }

    thread_t *thr = proc->threads;
    while (thr) {
        if (thr->state != THREAD_ZOMBIE) {
            return thr;
        }
        thr = thr->proc_next;
    }

    return NULL;
}