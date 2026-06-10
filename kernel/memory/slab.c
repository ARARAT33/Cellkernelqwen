/*
 * CellKernel - Kernel Memory Allocator (SLAB/SLUB)
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

#include "../include/kernel.h"

#define PAGE_SIZE           4096
#define SLAB_MAGIC          0xDEADBEEFCAFEBABEULL

/* Slab cache flags */
#define SLAB_FLAG_NONE      0x00
#define SLAB_FLAG_HWCACHE   0x01
#define SLAB_FLAG_POISON    0x02
#define SLAB_FLAG_REDZONE   0x04

/* Slab object status */
#define OBJ_FREE            0x00
#define OBJ_USED            0x01
#define OBJ_FULL            0x02

/* Slab descriptor */
typedef struct slab {
    struct slab_cache *cache;           /* Parent cache */
    uint64_t magic;                     /* Magic number for validation */
    uint32_t inuse;                     /* Number of used objects */
    uint32_t total;                     /* Total objects in slab */
    void *freelist;                     /* Free list head */
    uint8_t *objects;                   /* Object array */
    uint8_t *bitmap;                    /* Allocation bitmap */
    struct slab *next;                  /* Next slab in list */
    struct slab *prev;                  /* Previous slab in list */
} slab_t;

/* Slab cache */
typedef struct slab_cache {
    char name[32];                      /* Cache name */
    size_t object_size;                 /* Object size */
    size_t aligned_size;                /* Aligned object size */
    size_t slab_size;                   /* Slab size in bytes */
    uint32_t objects_per_slab;          /* Objects per slab */
    uint32_t flags;                     /* Cache flags */
    uint32_t color;                     /* Cache coloring offset */
    slab_t *slabs_partial;              /* Partially allocated slabs */
    slab_t *slabs_full;                 /* Fully allocated slabs */
    slab_t *slabs_free;                 /* Free slabs */
    atomic_t nr_objects;                /* Total objects */
    atomic_t nr_slabs;                  /* Total slabs */
} slab_cache_t;

/* Maximum caches */
#define MAX_CACHES          64
#define MIN_OBJECT_SIZE     32
#define MAX_OBJECT_SIZE     (PAGE_SIZE / 2)

/* Global slab caches */
static slab_cache_t g_caches[MAX_CACHES];
static uint32_t g_cache_count = 0;

/* External functions from frame allocator */
extern void *alloc_page(int zone_idx);
extern void free_page(void *page);

/* Initialize a slab */
static void slab_init(slab_t *slab, slab_cache_t *cache, void *memory) {
    slab->cache = cache;
    slab->magic = SLAB_MAGIC;
    slab->inuse = 0;
    slab->total = cache->objects_per_slab;
    slab->objects = (uint8_t *)memory + cache->color;
    slab->freelist = slab->objects;
    slab->next = NULL;
    slab->prev = NULL;
    
    /* Initialize bitmap */
    slab->bitmap = (uint8_t *)calloc(cache->objects_per_slab / 8 + 1, 1);
    
    /* Build freelist */
    size_t i;
    uint8_t *obj = slab->objects;
    for (i = 0; i < cache->objects_per_slab - 1; i++) {
        *(void **)obj = obj + cache->aligned_size;
        obj += cache->aligned_size;
    }
    *(void **)obj = NULL;
}

/* Create a new slab cache */
slab_cache_t *kmem_cache_create(const char *name, size_t size, uint32_t flags) {
    if (g_cache_count >= MAX_CACHES) {
        return NULL;
    }
    
    if (size < MIN_OBJECT_SIZE || size > MAX_OBJECT_SIZE) {
        return NULL;
    }
    
    slab_cache_t *cache = &g_caches[g_cache_count++];
    
    /* Initialize cache */
    memset(cache, 0, sizeof(slab_cache_t));
    strncpy(cache->name, name, sizeof(cache->name) - 1);
    cache->object_size = size;
    cache->flags = flags;
    
    /* Align object size to cache line (typically 64 bytes) */
    cache->aligned_size = ALIGN_UP(size, 64);
    
    /* Calculate slab size and objects per slab */
    cache->slab_size = PAGE_SIZE;
    cache->objects_per_slab = (cache->slab_size - sizeof(slab_t)) / cache->aligned_size;
    
    /* Apply color for cache alignment */
    if (flags & SLAB_FLAG_HWCACHE) {
        cache->color = (g_cache_count % 8) * 64;
    }
    
    return cache;
}

/* Allocate object from cache */
void *kmem_cache_alloc(slab_cache_t *cache) {
    if (!cache) {
        return NULL;
    }
    
    slab_t *slab = cache->slabs_partial;
    
    /* Try partial slabs first */
    if (!slab && cache->slabs_free) {
        /* Move a free slab to partial */
        slab = cache->slabs_free;
        cache->slabs_free = slab->next;
        if (cache->slabs_free) {
            cache->slabs_free->prev = NULL;
        }
        
        /* Add to partial list */
        slab->next = cache->slabs_partial;
        slab->prev = NULL;
        if (cache->slabs_partial) {
            cache->slabs_partial->prev = slab;
        }
        cache->slabs_partial = slab;
    } else if (!slab) {
        /* Allocate new slab */
        void *memory = alloc_page(0);
        if (!memory) {
            return NULL;
        }
        
        slab = (slab_t *)memory;
        memory = (uint8_t *)memory + ALIGN_UP(sizeof(slab_t), cache->aligned_size);
        
        slab_init(slab, cache, memory);
        
        /* Add to partial list */
        slab->next = cache->slabs_partial;
        slab->prev = NULL;
        if (cache->slabs_partial) {
            cache->slabs_partial->prev = slab;
        }
        cache->slabs_partial = slab;
        
        atomic_inc(&cache->nr_slabs);
    }
    
    /* Allocate from freelist */
    void *obj = slab->freelist;
    if (!obj) {
        return NULL;
    }
    
    slab->freelist = *(void **)obj;
    slab->inuse++;
    
    /* Check if slab is now full */
    if (slab->inuse == slab->total) {
        /* Move to full list */
        if (slab->prev) {
            slab->prev->next = slab->next;
        } else {
            cache->slabs_partial = slab->next;
        }
        
        if (slab->next) {
            slab->next->prev = slab->prev;
        }
        
        slab->next = cache->slabs_full;
        slab->prev = NULL;
        if (cache->slabs_full) {
            cache->slabs_full->prev = slab;
        }
        cache->slabs_full = slab;
    }
    
    atomic_inc(&cache->nr_objects);
    
    /* Poison if enabled */
    if (cache->flags & SLAB_FLAG_POISON) {
        memset(obj, 0xCC, cache->object_size);
    }
    
    return obj;
}

/* Free object back to cache */
void kmem_cache_free(slab_cache_t *cache, void *obj) {
    if (!cache || !obj) {
        return;
    }
    
    /* Find the slab containing this object */
    slab_t *slab = cache->slabs_partial;
    while (slab) {
        if ((uint8_t *)obj >= slab->objects && 
            (uint8_t *)obj < slab->objects + (slab->total * cache->aligned_size)) {
            break;
        }
        slab = slab->next;
    }
    
    if (!slab) {
        slab = cache->slabs_full;
        while (slab) {
            if ((uint8_t *)obj >= slab->objects && 
                (uint8_t *)obj < slab->objects + (slab->total * cache->aligned_size)) {
                break;
            }
            slab = slab->next;
        }
    }
    
    if (!slab || slab->magic != SLAB_MAGIC) {
        return;
    }
    
    /* Return object to freelist */
    *(void **)obj = slab->freelist;
    slab->freelist = obj;
    slab->inuse--;
    
    atomic_dec(&cache->nr_objects);
    
    /* Check if slab is now empty */
    if (slab->inuse == 0) {
        /* Remove from current list */
        if (slab->prev) {
            slab->prev->next = slab->next;
        } else {
            cache->slabs_partial = slab->next;
        }
        
        if (slab->next) {
            slab->next->prev = slab->prev;
        }
        
        /* Add to free list */
        slab->next = cache->slabs_free;
        slab->prev = NULL;
        if (cache->slabs_free) {
            cache->slabs_free->prev = slab;
        }
        cache->slabs_free = slab;
        
        atomic_dec(&cache->nr_slabs);
        
        /* Optionally free the slab if too many free slabs */
        /* Implementation depends on memory pressure */
    } else if (slab == cache->slabs_full) {
        /* Move from full to partial */
        cache->slabs_full = slab->next;
        if (cache->slabs_full) {
            cache->slabs_full->prev = NULL;
        }
        
        slab->next = cache->slabs_partial;
        slab->prev = NULL;
        if (cache->slabs_partial) {
            cache->slabs_partial->prev = slab;
        }
        cache->slabs_partial = slab;
    }
}

/* General kernel allocation */
void *kmalloc(size_t size) {
    /* Find appropriate cache */
    uint32_t i;
    for (i = 0; i < g_cache_count; i++) {
        if (g_caches[i].object_size >= size) {
            return kmem_cache_alloc(&g_caches[i]);
        }
    }
    
    /* Too large for slab, allocate pages directly */
    if (size > MAX_OBJECT_SIZE) {
        uint64_t order = 0;
        size_t alloc_size = size + PAGE_SIZE;
        while ((1UL << order) * PAGE_SIZE < alloc_size) {
            order++;
        }
        return alloc_pages(order, 0);
    }
    
    return NULL;
}

/* Free kernel memory */
void kfree(void *ptr) {
    if (!ptr) {
        return;
    }
    
    /* For now, assume it came from a slab cache */
    /* In production, would need to track which cache */
    uint32_t i;
    for (i = 0; i < g_cache_count; i++) {
        /* Check if pointer belongs to this cache */
        slab_t *slab = g_caches[i].slabs_partial;
        while (slab) {
            if ((uint8_t *)ptr >= slab->objects && 
                (uint8_t *)ptr < slab->objects + (slab->total * g_caches[i].aligned_size)) {
                kmem_cache_free(&g_caches[i], ptr);
                return;
            }
            slab = slab->next;
        }
    }
}

/* Initialize slab subsystem */
void slab_init_subsystem(void) {
    /* Create common caches */
    kmem_cache_create("tiny-32", 32, SLAB_FLAG_HWCACHE);
    kmem_cache_create("small-64", 64, SLAB_FLAG_HWCACHE);
    kmem_cache_create("medium-128", 128, SLAB_FLAG_HWCACHE);
    kmem_cache_create("large-256", 256, SLAB_FLAG_HWCACHE);
    kmem_cache_create("big-512", 512, SLAB_FLAG_HWCACHE);
    kmem_cache_create("huge-1024", 1024, SLAB_FLAG_HWCACHE);
    kmem_cache_create("task_struct", 2048, SLAB_FLAG_POISON);
}
