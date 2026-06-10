/*
 * CellKernel - Lock-free IPC Message Passing
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

#include "../include/kernel.h"

#define IPC_QUEUE_SIZE      1024
#define IPC_MAX_MESSAGE     4096
#define IPC_MAGIC           0xCAFEBABE

/* Lock-free MPSC queue node */
typedef struct ipc_node {
    struct ipc_node *next;
    ipc_message_t message;
    uint64_t timestamp;
    uint32_t magic;
} ipc_node_t;

/* Lock-free IPC channel */
typedef struct {
    cell_id_t sender_id;
    cell_id_t receiver_id;
    
    /* Lock-free queue head/tail */
    atomic_t head;
    atomic_t tail;
    
    /* Queue storage */
    ipc_node_t *nodes[IPC_QUEUE_SIZE];
    
    /* Statistics */
    atomic_t messages_sent;
    atomic_t messages_received;
    atomic_t messages_dropped;
    
    /* Rate limiting */
    uint64_t rate_limit;              /* Messages per second */
    uint64_t quota_remaining;         /* Remaining quota */
    uint64_t quota_reset_time;        /* When quota resets */
    
    uint32_t flags;
    uint32_t refcount;
} ipc_channel_t;

/* Global IPC channels */
#define MAX_CHANNELS        1024
static ipc_channel_t *g_channels[MAX_CHANNELS];
static atomic_t g_channel_counter = ATOMIC_INIT(0);

/* External functions */
extern void *kmalloc(size_t size);
extern void kfree(void *ptr);
extern uint64_t get_system_time(void);

/* Initialize IPC subsystem */
void ipc_subsystem_init(void) {
    int i;
    for (i = 0; i < MAX_CHANNELS; i++) {
        g_channels[i] = NULL;
    }
    atomic_set(&g_channel_counter, 0);
}

/* Create new IPC channel */
ipc_channel_t *ipc_channel_create(cell_id_t sender, cell_id_t receiver) {
    uint32_t channel_id = atomic_inc(&g_channel_counter);
    
    if (channel_id >= MAX_CHANNELS) {
        return NULL;
    }
    
    ipc_channel_t *channel = kmalloc(sizeof(ipc_channel_t));
    if (!channel) {
        return NULL;
    }
    
    memset(channel, 0, sizeof(ipc_channel_t));
    channel->sender_id = sender;
    channel->receiver_id = receiver;
    atomic_set(&channel->head, 0);
    atomic_set(&channel->tail, 0);
    channel->rate_limit = 1000;       /* Default: 1000 msg/sec */
    channel->quota_remaining = 10000; /* Default quota */
    channel->quota_reset_time = get_system_time() + 1000;
    channel->refcount = 1;
    
    /* Pre-allocate queue nodes */
    int i;
    for (i = 0; i < IPC_QUEUE_SIZE; i++) {
        channel->nodes[i] = kmalloc(sizeof(ipc_node_t));
        if (channel->nodes[i]) {
            memset(channel->nodes[i], 0, sizeof(ipc_node_t));
            channel->nodes[i]->magic = IPC_MAGIC;
        }
    }
    
    g_channels[channel_id] = channel;
    return channel;
}

/* Send message (lock-free) */
int ipc_send(ipc_channel_t *channel, ipc_message_t *msg) {
    if (!channel || !msg) {
        return CK_ERROR_INVALID_ARG;
    }
    
    /* Check rate limit */
    uint64_t now = get_system_time();
    if (now >= channel->quota_reset_time) {
        channel->quota_remaining = channel->rate_limit;
        channel->quota_reset_time = now + 1000;
    }
    
    if (channel->quota_remaining == 0) {
        atomic_inc(&channel->messages_dropped);
        return CK_ERROR_BUSY;
    }
    
    channel->quota_remaining--;
    
    /* Get next free slot */
    uint32_t tail = atomic_read(&channel->tail);
    uint32_t next_tail = (tail + 1) % IPC_QUEUE_SIZE;
    
    /* Check if queue is full */
    if (next_tail == atomic_read(&channel->head)) {
        atomic_inc(&channel->messages_dropped);
        return CK_ERROR_OVERFLOW;
    }
    
    /* Copy message to queue (lock-free) */
    ipc_node_t *node = channel->nodes[tail];
    if (!node || node->magic != IPC_MAGIC) {
        return CK_ERROR_NO_MEMORY;
    }
    
    memcpy(&node->message, msg, sizeof(ipc_message_t));
    node->timestamp = now;
    
    /* Memory barrier before updating tail */
    wmb();
    
    /* Update tail pointer */
    atomic_set(&channel->tail, next_tail);
    atomic_inc(&channel->messages_sent);
    
    return CK_SUCCESS;
}

/* Receive message (lock-free) */
int ipc_receive(ipc_channel_t *channel, ipc_message_t *msg) {
    if (!channel || !msg) {
        return CK_ERROR_INVALID_ARG;
    }
    
    uint32_t head = atomic_read(&channel->head);
    uint32_t tail = atomic_read(&channel->tail);
    
    /* Check if queue is empty */
    if (head == tail) {
        return CK_ERROR_NOT_FOUND;
    }
    
    /* Copy message from queue */
    ipc_node_t *node = channel->nodes[head];
    if (!node || node->magic != IPC_MAGIC) {
        return CK_ERROR;
    }
    
    memcpy(msg, &node->message, sizeof(ipc_message_t));
    
    /* Memory barrier before updating head */
    rmb();
    
    /* Update head pointer */
    uint32_t next_head = (head + 1) % IPC_QUEUE_SIZE;
    atomic_set(&channel->head, next_head);
    atomic_inc(&channel->messages_received);
    
    return CK_SUCCESS;
}

/* Destroy IPC channel */
void ipc_channel_destroy(ipc_channel_t *channel) {
    if (!channel) {
        return;
    }
    
    if (--channel->refcount > 0) {
        return;
    }
    
    int i;
    for (i = 0; i < IPC_QUEUE_SIZE; i++) {
        if (channel->nodes[i]) {
            kfree(channel->nodes[i]);
        }
    }
    
    kfree(channel);
}

/* Set rate limit for channel */
void ipc_set_rate_limit(ipc_channel_t *channel, uint64_t limit) {
    if (channel) {
        channel->rate_limit = limit;
    }
}

/* Get channel statistics */
void ipc_get_stats(ipc_channel_t *channel, uint64_t *sent, uint64_t *recv, uint64_t *dropped) {
    if (!channel) {
        return;
    }
    
    if (sent) *sent = atomic_read(&channel->messages_sent);
    if (recv) *recv = atomic_read(&channel->messages_received);
    if (dropped) *dropped = atomic_read(&channel->messages_dropped);
}
