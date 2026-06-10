/*
 * CellKernel - A20 Gate Control
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

.section .text
.global check_a20
.type check_a20, @function

/* Check if A20 is enabled */
check_a20:
    push %rbx
    push %rsi
    push %rdi
    
    /* Save current value at 0x0000:FFFF */
    mov $0xFFFF, %esi
    mov %es:(%esi), %ebx
    
    /* Read value at 0x1000:FFEF (should be same if A20 enabled) */
    mov $0x1000, %eax
    mov %eax, %es
    mov $0xFFEF, %edi
    mov %es:(%edi), %eax
    
    cmp %ebx, %eax
    je .a20_disabled
    
    /* A20 is enabled */
    xor %eax, %eax
    jmp .done
    
.a20_disabled:
    mov $1, %eax
    
.done:
    pop %rdi
    pop %rsi
    pop %rbx
    ret


.section .text
.global enable_a20_fast
.type enable_a20_fast, @function

/* Fast A20 enable using port 0x92 */
enable_a20_fast:
    push %rax
    
    in $0x92, %al
    test $0x02, %al
    jnz .a20_already_enabled
    
    /* Enable A20 */
    or $0x02, %al
    out $0x92, %al
    
    /* Small delay */
    jmp .delay1
.delay1:
    jmp .delay2
.delay2:
    
.a20_already_enabled:
    pop %rax
    ret


.section .text
.global disable_a20_fast
.type disable_a20_fast, @function

/* Disable A20 (for real mode compatibility) */
disable_a20_fast:
    push %rax
    
    in $0x92, %al
    and $0xFD, %al
    out $0x92, %al
    
    pop %rax
    ret
