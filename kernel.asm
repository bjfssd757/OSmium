[BITS 64]

global start
extern kmain
extern stack64_top

section .text
start:
    cli
    mov rsp, stack64_top
    xor rbp, rbp
    push 0
    mov rbp, rsp
    call kmain

.hang:
    hlt
    jmp .hang
