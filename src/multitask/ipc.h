#ifndef IPC_H
#define IPC_H

#include <stdint.h>

#define IPC_CODE_BLOCKED -100

typedef struct
{
    int sender_pid;
    int sender_tid;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
    uint64_t arg6;
    uint64_t arg7;
    uint64_t arg8;
} ipc_msg_t;

int sys_ipc_call(int target_pid, int target_tid, ipc_msg_t *msg, ipc_msg_t *reply_buf);
int sys_ipc_receive(int filter_pid, int filter_tid, ipc_msg_t *msg_buf);
int sys_ipc_send(int target_pid, int target_tid, ipc_msg_t *msg);

#endif // IPC_H