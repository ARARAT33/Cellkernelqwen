/*
 * CellKernel - Kernel Panic Handler
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

#include "../include/kernel.h"

#define PANIC_MAGIC         0xDEADBEEF
#define CRASH_DUMP_VERSION  1

/* Crash dump structure */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint64_t timestamp;
    uint64_t panic_cpu;
    uint64_t cr0, cr2, cr3, cr4;
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip, cs, rflags;
    uint64_t error_code;
    char message[256];
} crash_dump_t;

static crash_dump_t g_crash_dump;
static volatile int g_in_panic = 0;

/* Get CPU ID */
static inline uint64_t get_cpu_id(void) {
    uint64_t id;
    __asm__ volatile ("mov %%gs:(0), %0" : "=r"(id));
    return id;
}

/* Read control registers */
static inline uint64_t read_cr0(void) {
    uint64_t val;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(val));
    return val;
}

static inline uint64_t read_cr2(void) {
    uint64_t val;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(val));
    return val;
}

static inline uint64_t read_cr3(void) {
    uint64_t val;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(val));
    return val;
}

static inline uint64_t read_cr4(void) {
    uint64_t val;
    __asm__ volatile ("mov %%cr4, %0" : "=r"(val));
    return val;
}

/* Print hex dump */
static void print_hexdump(uint64_t addr, size_t len) {
    uint8_t *ptr = (uint8_t *)addr;
    size_t i, j;
    
    for (i = 0; i < len; i += 16) {
        serial_print_hex(addr + i);
        serial_print(": ");
        
        for (j = 0; j < 16 && (i + j) < len; j++) {
            if ((ptr[i + j] & 0xF0) == 0) serial_putc('0');
            serial_print_hex(ptr[i + j]);
            serial_putc(' ');
        }
        
        serial_print("  ");
        
        for (j = 0; j < 16 && (i + j) < len; j++) {
            char c = ptr[i + j];
            if (c >= 32 && c < 127) {
                serial_putc(c);
            } else {
                serial_putc('.');
            }
        }
        
        serial_putc('\r');
        serial_putc('\n');
    }
}

/* Save crash dump */
static void save_crash_dump(const char *msg, uint64_t error_code) {
    memset(&g_crash_dump, 0, sizeof(g_crash_dump));
    
    g_crash_dump.magic = PANIC_MAGIC;
    g_crash_dump.version = CRASH_DUMP_VERSION;
    g_crash_dump.timestamp = 0;  /* Would use real time */
    g_crash_dump.panic_cpu = get_cpu_id();
    
    /* Save control registers */
    g_crash_dump.cr0 = read_cr0();
    g_crash_dump.cr2 = read_cr2();
    g_crash_dump.cr3 = read_cr3();
    g_crash_dump.cr4 = read_cr4();
    
    /* Save general purpose registers (would need assembly to capture) */
    /* For now, placeholder values */
    g_crash_dump.rax = 0;
    g_crash_dump.rbx = 0;
    g_crash_dump.rcx = 0;
    g_crash_dump.rdx = 0;
    g_crash_dump.rsi = 0;
    g_crash_dump.rdi = 0;
    g_crash_dump.rbp = 0;
    g_crash_dump.rsp = cpu_read_rsp();
    g_crash_dump.r8 = 0;
    g_crash_dump.r9 = 0;
    g_crash_dump.r10 = 0;
    g_crash_dump.r11 = 0;
    g_crash_dump.r12 = 0;
    g_crash_dump.r13 = 0;
    g_crash_dump.r14 = 0;
    g_crash_dump.r15 = 0;
    
    /* Save instruction pointer and flags */
    g_crash_dump.rip = cpu_read_rip();
    g_crash_dump.cs = 0;
    g_crash_dump.rflags = cpu_read_rflags();
    g_crash_dump.error_code = error_code;
    
    /* Save panic message */
    if (msg) {
        strncpy(g_crash_dump.message, msg, sizeof(g_crash_dump.message) - 1);
    }
}

/* Kernel panic handler */
void kernel_panic(const char *msg) {
    /* Disable interrupts */
    cpu_cli();
    
    /* Prevent recursive panics */
    if (g_in_panic) {
        serial_println("[!] Recursive panic detected!");
        while (1) {
            cpu_halt();
        }
    }
    
    g_in_panic = 1;
    
    serial_println("");
    serial_println("========================================");
    serial_println("!!! CELLKERNEL PANIC !!!");
    serial_println("========================================");
    serial_println("");
    
    /* Print message */
    if (msg) {
        serial_print("Reason: ");
        serial_println(msg);
        serial_println("");
    }
    
    /* Save crash dump */
    save_crash_dump(msg, 0);
    
    /* Print register state */
    serial_println("Register State:");
    serial_print("  RAX: "); serial_print_hex(g_crash_dump.rax);
    serial_print("  RBX: "); serial_print_hex(g_crash_dump.rbx);
    serial_print("  RCX: "); serial_print_hex(g_crash_dump.rcx);
    serial_print("  RDX: "); serial_print_hex(g_crash_dump.rdx);
    serial_putc('\r'); serial_putc('\n');
    
    serial_print("  RSI: "); serial_print_hex(g_crash_dump.rsi);
    serial_print("  RDI: "); serial_print_hex(g_crash_dump.rdi);
    serial_print("  RBP: "); serial_print_hex(g_crash_dump.rbp);
    serial_print("  RSP: "); serial_print_hex(g_crash_dump.rsp);
    serial_putc('\r'); serial_putc('\n');
    
    serial_print("  R8:  "); serial_print_hex(g_crash_dump.r8);
    serial_print("  R9:  "); serial_print_hex(g_crash_dump.r9);
    serial_print("  R10: "); serial_print_hex(g_crash_dump.r10);
    serial_print("  R11: "); serial_print_hex(g_crash_dump.r11);
    serial_putc('\r'); serial_putc('\n');
    
    serial_print("  R12: "); serial_print_hex(g_crash_dump.r12);
    serial_print("  R13: "); serial_print_hex(g_crash_dump.r13);
    serial_print("  R14: "); serial_print_hex(g_crash_dump.r14);
    serial_print("  R15: "); serial_print_hex(g_crash_dump.r15);
    serial_putc('\r'); serial_putc('\n');
    
    serial_print("  RIP: "); serial_print_hex(g_crash_dump.rip);
    serial_print("  CS:  "); serial_print_hex(g_crash_dump.cs);
    serial_print("  RFLAGS: "); serial_print_hex(g_crash_dump.rflags);
    serial_putc('\r'); serial_putc('\n');
    
    serial_println("");
    serial_println("Control Registers:");
    serial_print("  CR0: "); serial_print_hex(g_crash_dump.cr0);
    serial_print("  CR2: "); serial_print_hex(g_crash_dump.cr2);
    serial_putc('\r'); serial_putc('\n');
    serial_print("  CR3: "); serial_print_hex(g_crash_dump.cr3);
    serial_print("  CR4: "); serial_print_hex(g_crash_dump.cr4);
    serial_putc('\r'); serial_putc('\n');
    
    serial_println("");
    serial_println("Stack dump (first 256 bytes):");
    print_hexdump(g_crash_dump.rsp, 256);
    
    serial_println("");
    serial_println("========================================");
    serial_println("System halted. Please reboot.");
    serial_println("========================================");
    
    /* Halt all CPUs */
    while (1) {
        /* Send IPI to halt other CPUs (SMP only) */
        cpu_halt();
    }
}

/* Assert failure handler */
void assert_fail(const char *expr, const char *file, int line, const char *func) {
    char msg[256];
    
    snprintf(msg, sizeof(msg), "Assertion failed: %s at %s:%d in %s()",
             expr, file, line, func);
    
    kernel_panic(msg);
}
