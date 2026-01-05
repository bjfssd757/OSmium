#include "ipc.h"
#include "multitask.h"
#include "../libc/string.h"

static int enqueue_message(thread_t *target, ipc_msg_t *msg)
{
    if (target->mailbox_count >= MAILBOX_SIZE)
        return -1;

    target->mailbox[target->mailbox_tail] = *msg;
    target->mailbox_tail = (target->mailbox_tail + 1) % MAILBOX_SIZE;
    target->mailbox_count++;
    return 0;
}

static int dequeue_message(thread_t *thr, int filter_pid, ipc_msg_t *out_msg)
{
    if (thr->mailbox_count == 0)
        return -1;

    for (int i = 0; i < thr->mailbox_count;  i++)
    {
        int idx = (thr->mailbox_head + i) % MAILBOX_SIZE;
        if (filter_pid == 0 || thr->mailbox[idx].sender_pid == filter_pid)
        {
            *out_msg = thr->mailbox[idx];
            for (int j = i; j < thr->mailbox_count - 1; j++)
            {
                int from_idx = (thr->mailbox_head + j + 1) % MAILBOX_SIZE;
                int to_idx = (thr->mailbox_head + j) % MAILBOX_SIZE;
                thr->mailbox[to_idx] = thr->mailbox[from_idx];
            }
            thr->mailbox_count--;
            return 0;
        }
    }
    return -1;
}

int sys_ipc_send(int target_pid, int target_tid, ipc_msg_t *msg)
{
    thread_t *current = get_current_thread();
    thread_t* target = (target_tid != 0)
        ? find_thread(target_pid, target_tid)
        : get_first_alive_thread(target_pid);

    
    if (!target || target->state == THREAD_ZOMBIE)
    {
        return -1;
    }

    msg->sender_pid = current->tid;

    if (enqueue_message(target, msg) != 0)
    {
        return -1;
    }

    if (target->state == THREAD_BLOCKED && target->ipc_blocked_on_pid == 0)
    {
        target->state = THREAD_READY;
    }
    else if (target->state == THREAD_BLOCKED && target->ipc_blocked_on_pid == current->tid)
    {
        if (target->ipc_reply_msg)
        {
            memcpy(target->ipc_reply_msg, msg, sizeof(ipc_msg_t));
        }
        target->state = THREAD_READY;
        target->ipc_blocked_on_pid = 0;
        target->ipc_reply_msg = NULL;
    }

    return 0;
}

int sys_ipc_receive(int filter_pid, int filter_tid, ipc_msg_t *msg_buf)
{
    thread_t* current = get_current_thread();

    if (dequeue_message(current, filter_pid, msg_buf) == 0) {
        return 0;
    }

    current->state = THREAD_BLOCKED;
    current->ipc_blocked_on_pid = 0;
    
    return IPC_CODE_BLOCKED;
}

int sys_ipc_call(int target_pid, int target_tid, ipc_msg_t *msg, ipc_msg_t *reply_buf)
{
    int send_rc = sys_ipc_send(target_pid, target_tid, msg);
    if (send_rc != 0) {
        return send_rc;
    }

    thread_t* current = get_current_thread();
    current->state = THREAD_BLOCKED;
    current->ipc_blocked_on_pid = target_pid;
    current->ipc_reply_msg = reply_buf;
    
    return IPC_CODE_BLOCKED;
}