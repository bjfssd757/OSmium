global jump_to_user

jump_to_user:
    ; rdi = entry, rsi = rsp, rdx = pml4_phys

    mov cr3, rdx

    push 0x23       ; SS (User Data Selector | RPL 3)
    push rsi
    push 0x202      ; RFLAGS (Interrupts Enabled)
    push 0x10       ; CS (User Code Selector | RPL 3)
    push rdi        ; RIP

    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    iretq