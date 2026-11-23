OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(0x05);
    data PT_LOAD FLAGS(0x06);
}

SECTIONS
{
    . = 0xffffffff80000000 + SIZEOF_HEADERS;

    .text : ALIGN(0x1000) {
        *(.text .text.*)
        *(.rodata .rodata.*)
    } :text

    .data : ALIGN(0x1000) {
        KEEP(*(.limine_requests_start_marker))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end_marker))
        *(.data .data.*)
    } :data

    .bss : ALIGN(0x1000) {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
        *(.comment)
    }
}
