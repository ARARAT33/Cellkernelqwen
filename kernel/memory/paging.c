/*
 * CellKernel - Paging Subsystem (4/5-level paging)
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

#include "../include/kernel.h"

#define PAGE_SIZE           4096
#define PAGE_SHIFT          12
#define PTRS_PER_PTE        512
#define PTRS_PER_PMD        512
#define PTRS_PER_PUD        512
#define PTRS_PER_PGD        512

/* Page table entry flags */
#define PTE_PRESENT         0x001
#define PTE_WRITABLE        0x002
#define PTE_USER            0x004
#define PTE_PWT             0x008
#define PTE_PCD             0x010
#define PTE_ACCESSED        0x020
#define PTE_DIRTY           0x040
#define PTE_PAT             0x080
#define PTE_GLOBAL          0x100
#define PTE_NX              0x8000000000000000ULL

/* Page table levels */
typedef struct {
    uint64_t entries[PTRS_PER_PGD];
} pgd_t;

typedef struct {
    uint64_t entries[PTRS_PER_PUD];
} pud_t;

typedef struct {
    uint64_t entries[PTRS_PER_PMD];
} pmd_t;

typedef struct {
    uint64_t entries[PTRS_PER_PTE];
} pte_t;

/* Current page tables */
static pgd_t *g_kernel_pgd = NULL;
static uint64_t g_paging_enabled = 0;

/* External functions */
extern void *alloc_page(int zone_idx);
extern void free_page(void *page);
extern void *phys_to_virt(uint64_t phys);
extern uint64_t virt_to_phys(void *virt);

/* Get PGD entry */
static inline pgd_t *pgd_offset(pgd_t *pgd, uint64_t addr) {
    return &pgd[((addr >> 39) & (PTRS_PER_PGD - 1))];
}

/* Get PUD entry */
static inline pud_t *pud_offset(pgd_t *pgd, uint64_t addr) {
    uint64_t entry = pgd->entries[(addr >> 39) & (PTRS_PER_PGD - 1)];
    if (!(entry & PTE_PRESENT)) {
        return NULL;
    }
    return (pud_t *)phys_to_virt(entry & 0x000FFFFFFFFFF000ULL);
}

/* Get PMD entry */
static inline pmd_t *pmd_offset(pud_t *pud, uint64_t addr) {
    uint64_t entry = pud->entries[(addr >> 30) & (PTRS_PER_PUD - 1)];
    if (!(entry & PTE_PRESENT)) {
        return NULL;
    }
    return (pmd_t *)phys_to_virt(entry & 0x000FFFFFFFFFF000ULL);
}

/* Get PTE entry */
static inline pte_t *pte_offset(pmd_t *pmd, uint64_t addr) {
    uint64_t entry = pmd->entries[(addr >> 21) & (PTRS_PER_PMD - 1)];
    if (!(entry & PTE_PRESENT)) {
        return NULL;
    }
    return (pte_t *)phys_to_virt(entry & 0x000FFFFFFFFFF000ULL);
}

/* Create a new page table entry */
static uint64_t create_pte(uint64_t phys_addr, uint64_t flags) {
    return (phys_addr & 0x000FFFFFFFFFF000ULL) | flags;
}

/* Map a physical page to virtual address */
int map_page(pgd_t *pgd, uint64_t virt, uint64_t phys, uint64_t flags) {
    /* Get PGD entry */
    uint64_t pgd_idx = (virt >> 39) & (PTRS_PER_PGD - 1);
    uint64_t pgd_entry = pgd->entries[pgd_idx];
    
    pud_t *pud;
    if (!(pgd_entry & PTE_PRESENT)) {
        /* Allocate new PUD */
        pud = (pud_t *)alloc_page(0);
        if (!pud) {
            return CK_ERROR_NO_MEMORY;
        }
        memset(pud, 0, PAGE_SIZE);
        pgd->entries[pgd_idx] = create_pte(virt_to_phys(pud), PTE_PRESENT | PTE_WRITABLE);
    } else {
        pud = (pud_t *)phys_to_virt(pgd_entry & 0x000FFFFFFFFFF000ULL);
    }
    
    /* Get PUD entry */
    uint64_t pud_idx = (virt >> 30) & (PTRS_PER_PUD - 1);
    uint64_t pud_entry = pud->entries[pud_idx];
    
    pmd_t *pmd;
    if (!(pud_entry & PTE_PRESENT)) {
        /* Allocate new PMD */
        pmd = (pmd_t *)alloc_page(0);
        if (!pmd) {
            return CK_ERROR_NO_MEMORY;
        }
        memset(pmd, 0, PAGE_SIZE);
        pud->entries[pud_idx] = create_pte(virt_to_phys(pmd), PTE_PRESENT | PTE_WRITABLE);
    } else {
        pmd = (pmd_t *)phys_to_virt(pud_entry & 0x000FFFFFFFFFF000ULL);
    }
    
    /* Get PMD entry */
    uint64_t pmd_idx = (virt >> 21) & (PTRS_PER_PMD - 1);
    uint64_t pmd_entry = pmd->entries[pmd_idx];
    
    pte_t *pte;
    if (!(pmd_entry & PTE_PRESENT)) {
        /* Allocate new PTE */
        pte = (pte_t *)alloc_page(0);
        if (!pte) {
            return CK_ERROR_NO_MEMORY;
        }
        memset(pte, 0, PAGE_SIZE);
        pmd->entries[pmd_idx] = create_pte(virt_to_phys(pte), PTE_PRESENT | PTE_WRITABLE);
    } else {
        pte = (pte_t *)phys_to_virt(pmd_entry & 0x000FFFFFFFFFF000ULL);
    }
    
    /* Set PTE entry */
    uint64_t pte_idx = (virt >> 12) & (PTRS_PER_PTE - 1);
    pte->entries[pte_idx] = create_pte(phys, flags | PTE_PRESENT);
    
    /* Invalidate TLB */
    __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");
    
    return CK_SUCCESS;
}

/* Unmap a virtual page */
int unmap_page(pgd_t *pgd, uint64_t virt) {
    pud_t *pud = pud_offset(pgd, virt);
    if (!pud) {
        return CK_ERROR_NOT_FOUND;
    }
    
    pmd_t *pmd = pmd_offset(pud, virt);
    if (!pmd) {
        return CK_ERROR_NOT_FOUND;
    }
    
    pte_t *pte = pte_offset(pmd, virt);
    if (!pte) {
        return CK_ERROR_NOT_FOUND;
    }
    
    uint64_t pte_idx = (virt >> 12) & (PTRS_PER_PTE - 1);
    pte->entries[pte_idx] = 0;
    
    /* Invalidate TLB */
    __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");
    
    return CK_SUCCESS;
}

/* Create identity mapping for kernel */
static int create_identity_map(pgd_t *pgd, uint64_t start, uint64_t end, uint64_t flags) {
    uint64_t addr;
    for (addr = start; addr < end; addr += PAGE_SIZE) {
        int ret = map_page(pgd, addr, addr, flags);
        if (ret != CK_SUCCESS) {
            return ret;
        }
    }
    return CK_SUCCESS;
}

/* Initialize paging subsystem */
void paging_init(void) {
    /* Allocate PGD */
    g_kernel_pgd = (pgd_t *)alloc_page(0);
    if (!g_kernel_pgd) {
        return;
    }
    memset(g_kernel_pgd, 0, PAGE_SIZE);
    
    /* Create identity mapping for low memory (for early boot) */
    create_identity_map(g_kernel_pgd, 0, 0x100000000ULL, 
                        PTE_PRESENT | PTE_WRITABLE | PTE_GLOBAL);
    
    /* Map kernel space */
    extern uint64_t _kernel_start, _kernel_end;
    uint64_t kernel_start = (uint64_t)&_kernel_start & ~(PAGE_SIZE - 1);
    uint64_t kernel_end = ((uint64_t)&_kernel_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    create_identity_map(g_kernel_pgd, kernel_start, kernel_end,
                        PTE_PRESENT | PTE_WRITABLE | PTE_GLOBAL | PTE_NX);
}

/* Enable paging */
void enable_paging(void) {
    if (!g_kernel_pgd) {
        return;
    }
    
    /* Load CR3 with PGD physical address */
    uint64_t cr3 = virt_to_phys(g_kernel_pgd);
    __asm__ volatile ("mov %0, %%cr3" :: "r"(cr3) : "memory");
    
    /* Enable PAE and PGE */
    uint64_t cr4;
    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 5);  /* PAE */
    cr4 |= (1 << 7);  /* PGE */
    __asm__ volatile ("mov %0, %%cr4" :: "r"(cr4) : "memory");
    
    /* Enable long mode and NX */
    uint64_t efer;
    __asm__ volatile ("mov %%ecx, %0" : "=r"(efer));
    __asm__ volatile ("rdmsr" :: "c"(0xC0000080));
    efer |= (1 << 8);   /* LME */
    efer |= (1 << 11);  /* NXE */
    __asm__ volatile ("wrmsr" :: "c"(0xC0000080), "a"(efer & 0xFFFFFFFF), "d"(efer >> 32));
    
    /* Enable paging */
    uint64_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1 << 31);   /* PG */
    cr0 |= (1 << 16);   /* WP */
    __asm__ volatile ("mov %0, %%cr0" :: "r"(cr0) : "memory");
    
    g_paging_enabled = 1;
}

/* Get current CR3 value */
uint64_t get_cr3(void) {
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

/* Flush TLB */
void flush_tlb(void) {
    uint64_t cr3 = get_cr3();
    __asm__ volatile ("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

/* Flush single page from TLB */
void flush_tlb_page(uint64_t virt) {
    __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");
}
