/*
 * CellKernel - BIOS Boot Entry Point (x86_64)
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 * 
 * Multiboot compliant bootloader entry point
 */

.section .multiboot_header
.align 8

/* Multiboot 2 header magic */
.set MULTIBOOT2_MAGIC, 0xE85250D6
.set MULTIBOOT2_ARCH, 0        /* x86_64 protected mode */
.set MULTIBOOT2_HEADER_LEN, multiboot_header_end - multiboot_header_start
.set MULTIBOOT2_CHECKSUM, -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + MULTIBOOT2_HEADER_LEN)

multiboot_header_start:
    .long MULTIBOOT2_MAGIC
    .long MULTIBOOT2_ARCH
    .long MULTIBOOT2_HEADER_LEN
    .long MULTIBOOT2_CHECKSUM

    /* End tag */
    .short 0
    .short 0
    .long 0

multiboot_header_end:

.section .text
.global _start
.type _start, @function

_start:
    /* Disable interrupts */
    cli
    
    /* Set up stack (temporary, will be replaced by kernel) */
    mov $stack_top, %rsp
    
    /* Clear BSS section */
    xor %rax, %rax
    lea _bss_start(%rip), %rdi
    lea _bss_end(%rip), %rcx
    sub %rdi, %rcx
    shr $3, %rcx
    rep stosq
    
    /* Load kernel entry point address into rdi */
    lea kernel_main(%rip), %rdi
    
    /* Jump to kernel main */
    call kernel_main
    
    /* Should never reach here */
    hlt
    
.section .bss
.align 16
.space 0x4000            /* 16KB stack */
stack_top:
