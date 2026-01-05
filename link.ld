OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(start)

PHDRS
{
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata  PT_LOAD    FLAGS((1 << 2)) ;            /* Read only */
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
}

SECTIONS
{
    . = 0xFFFFFFFF80000000;

    .text : {
        *(.limine_requests_start)
        KEEP(*(.limine_requests))
        *(.limine_requests_end)
        *(.text*)
    } :text

    . = ALIGN(4096);
    .rodata : { *(.rodata*) } :rodata

    . = ALIGN(4096);
    .data : { *(.data*) } :data

    . = ALIGN(4096);
    .bss : {
        *(.bss*)
        *(COMMON)

        . = ALIGN(16);
        stack64_bottom = .;
        . += 64K;
        stack64_top = .;
    } :data

    _heap_start = ALIGN(4096);
}