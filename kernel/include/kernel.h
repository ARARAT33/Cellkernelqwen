/*
 * CellKernel - Main Kernel Header
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

#ifndef CELLKERNEL_KERNEL_H
#define CELLKERNEL_KERNEL_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * EFI Memory Types and Structures
 * ============================================================================ */

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiMaxMemoryType
} efi_memory_type_t;

typedef struct {
    uint32_t type;
    void *physical_start;
    void *virtual_start;
    uint64_t num_pages;
    uint64_t attribute;
} efi_memory_descriptor_t;

/* ============================================================================
 * Error Codes
 * ============================================================================ */

typedef int32_t ck_status_t;

#define CK_SUCCESS              0
#define CK_ERROR               -1
#define CK_ERROR_INVALID_ARG   -2
#define CK_ERROR_NO_MEMORY     -3
#define CK_ERROR_NOT_FOUND     -4
#define CK_ERROR_ALREADY_EXISTS -5
#define CK_ERROR_BUSY          -6
#define CK_ERROR_TIMEOUT       -7
#define CK_ERROR_ACCESS_DENIED -8
#define CK_ERROR_NOT_SUPPORTED -9
#define CK_ERROR_OVERFLOW      -10
#define CK_ERROR_UNDERFLOW     -11

/* ============================================================================
 * Cell ID and Capability Types
 * ============================================================================ */

typedef uint64_t cell_id_t;
typedef uint64_t capability_t;
typedef uint64_t handle_t;

#define CELL_ID_NULL        0
#define CELL_ID_KERNEL      1
#define CELL_ID_INIT        2
#define CELL_ID_MAX         0xFFFFFFFFFFFFFFFFULL

#define CAPABILITY_NONE     0
#define CAPABILITY_ALL      0xFFFFFFFFFFFFFFFFULL

/* ============================================================================
 * Memory Region Structure
 * ============================================================================ */

typedef struct {
    void *base;
    size_t size;
    uint32_t flags;
} memory_region_t;

#define MEM_FLAG_READ       0x01
#define MEM_FLAG_WRITE      0x02
#define MEM_FLAG_EXECUTE    0x04
#define MEM_FLAG_USER       0x08
#define MEM_FLAG_KERNEL     0x10
#define MEM_FLAG_DEVICE     0x20
#define MEM_FLAG_CACHEABLE  0x40
#define MEM_FLAG_UNCACHED   0x80

/* ============================================================================
 * IPC Message Structure
 * ============================================================================ */

#define IPC_MESSAGE_MAX_SIZE  4096

typedef struct {
    cell_id_t sender;
    cell_id_t receiver;
    uint32_t type;
    uint32_t flags;
    uint64_t data_size;
    uint8_t data[IPC_MESSAGE_MAX_SIZE];
} ipc_message_t;

#define IPC_TYPE_REQUEST      0
#define IPC_TYPE_RESPONSE     1
#define IPC_TYPE_NOTIFICATION 2
#define IPC_TYPE_SIGNAL       3

/* ============================================================================
 * Scheduler Types
 * ============================================================================ */

typedef enum {
    THREAD_STATE_UNUSED = 0,
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING,
    THREAD_STATE_BLOCKED,
    THREAD_STATE_SLEEPING,
    THREAD_STATE_ZOMBIE
} thread_state_t;

typedef enum {
    PRIORITY_IDLE = 0,
    PRIORITY_LOW = 1,
    PRIORITY_NORMAL = 2,
    PRIORITY_HIGH = 3,
    PRIORITY_REALTIME = 4
} thread_priority_t;

typedef struct {
    cell_id_t cell_id;
    uint64_t thread_id;
    thread_state_t state;
    thread_priority_t priority;
    void *stack_base;
    size_t stack_size;
    void *instruction_pointer;
} thread_info_t;

/* ============================================================================
 * Kernel Utility Macros
 * ============================================================================ */

#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))
#define ALIGN_UP(x, align)  (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define OFFSETOF(type, member) __builtin_offsetof(type, member)
#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - OFFSETOF(type, member)))

/* ============================================================================
 * Inline Assembly Helpers
 * ============================================================================ */

static inline void cpu_halt(void) {
    __asm__ volatile ("hlt");
}

static inline void cpu_nop(void) {
    __asm__ volatile ("nop");
}

static inline void cpu_cli(void) {
    __asm__ volatile ("cli");
}

static inline void cpu_sti(void) {
    __asm__ volatile ("sti");
}

static inline uint64_t cpu_read_rflags(void) {
    uint64_t flags;
    __asm__ volatile ("pushfq; pop %0" : "=r"(flags));
    return flags;
}

static inline uint64_t cpu_read_rsp(void) {
    uint64_t rsp;
    __asm__ volatile ("mov %%rsp, %0" : "=r"(rsp));
    return rsp;
}

static inline uint64_t cpu_read_rip(void) {
    uint64_t rip;
    __asm__ volatile ("call 0f; 0: pop %0" : "=r"(rip));
    return rip;
}

/* ============================================================================
 * Memory Barriers
 * ============================================================================ */

#define mb()      __asm__ volatile ("mfence" ::: "memory")
#define rmb()     __asm__ volatile ("lfence" ::: "memory")
#define wmb()     __asm__ volatile ("sfence" ::: "memory")
#define barrier() __asm__ volatile ("" ::: "memory")

/* ============================================================================
 * Atomic Operations
 * ============================================================================ */

typedef struct {
    volatile int64_t counter;
} atomic_t;

#define ATOMIC_INIT(i)  { .counter = (i) }

static inline int64_t atomic_read(const atomic_t *a) {
    return a->counter;
}

static inline void atomic_set(atomic_t *a, int64_t v) {
    a->counter = v;
}

static inline void atomic_add(int64_t i, atomic_t *a) {
    __asm__ volatile ("lock; addq %1, %0"
                      : "+m"(a->counter)
                      : "ir"(i)
                      : "memory", "cc");
}

static inline void atomic_sub(int64_t i, atomic_t *a) {
    __asm__ volatile ("lock; subq %1, %0"
                      : "+m"(a->counter)
                      : "ir"(i)
                      : "memory", "cc");
}

static inline int64_t atomic_inc(atomic_t *a) {
    int64_t old_val;
    __asm__ volatile ("lock; xaddq %0, %1"
                      : "=r"(old_val), "+m"(a->counter)
                      : "0"(1)
                      : "memory", "cc");
    return old_val + 1;
}

static inline int64_t atomic_dec(atomic_t *a) {
    int64_t old_val;
    __asm__ volatile ("lock; xaddq %0, %1"
                      : "=r"(old_val), "+m"(a->counter)
                      : "0"(-1)
                      : "memory", "cc");
    return old_val - 1;
}

static inline int64_t atomic_cmpxchg(atomic_t *a, int64_t old_val, int64_t new_val) {
    int64_t prev;
    __asm__ volatile ("lock; cmpxchgq %2, %1"
                      : "=a"(prev), "+m"(a->counter)
                      : "r"(new_val), "0"(old_val)
                      : "memory", "cc");
    return prev;
}

/* ============================================================================
 * Bit Operations
 * ============================================================================ */

static inline void set_bit(int nr, volatile void *addr) {
    __asm__ volatile ("btsq %1, %0"
                      : "+m"(*(volatile uint64_t *)addr)
                      : "Ir"(nr)
                      : "cc");
}

static inline void clear_bit(int nr, volatile void *addr) {
    __asm__ volatile ("btrq %1, %0"
                      : "+m"(*(volatile uint64_t *)addr)
                      : "Ir"(nr)
                      : "cc");
}

static inline int test_bit(int nr, volatile void *addr) {
    int oldbit;
    __asm__ volatile ("btq %2, %1; sbbl %0, %0"
                      : "=r"(oldbit)
                      : "m"(*(volatile uint64_t *)addr), "Ir"(nr)
                      : "cc");
    return oldbit;
}

static inline int test_and_set_bit(int nr, volatile void *addr) {
    int oldbit;
    __asm__ volatile ("lock; btsq %2, %1; sbbl %0, %0"
                      : "=r"(oldbit), "+m"(*(volatile uint64_t *)addr)
                      : "Ir"(nr)
                      : "memory", "cc");
    return oldbit;
}

static inline int test_and_clear_bit(int nr, volatile void *addr) {
    int oldbit;
    __asm__ volatile ("lock; btrq %2, %1; sbbl %0, %0"
                      : "=r"(oldbit), "+m"(*(volatile uint64_t *)addr)
                      : "Ir"(nr)
                      : "memory", "cc");
    return oldbit;
}

/* ============================================================================
 * Compiler Attributes
 * ============================================================================ */

#define __packed        __attribute__((packed))
#define __aligned(x)    __attribute__((aligned(x)))
#define __noreturn      __attribute__((noreturn))
#define __always_inline inline __attribute__((always_inline))
#define __never_inline  __attribute__((noinline))
#define __weak          __attribute__((weak))
#define __pure          __attribute__((pure))
#define __const         __attribute__((const))
#define __unused        __attribute__((unused))
#define __maybe_unused  __attribute__((maybe_unused))
#define __likely(x)     __builtin_expect(!!(x), 1)
#define __unlikely(x)   __builtin_expect(!!(x), 0)

#endif /* CELLKERNEL_KERNEL_H */
