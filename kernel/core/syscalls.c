/*
 * CellKernel - Syscall Interface (Fast System Calls)
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

#include "../include/kernel.h"

#define MAX_SYSCALLS        512
#define SYSCALL_NONE        0

/* Syscall handler type */
typedef int64_t (*syscall_handler_t)(uint64_t arg1, uint64_t arg2, 
                                      uint64_t arg3, uint64_t arg4,
                                      uint64_t arg5, uint64_t arg6);

/* Syscall table */
static syscall_handler_t g_syscall_table[MAX_SYSCALLS];
static const char *g_syscall_names[MAX_SYSCALLS];

/* Syscall numbers */
enum syscall_nr {
    SYS_READ            = 0,
    SYS_WRITE           = 1,
    SYS_OPEN            = 2,
    SYS_CLOSE           = 3,
    SYS_STAT            = 4,
    SYS_FSTAT           = 5,
    SYS_LSTAT           = 6,
    SYS_POLL            = 7,
    SYS_LSEEK           = 8,
    SYS_MMAP            = 9,
    SYS_MPROTECT        = 10,
    SYS_MUNMAP          = 11,
    SYS_BRK             = 12,
    SYS_EXIT            = 60,
    SYS_EXIT_GROUP      = 231,
    SYS_FORK            = 57,
    SYS_EXECVE          = 59,
    SYS_WAIT4           = 61,
    SYS_KILL            = 62,
    SYS_GETPID          = 39,
    SYS_GETUID          = 102,
    SYS_GETEUID         = 107,
    SYS_ACCESS          = 21,
    SYS_PIPE            = 22,
    SYS_SELECT          = 23,
    SYS_SOCKET          = 41,
    SYS_CONNECT         = 42,
    SYS_SENDTO          = 44,
    SYS_RECVFROM        = 45,
    SYS_IOCTL           = 16,
    SYS_FCNTL           = 72,
    SYS_DUP             = 32,
    SYS_DUP2            = 33,
    SYS_NANOSLEEP       = 35,
    SYS_GETTIMEOFDAY    = 96,
    SYS_GETRLIMIT       = 97,
    SYS_SYSINFO         = 99,
    
    /* CellKernel-specific syscalls */
    SYS_CELL_CREATE     = 300,
    SYS_CELL_DESTROY    = 301,
    SYS_CELL_YIELD      = 302,
    SYS_IPC_SEND        = 303,
    SYS_IPC_RECV        = 304,
    SYS_CAPABILITY_GRANT = 305,
    SYS_CAPABILITY_REVOKE = 306,
};

/* Stub syscall implementations */
static int64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count) {
    /* TODO: Implement file read */
    return CK_ERROR_NOT_SUPPORTED;
}

static int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count) {
    /* TODO: Implement file write */
    return CK_ERROR_NOT_SUPPORTED;
}

static int64_t sys_exit(uint64_t status) {
    /* TODO: Implement process exit */
    return 0;
}

static int64_t sys_getpid(void) {
    /* TODO: Return actual PID */
    return 1;
}

static int64_t sys_cell_create(uint64_t config, uint64_t flags) {
    /* TODO: Implement cell creation */
    return CK_ERROR_NOT_SUPPORTED;
}

static int64_t sys_ipc_send(uint64_t channel, uint64_t msg, uint64_t size) {
    /* TODO: Implement IPC send */
    return CK_ERROR_NOT_SUPPORTED;
}

static int64_t sys_ipc_recv(uint64_t channel, uint64_t msg, uint64_t size) {
    /* TODO: Implement IPC receive */
    return CK_ERROR_NOT_SUPPORTED;
}

/* Register syscall handler */
int register_syscall(uint64_t nr, syscall_handler_t handler, const char *name) {
    if (nr >= MAX_SYSCALLS) {
        return CK_ERROR_INVALID_ARG;
    }
    
    g_syscall_table[nr] = handler;
    g_syscall_names[nr] = name;
    
    return CK_SUCCESS;
}

/* Syscall dispatcher */
static int64_t syscall_dispatch(uint64_t nr, uint64_t arg1, uint64_t arg2,
                                 uint64_t arg3, uint64_t arg4,
                                 uint64_t arg5, uint64_t arg6) {
    if (nr >= MAX_SYSCALLS) {
        return CK_ERROR_NOT_SUPPORTED;
    }
    
    syscall_handler_t handler = g_syscall_table[nr];
    if (!handler) {
        return CK_ERROR_NOT_SUPPORTED;
    }
    
    return handler(arg1, arg2, arg3, arg4, arg5, arg6);
}

/* Assembly syscall entry point wrapper */
void syscall_entry(void) {
    /* This is called from assembly stub that saves registers */
    /* Arguments are in: rdi, rsi, rdx, r10, r8, r9 */
    /* Syscall number is in rax */
    
    /* In real implementation, this would be inline assembly */
    /* that extracts arguments and calls syscall_dispatch */
}

/* Initialize syscall subsystem */
void syscall_init(void) {
    serial_println("[*] Initializing syscall interface...");
    
    /* Clear syscall table */
    memset(g_syscall_table, 0, sizeof(g_syscall_table));
    memset(g_syscall_names, 0, sizeof(g_syscall_names));
    
    /* Register standard POSIX syscalls */
    register_syscall(SYS_READ, (syscall_handler_t)sys_read, "read");
    register_syscall(SYS_WRITE, (syscall_handler_t)sys_write, "write");
    register_syscall(SYS_EXIT, (syscall_handler_t)sys_exit, "exit");
    register_syscall(SYS_GETPID, (syscall_handler_t)sys_getpid, "getpid");
    
    /* Register CellKernel-specific syscalls */
    register_syscall(SYS_CELL_CREATE, (syscall_handler_t)sys_cell_create, "cell_create");
    register_syscall(SYS_IPC_SEND, (syscall_handler_t)sys_ipc_send, "ipc_send");
    register_syscall(SYS_IPC_RECV, (syscall_handler_t)sys_ipc_recv, "ipc_recv");
    
    /* Set up SYSCALL/SYSRET MSRs for x86_64 fast syscalls */
    /* STAR/LSTAR MSR */
    uint64_t lstar = (uint64_t)syscall_entry;
    __asm__ volatile ("wrmsr" :: "c"(0xC0000082), "a"(lstar & 0xFFFFFFFF), "d"(lstar >> 32));
    
    /* SFMASK MSR - mask RFLAGS on syscall */
    __asm__ volatile ("wrmsr" :: "c"(0xC0000084), "a"(0x00200200), "d"(0));
    
    /* KERNEL_GSBASE MSR */
    __asm__ volatile ("wrmsr" :: "c"(0xC0000102), "a"(0), "d"(0));
    
    /* Enable SYSCALL instruction */
    uint64_t efer;
    __asm__ volatile ("rdmsr" : "=a"(efer & 0xFFFFFFFF), "=d"(efer >> 32) : "c"(0xC0000080));
    efer |= (1 << 0);  /* SCE - Syscall Extensions */
    __asm__ volatile ("wrmsr" :: "c"(0xC0000080), "a"(efer & 0xFFFFFFFF), "d"(efer >> 32));
    
    serial_println("[+] Syscall interface initialized");
}
