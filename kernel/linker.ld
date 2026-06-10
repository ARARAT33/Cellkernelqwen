/* CellKernel Linker Script for x86_64
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

ENTRY(kernel_main)

SECTIONS
{
    /* Load at 1MB (typical kernel load address) */
    . = 0x100000;
    
    _kernel_start = .;
    
    /* Boot section from EFI/loader */
    .boot : ALIGN(4K)
    {
        *(.boot)
        *(.boot.*)
    }
    
    /* Text section (code) */
    _text_start = .;
    .text : ALIGN(4K)
    {
        *(.text)
        *(.text.*)
        *(.init)
        *(.fini)
    }
    _text_end = .;
    
    /* Read-only data */
    .rodata : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata.*)
    }
    
    /* Exception tables */
    .exception_table : ALIGN(4K)
    {
        *(.exception_table)
        *(.exception_table.*)
    }
    
    /* Data section (initialized data) */
    _data_start = .;
    .data : ALIGN(4K)
    {
        *(.data)
        *(.data.*)
    }
    _data_end = .;
    
    /* BSS section (zero-initialized data) */
    _bss_start = .;
    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }
    _bss_end = .;
    
    /* Kernel stack */
    .stack : ALIGN(16)
    {
        _stack_bottom = .;
        . += 0x10000; /* 64KB kernel stack */
        _stack_top = .;
    }
    
    /* Page tables */
    .page_tables : ALIGN(4K)
    {
        _page_tables_start = .;
        . += 0x100000; /* 1MB for page tables */
        _page_tables_end = .;
    }
    
    /* Heap area */
    .heap : ALIGN(4K)
    {
        _heap_start = .;
        . += 0x10000000; /* 256MB initial heap */
        _heap_end = .;
    }
    
    _kernel_end = .;
    
    /* Discard unwanted sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
        *(.eh_frame_hdr)
    }
}

/* Provide symbols for kernel sections */
PROVIDE(_kernel_size = _kernel_end - _kernel_start);
PROVIDE(_text_size = _text_end - _text_start);
PROVIDE(_data_size = _data_end - _data_start);
PROVIDE(_bss_size = _bss_end - _bss_start);
PROVIDE(_stack_size = _stack_top - _stack_bottom);
PROVIDE(_heap_size = _heap_end - _heap_start);
