; isr80.asm
[bits 64]

extern syscall_handler
global isr80

isr80:
    sub     rsp, 8

    push    r15
    push    r14
    push    r13
    push    r12
    push    r11
    push    rbp
    push    rbx
    push    rcx

    push    r9
    push    r8
    push    r10
    push    rdx
    push    rsi
    push    rdi
    push    rax
    
    mov     rdi, rsp
    call    syscall_handler
    
    mov     [rsp], rax

    pop     rax
    pop     rdi
    pop     rsi
    pop     rdx
    pop     r10
    pop     r8
    pop     r9
    pop     rcx
    pop     rbx
    pop     rbp
    pop     r11
    pop     r12
    pop     r13
    pop     r14
    pop     r15
    add     rsp, 8
    
    iretq

section .note.GNU-stack