[BITS 64]

%macro ISR_STUB_ERR 1
global isr_stub_%1
isr_stub_%1:
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

    mov rsi, %1
    mov rsi, [rsp + 120]
    extern page_fault_handler
    call page_fault_handler

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

    add rsp, 0
    iretq
%endmacro

ISR_STUB_ERR 14

section .note.GNU-stack
; empty