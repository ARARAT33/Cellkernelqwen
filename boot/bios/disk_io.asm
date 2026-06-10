/*
 * CellKernel - Disk I/O Routines (BIOS INT 13h)
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

.section .text
.global bios_disk_read
.type bios_disk_read, @function

/* 
 * bios_disk_read(drive, lba, buffer, count)
 *   rdi = drive number (0x80 for first HDD)
 *   rsi = LBA address
 *   rdx = destination buffer
 *   rcx = sector count
 */
bios_disk_read:
    push %rbp
    mov %rsp, %rbp
    push %rbx
    push %rsi
    push %rdi
    
    /* Save parameters */
    mov %rdi, %rbx        /* drive */
    mov %rsi, %r12        /* lba low */
    mov %rdx, %r13        /* buffer */
    mov %rcx, %r14        /* count */
    
.read_loop:
    /* Enable A20 if needed */
    call enable_a20_gate
    
    /* Set up disk address packet */
    sub $16, %rsp
    movb $0x10, (%rsp)           /* packet size */
    movb $0x00, 1(%rsp)          /* reserved */
    movw %r14w, 2(%rsp)          /* block count */
    movw %r12w, 4(%rsp)          /* LBA low */
    movw %r12w, 6(%rsp)          /* LBA mid */
    movl %r12d, 8(%rsp)          /* LBA high */
    lea 12(%rsp), %rax
    mov %rax, (%rsp)             /* buffer address */
    
    /* INT 13h AH=42h - Extended Read */
    mov $0x42, %ah
    mov %bx, %dl                 /* drive */
    lea 16(%rsp), %si            /* DAP */
    int $0x13
    
    jc .read_error
    
    add $16, %rsp
    
    /* Check if more sectors to read */
    dec %r14
    jz .done
    
    inc %r12
    jmp .read_loop
    
.read_error:
    add $16, %rsp
    mov $-1, %rax
    jmp .cleanup
    
.done:
    xor %rax, %rax
    
.cleanup:
    pop %rdi
    pop %rsi
    pop %rbx
    pop %rbp
    ret


/* Enable A20 gate using keyboard controller */
.section .text
.global enable_a20_gate
.type enable_a20_gate, @function

enable_a20_gate:
    push %rax
    push %rbx
    
.wait_kbd:
    in $0x64, %al
    test $0x02, %al
    jnz .wait_kbd
    
    out $0x60, %al              /* Write command */
    
    in $0x64, %al
    test $0x02, %al
    jnz .in $0x64, %al
    
    mov $0xD1, %al              /* Output port command */
    out $0x64, %al
    
    in $0x64, %al
    test $0x02, %al
    jnz .wait_output
    
    mov $0xDF, %al              /* Enable A20 */
    out $0x60, %al
    
.wait_output:
    in $0x64, %al
    test $0x02, %al
    jnz .wait_output
    
    pop %rbx
    pop %rax
    ret
