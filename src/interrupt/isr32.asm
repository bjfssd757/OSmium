[BITS 64]

global isr32
extern timer_tick
extern schedule_from_isr

isr32:
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    push qword 0        ; err_code
    push qword 32       ; int_no

    call timer_tick

    sub rsp, 8
    lea rdi, [rsp + 8]
    mov rsi, rsp
    call schedule_from_isr

    mov rsp, [rsp]

    add rsp, 16

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

    iretq

section .note.GNU-stack
; empty