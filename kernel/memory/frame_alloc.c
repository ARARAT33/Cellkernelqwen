/*
 * CellKernel - Physical Frame Allocator (Buddy System)
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

#include "../include/kernel.h"

#define PAGE_SIZE       4096
#define PAGE_SHIFT      12
#define MAX_ORDER       11              /* 2^11 = 2048 pages = 8MB */
#define NR_FREE_AREA    (MAX_ORDER + 1)

/* Free area structure for buddy system */
typedef struct {
    uint64_t nr_free;                   /* Number of free pages */
    void *free_list;                    /* Linked list of free blocks */
} free_area_t;

/* Zone structure */
typedef struct {
    free_area_t free_area[NR_FREE_AREA];
    uint64_t zone_start_pfn;
    uint64_t zone_size;
    uint64_t *bitmap;                   /* Allocation bitmap */
    uint64_t bitmap_size;
} zone_t;

/* Global zones */
#define MAX_ZONES       3
#define ZONE_DMA        0
#define ZONE_NORMAL     1
#define ZONE_HIGHMEM    2

static zone_t g_zones[MAX_ZONES];
static uint64_t g_total_pages = 0;
static uint64_t g_free_pages = 0;

/* Bitmap operations */
static inline void bitmap_set(uint64_t *map, uint64_t bit) {
    map[bit / 64] |= (1ULL << (bit % 64));
}

static inline void bitmap_clear(uint64_t *map, uint64_t bit) {
    map[bit / 64] &= ~(1ULL << (bit % 64));
}

static inline int bitmap_test(uint64_t *map, uint64_t bit) {
    return (map[bit / 64] >> (bit % 64)) & 1;
}

static inline int bitmap_find_zero(uint64_t *map, uint64_t size, uint64_t n) {
    uint64_t i, j;
    for (i = 0; i < size; i++) {
        if (map[i] == 0xFFFFFFFFFFFFFFFFULL) continue;
        for (j = 0; j < 64; j++) {
            if (!((map[i] >> j) & 1)) {
                return i * 64 + j;
            }
        }
    }
    return -1;
}

/* Get buddy address */
static inline void *get_buddy(void *page, uint64_t order) {
    uint64_t pfn = (uint64_t)page >> PAGE_SHIFT;
    uint64_t buddy_pfn = pfn ^ (1UL << order);
    return (void *)(buddy_pfn << PAGE_SHIFT);
}

/* Initialize frame allocator with memory map from EFI */
void frame_allocator_init(efi_memory_descriptor_t *memory_map, 
                          uint64_t map_size, uint64_t desc_size) {
    uint64_t entries = map_size / desc_size;
    uint64_t i;
    
    efi_memory_descriptor_t *desc = memory_map;
    for (i = 0; i < entries; i++) {
        if (desc->type == EfiConventionalMemory) {
            uint64_t start_pfn = (uint64_t)desc->physical_start >> PAGE_SHIFT;
            uint64_t num_pages = desc->num_pages;
            
            /* Determine zone */
            int zone_idx = ZONE_NORMAL;
            if ((uint64_t)desc->physical_start < 0x10000000ULL) {
                zone_idx = ZONE_DMA;
            }
            
            zone_t *zone = &g_zones[zone_idx];
            if (zone->zone_start_pfn == 0) {
                zone->zone_start_pfn = start_pfn;
            }
            zone->zone_size += num_pages;
            
            g_total_pages += num_pages;
            g_free_pages += num_pages;
        }
        
        desc = (efi_memory_descriptor_t *)((uint64_t)desc + desc_size);
    }
    
    /* Initialize free lists */
    for (i = 0; i < MAX_ZONES; i++) {
        int j;
        for (j = 0; j < NR_FREE_AREA; j++) {
            g_zones[i].free_area[j].nr_free = 0;
            g_zones[i].free_area[j].free_list = NULL;
        }
    }
}

/* Allocate a single page from specified zone */
static void *alloc_pages_zone(zone_t *zone, uint64_t order) {
    if (order >= NR_FREE_AREA) {
        return NULL;
    }
    
    free_area_t *area = &zone->free_area[order];
    if (area->nr_free == 0) {
        /* Try higher order and split */
        if (order + 1 < NR_FREE_AREA) {
            void *page = alloc_pages_zone(zone, order + 1);
            if (page) {
                /* Split the block */
                void *buddy = get_buddy(page, order);
                zone->free_area[order].free_list = buddy;
                zone->free_area[order].nr_free++;
                return page;
            }
        }
        return NULL;
    }
    
    /* Remove from free list */
    void *page = area->free_list;
    area->free_list = *(void **)page;
    area->nr_free--;
    
    g_free_pages -= (1UL << order);
    
    return page;
}

/* Allocate contiguous physical pages */
void *alloc_pages(uint64_t order, int zone_idx) {
    if (zone_idx < 0 || zone_idx >= MAX_ZONES) {
        zone_idx = ZONE_NORMAL;
    }
    
    return alloc_pages_zone(&g_zones[zone_idx], order);
}

/* Allocate single page */
void *alloc_page(int zone_idx) {
    return alloc_pages(0, zone_idx);
}

/* Free pages back to buddy system */
void free_pages(void *page, uint64_t order) {
    if (!page) return;
    
    uint64_t pfn = (uint64_t)page >> PAGE_SHIFT;
    int zone_idx = ZONE_NORMAL;
    
    if (pfn < 0x10000000ULL >> PAGE_SHIFT) {
        zone_idx = ZONE_DMA;
    }
    
    zone_t *zone = &g_zones[zone_idx];
    
    /* Coalesce with buddies */
    while (order < MAX_ORDER) {
        void *buddy = get_buddy(page, order);
        
        /* Check if buddy is free */
        if (zone->free_area[order].free_list == buddy) {
            /* Merge with buddy */
            zone->free_area[order].free_list = *(void **)buddy;
            zone->free_area[order].nr_free--;
            page = (void *)((uint64_t)page & (uint64_t)buddy);
            order++;
        } else {
            break;
        }
    }
    
    /* Add to free list */
    *(void **)page = zone->free_area[order].free_list;
    zone->free_area[order].free_list = page;
    zone->free_area[order].nr_free++;
    
    g_free_pages += (1UL << order);
}

/* Free single page */
void free_page(void *page) {
    free_pages(page, 0);
}

/* Get statistics */
uint64_t get_total_pages(void) {
    return g_total_pages;
}

uint64_t get_free_pages(void) {
    return g_free_pages;
}

uint64_t get_used_pages(void) {
    return g_total_pages - g_free_pages;
}
