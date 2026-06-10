/*
 * CellKernel - Multiboot Header
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 * 
 * Multiboot 2.0 specification compliant header
 */

.section .multiboot_header, "aw"
.align 8

/* Multiboot 2 magic number */
.set MULTIBOOT2_MAGIC, 0xE85250D6
.set MULTIBOOT2_ARCH, 0           /* Protected mode (i386) */
.set MULTIBOOT2_HEADER_LEN, multiboot_header_end - multiboot_header_start
.set MULTIBOOT2_CHECKSUM, -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + MULTIBOOT2_HEADER_LEN)

multiboot_header_start:
    /* Tag header */
    .long MULTIBOOT2_MAGIC
    .long MULTIBOOT2_ARCH
    .long MULTIBOOT2_HEADER_LEN
    .long MULTIBOOT2_CHECKSUM
    
    /* Information request tag */
    .short 1                      /* Type: information request */
    .short 0                      /* Flags */
    .long 8                       /* Size */
    
    /* Request memory map */
    .long 6                       /* Memory map tag type */
    
    /* Request framebuffer */
    .long 5                       /* Framebuffer tag type */
    
    /* End tag */
    .short 0                      /* Type: end */
    .short 0                      /* Flags */
    .long 8                       /* Size */

multiboot_header_end:


.section .text
.global multiboot_entry
.type multiboot_entry, @function

multiboot_entry:
    /* Disable interrupts */
    cli
    
    /* Check magic number */
    cmp $MULTIBOOT2_MAGIC, %eax
    jne .bad_magic
    
    /* Get multiboot info address from EBX */
    mov %ebx, %rdi               /* Pass multiboot info to kernel */
    
    /* Set up stack */
    mov $stack_top, %rsp
    
    /* Clear direction flag */
    cld
    
    /* Clear BSS */
    xor %rax, %rax
    lea _bss_start(%rip), %rdi
    lea _bss_end(%rip), %rcx
    sub %rdi, %rcx
    shr $3, %rcx
    rep stosq
    
    /* Call kernel main */
    call kernel_main
    
    /* Should never reach here */
.bad_magic:
    hlt
    jmp .bad_magic

.section .bss
.align 16
.space 0x8000                    /* 32KB stack */
stack_top:
