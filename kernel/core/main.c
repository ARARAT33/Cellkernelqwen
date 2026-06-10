/*
 * CellKernel - Main Kernel Entry Point
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 * 
 * True microkernel architecture with capability-based security
 */

#include <stdint.h>
#include <stddef.h>
#include "../include/kernel.h"

/* Kernel version */
#define CELLKERNEL_VERSION_MAJOR    1
#define CELLKERNEL_VERSION_MINOR    0
#define CELLKERNEL_VERSION_PATCH    0

/* Kernel command line buffer */
static char g_kernel_cmdline[256] = {0};

/* Memory map from bootloader */
static efi_memory_descriptor_t *g_memory_map = NULL;
static uint64_t g_memory_map_size = 0;
static uint64_t g_descriptor_size = 0;

/* Kernel sections (defined in linker script) */
extern uint64_t _kernel_start;
extern uint64_t _kernel_end;
extern uint64_t _text_start;
extern uint64_t _text_end;
extern uint64_t _data_start;
extern uint64_t _data_end;
extern uint64_t _bss_start;
extern uint64_t _bss_end;

/* External initialization functions */
extern void hal_early_init(void);
extern void memory_manager_init(void);
extern void scheduler_init(void);
extern void interrupt_controller_init(void);
extern void syscall_init(void);
extern void cell_subsystem_init(void);
extern void ipc_subsystem_init(void);
extern void capability_system_init(void);
extern void security_subsystem_init(void);
extern void driver_framework_init(void);
extern void vfs_init(void);

/* Forward declarations */
static void print_banner(void);
static void print_memory_map(void);
static void kernel_panic(const char *msg);

/* Simple serial output for early boot logging */
static void serial_putc(char c) {
    volatile uint32_t *serial = (volatile uint32_t *)0x3F8;
    while (!((serial[5] >> 5) & 1));
    serial[0] = (uint32_t)c;
}

static void serial_print(const char *str) {
    while (*str) {
        serial_putc(*str++);
    }
}

static void serial_print_hex(uint64_t val) {
    char hex[] = "0123456789ABCDEF";
    char buf[18];
    int i;
    
    buf[16] = '\0';
    for (i = 15; i >= 0; i--) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    serial_print(buf);
}

static void serial_println(const char *str) {
    serial_print(str);
    serial_putc('\r');
    serial_putc('\n');
}

/* Print kernel banner */
static void print_banner(void) {
    serial_println("");
    serial_println("  ____           _         __  __             _");
    serial_println(" / ___|___   ___| | _____ |  \\/  | __ _ _ __ (_)_ __   ___");
    serial_println("| |   / _ \\ / __| |/ / _ \\| |\\/| |/ _` | '_ \\| | '_ \\ / _ \\");
    serial_println("| |__| (_) | (__|   < (_) | |  | | (_| | | | | | | | |  __/");
    serial_println(" \\____\\___/ \\___|_|\\_\\___/|_|  |_|\\__,_|_| |_|_|_| |_|\\___|");
    serial_println("");
    serial_println("CellKernel v" CELLKERNEL_VERSION_MAJOR "." 
                          CELLKERNEL_VERSION_MINOR "." 
                          CELLKERNEL_VERSION_PATCH);
    serial_println("True Microkernel Architecture - Zero Trust Security");
    serial_println("Copyright (c) 2024 CellKernel Project");
    serial_println("");
}

/* Print memory map from EFI */
static void print_memory_map(void) {
    uint64_t entries = g_memory_map_size / g_descriptor_size;
    uint64_t i;
    
    serial_println("[*] Memory Map:");
    serial_println("    Type          Physical Start     Pages      Attributes");
    
    efi_memory_descriptor_t *desc = g_memory_map;
    for (i = 0; i < entries; i++) {
        serial_print("    ");
        serial_print_hex(desc->type);
        serial_print("  ");
        serial_print_hex((uint64_t)desc->physical_start);
        serial_print("  ");
        serial_print_hex(desc->num_pages);
        serial_print("  ");
        serial_print_hex(desc->attribute);
        serial_putc('\r');
        serial_putc('\n');
        
        desc = (efi_memory_descriptor_t *)((uint64_t)desc + g_descriptor_size);
    }
}

/* Kernel panic handler */
static void kernel_panic(const char *msg) {
    serial_println("");
    serial_println("!!! KERNEL PANIC !!!");
    serial_print(msg);
    serial_println("");
    serial_println("System halted.");
    
    /* Disable interrupts */
    __asm__ volatile ("cli");
    
    /* Halt CPU */
    while (1) {
        __asm__ volatile ("hlt");
    }
}

/* Early kernel initialization */
static void kernel_early_init(void) {
    serial_println("[*] Early kernel initialization...");
    
    /* Initialize HAL */
    hal_early_init();
    serial_println("[+] HAL initialized");
}

/* Main kernel initialization */
static void kernel_main_init(void) {
    serial_println("[*] Initializing subsystems...");
    
    /* Initialize memory manager first */
    memory_manager_init();
    serial_println("[+] Memory manager initialized");
    
    /* Initialize interrupt controller */
    interrupt_controller_init();
    serial_println("[+] Interrupt controller initialized");
    
    /* Initialize scheduler */
    scheduler_init();
    serial_println("[+] Scheduler initialized");
    
    /* Initialize syscall interface */
    syscall_init();
    serial_println("[+] Syscall interface initialized");
    
    /* Initialize capability system */
    capability_system_init();
    serial_println("[+] Capability system initialized");
    
    /* Initialize cell subsystem */
    cell_subsystem_init();
    serial_println("[+] Cell subsystem initialized");
    
    /* Initialize IPC subsystem */
    ipc_subsystem_init();
    serial_println("[+] IPC subsystem initialized");
    
    /* Initialize security subsystem */
    security_subsystem_init();
    serial_println("[+] Security subsystem initialized");
    
    /* Initialize driver framework */
    driver_framework_init();
    serial_println("[+] Driver framework initialized");
    
    /* Initialize VFS */
    vfs_init();
    serial_println("[+] VFS initialized");
    
    serial_println("[+] All subsystems initialized successfully");
}

/* Idle loop */
static void kernel_idle(void) {
    serial_println("[*] Entering idle loop...");
    
    while (1) {
        /* Enable interrupts and halt */
        __asm__ volatile ("sti; hlt");
        
        /* Scheduler will be invoked on interrupt */
    }
}

/* Kernel entry point called from bootloader */
void kernel_main(void *memory_map, uint64_t map_size, uint64_t desc_size,
                 void *efi_table) {
    /* Store memory map info */
    g_memory_map = (efi_memory_descriptor_t *)memory_map;
    g_memory_map_size = map_size;
    g_descriptor_size = desc_size;
    
    /* Print banner */
    print_banner();
    
    /* Print kernel location */
    serial_print("[*] Kernel loaded at: ");
    serial_print_hex((uint64_t)&_kernel_start);
    serial_print(" - ");
    serial_print_hex((uint64_t)&_kernel_end);
    serial_println("");
    
    /* Print memory map */
    print_memory_map();
    
    /* Early initialization */
    kernel_early_init();
    
    /* Main initialization */
    kernel_main_init();
    
    serial_println("");
    serial_println("[+] CellKernel booted successfully!");
    serial_println("[*] Starting initial cell...");
    
    /* Enter idle loop - scheduler will run cells */
    kernel_idle();
}

/* Stub implementations for uninitialized subsystems */
__attribute__((weak))
void hal_early_init(void) {
    serial_println("[!] HAL early init stub");
}

__attribute__((weak))
void memory_manager_init(void) {
    serial_println("[!] Memory manager init stub");
}

__attribute__((weak))
void scheduler_init(void) {
    serial_println("[!] Scheduler init stub");
}

__attribute__((weak))
void interrupt_controller_init(void) {
    serial_println("[!] Interrupt controller init stub");
}

__attribute__((weak))
void syscall_init(void) {
    serial_println("[!] Syscall init stub");
}

__attribute__((weak))
void cell_subsystem_init(void) {
    serial_println("[!] Cell subsystem init stub");
}

__attribute__((weak))
void ipc_subsystem_init(void) {
    serial_println("[!] IPC subsystem init stub");
}

__attribute__((weak))
void capability_system_init(void) {
    serial_println("[!] Capability system init stub");
}

__attribute__((weak))
void security_subsystem_init(void) {
    serial_println("[!] Security subsystem init stub");
}

__attribute__((weak))
void driver_framework_init(void) {
    serial_println("[!] Driver framework init stub");
}

__attribute__((weak))
void vfs_init(void) {
    serial_println("[!] VFS init stub");
}
