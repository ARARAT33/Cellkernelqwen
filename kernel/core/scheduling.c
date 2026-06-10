/*
 * CellKernel - O(1) Scheduler
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

#include "../include/kernel.h"

#define MAX_CPUS            256
#define MAX_THREADS         4096
#define NR_PRIORITY_LEVELS  5
#define TIME_SLICE_MS       10

/* Thread states defined in kernel.h */

/* Runqueue per CPU */
typedef struct {
    spinlock_t lock;
    thread_info_t *current;
    thread_info_t *idle_thread;
    uint64_t nr_running;
    uint64_t nr_switches;
    
    /* Priority arrays for O(1) scheduling */
    struct list_head queue[NR_PRIORITY_LEVELS];
    uint64_t bitmap;                    /* Bitmap of non-empty queues */
} runqueue_t;

/* Global runqueues */
static runqueue_t g_runqueues[MAX_CPUS];
static uint64_t g_cpu_count = 1;

/* External functions */
extern void switch_to(thread_info_t *prev, thread_info_t *next);
extern void arch_schedule(void);

/* Initialize runqueue */
static void init_runqueue(runqueue_t *rq) {
    int i;
    rq->lock = SPINLOCK_UNLOCKED;
    rq->current = NULL;
    rq->idle_thread = NULL;
    rq->nr_running = 0;
    rq->nr_switches = 0;
    rq->bitmap = 0;
    
    for (i = 0; i < NR_PRIORITY_LEVELS; i++) {
        list_init(&rq->queue[i]);
    }
}

/* Add thread to runqueue */
void sched_enqueue(thread_info_t *thread) {
    if (!thread || thread->state != THREAD_STATE_READY) {
        return;
    }
    
    int cpu = 0;  /* In SMP, would use thread->cpu_affinity */
    runqueue_t *rq = &g_runqueues[cpu];
    
    spin_lock_irqsave(&rq->lock);
    
    /* Add to priority queue */
    list_add_tail(&thread->run_list, &rq->queue[thread->priority]);
    
    /* Update bitmap */
    rq->bitmap |= (1ULL << thread->priority);
    rq->nr_running++;
    
    spin_unlock_irqrestore(&rq->lock);
}

/* Remove thread from runqueue */
void sched_dequeue(thread_info_t *thread) {
    if (!thread) {
        return;
    }
    
    int cpu = 0;
    runqueue_t *rq = &g_runqueues[cpu];
    
    spin_lock_irqsave(&rq->lock);
    
    list_del(&thread->run_list);
    
    /* Check if queue is empty */
    if (list_empty(&rq->queue[thread->priority])) {
        rq->bitmap &= ~(1ULL << thread->priority);
    }
    
    rq->nr_running--;
    
    spin_unlock_irqrestore(&rq->lock);
}

/* Get next runnable thread - O(1) */
static thread_info_t *get_next_thread(runqueue_t *rq) {
    if (rq->bitmap == 0) {
        return rq->idle_thread;
    }
    
    /* Find highest priority non-empty queue */
    int priority = __builtin_clzll(rq->bitmap) ^ 63;
    
    /* Get first thread from queue */
    thread_info_t *thread = list_first_entry(&rq->queue[priority], 
                                              thread_info_t, run_list);
    
    return thread;
}

/* Schedule function - O(1) */
void schedule(void) {
    int cpu = 0;  /* Would use smp_processor_id() in SMP */
    runqueue_t *rq = &g_runqueues[cpu];
    
    spin_lock_irqsave(&rq->lock);
    
    thread_info_t *prev = rq->current;
    thread_info_t *next = get_next_thread(rq);
    
    if (prev == next) {
        spin_unlock_irqrestore(&rq->lock);
        return;
    }
    
    /* Context switch */
    rq->current = next;
    rq->nr_switches++;
    
    spin_unlock_irqrestore(&rq->lock);
    
    /* Perform actual context switch */
    switch_to(prev, next);
}

/* Yield CPU voluntarily */
void sched_yield(void) {
    thread_info_t *current = NULL;  /* Would get from per-CPU data */
    
    if (current && current->state == THREAD_STATE_RUNNING) {
        current->state = THREAD_STATE_READY;
        sched_enqueue(current);
        schedule();
    }
}

/* Block current thread */
void sched_block(thread_info_t *thread) {
    if (!thread) {
        return;
    }
    
    thread->state = THREAD_STATE_BLOCKED;
    sched_dequeue(thread);
    schedule();
}

/* Wake up blocked thread */
void sched_wakeup(thread_info_t *thread) {
    if (!thread || thread->state != THREAD_STATE_BLOCKED) {
        return;
    }
    
    thread->state = THREAD_STATE_READY;
    sched_enqueue(thread);
}

/* Sleep for specified milliseconds */
void sched_sleep(uint64_t ms) {
    thread_info_t *current = NULL;  /* Would get from per-CPU data */
    
    if (!current) {
        return;
    }
    
    current->state = THREAD_STATE_SLEEPING;
    sched_dequeue(current);
    
    /* Set wake-up time */
    current->wake_time = get_system_time() + ms;
    
    /* Add to sleep queue (not shown) */
    
    schedule();
}

/* Timer tick handler */
void sched_tick(void) {
    int cpu = 0;
    runqueue_t *rq = &g_runqueues[cpu];
    thread_info_t *current = rq->current;
    
    if (!current || current == rq->idle_thread) {
        return;
    }
    
    /* Decrement time slice */
    if (--current->time_slice <= 0) {
        /* Time slice expired, preempt */
        current->state = THREAD_STATE_READY;
        current->time_slice = TIME_SLICE_MS;
        sched_enqueue(current);
        schedule();
    }
}

/* Create new thread */
thread_info_t *sched_create_thread(cell_id_t cell_id, void (*entry)(void),
                                    void *stack_base, size_t stack_size,
                                    thread_priority_t priority) {
    thread_info_t *thread = kmalloc(sizeof(thread_info_t));
    if (!thread) {
        return NULL;
    }
    
    memset(thread, 0, sizeof(thread_info_t));
    thread->cell_id = cell_id;
    thread->thread_id = atomic_inc(&g_thread_counter);
    thread->state = THREAD_STATE_READY;
    thread->priority = priority;
    thread->stack_base = stack_base;
    thread->stack_size = stack_size;
    thread->instruction_pointer = entry;
    thread->time_slice = TIME_SLICE_MS;
    
    /* Initialize architecture-specific context */
    arch_init_thread_context(thread, entry, stack_base);
    
    return thread;
}

/* Destroy thread */
void sched_destroy_thread(thread_info_t *thread) {
    if (!thread) {
        return;
    }
    
    sched_dequeue(thread);
    thread->state = THREAD_STATE_ZOMBIE;
    
    /* Free resources */
    kfree(thread->stack_base);
    kfree(thread);
}

/* Initialize scheduler subsystem */
void scheduler_init(void) {
    int i;
    
    /* Detect CPU count */
    g_cpu_count = detect_cpu_count();
    if (g_cpu_count > MAX_CPUS) {
        g_cpu_count = MAX_CPUS;
    }
    
    /* Initialize runqueues */
    for (i = 0; i < g_cpu_count; i++) {
        init_runqueue(&g_runqueues[i]);
    }
    
    /* Create idle threads */
    for (i = 0; i < g_cpu_count; i++) {
        thread_info_t *idle = sched_create_thread(CELL_ID_KERNEL, idle_loop,
                                                   alloc_page(0), PAGE_SIZE,
                                                   PRIORITY_IDLE);
        if (idle) {
            g_runqueues[i].idle_thread = idle;
            g_runqueues[i].current = idle;
        }
    }
}
