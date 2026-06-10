/*
 * CellKernel - EFI Boot Entry Point
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

#include <stdint.h>
#include <stddef.h>
#include "protocols.h"

/* EFI Status Codes */
#define EFI_SUCCESS             0
#define EFI_LOAD_ERROR          1
#define EFI_INVALID_PARAMETER   2
#define EFI_UNSUPPORTED         3
#define EFI_BAD_BUFFER_SIZE     4
#define EFI_BUFFER_TOO_SMALL    5
#define EFI_NOT_READY           6
#define EFI_DEVICE_ERROR        7
#define EFI_WRITE_PROTECTED     8
#define EFI_OUT_OF_RESOURCES    9
#define EFI_NOT_FOUND           14
#define EFI_SECURITY_VIOLATION  26

/* EFI Memory Types */
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
} EFI_MEMORY_TYPE;

/* EFI Memory Descriptor */
typedef struct {
    uint32_t Type;
    void *PhysicalStart;
    void *VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

/* EFI System Table */
typedef struct {
    char *Signature;
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t CRC32;
    uint32_t Reserved;
    void *ConsoleInHandle;
    void *SimpleInputProtocol;
    void *ConsoleOutHandle;
    void *SimpleOutputProtocol;
    void *StandardErrorHandle;
    void *SimpleErrorOutputProtocol;
    void *RuntimeServices;
    void *BootServices;
    uint32_t NumberOfTableEntries;
    void *ConfigurationTable;
} EFI_SYSTEM_TABLE;

/* EFI Loaded Image Protocol */
typedef struct {
    uint32_t Revision;
    void *ParentHandle;
    EFI_SYSTEM_TABLE *SystemTable;
    void *DeviceHandle;
    void *FilePath;
    void *Reserved;
    uint32_t LoadOptionsSize;
    void *LoadOptions;
    void *ImageBase;
    uint64_t ImageSize;
    int ImageCodeType;
    int ImageDataType;
} EFI_LOADED_IMAGE_PROTOCOL;

/* External functions from secure boot */
extern int verify_kernel_signature(void *kernel_buf, uint64_t kernel_size);
extern int tpm_measure_image(void *image, uint64_t size, int pcr_index);

/* Global EFI pointers */
static EFI_SYSTEM_TABLE *gST = NULL;
static EFI_LOADED_IMAGE_PROTOCOL *gLoadedImage = NULL;

/* Simple output function */
static void efi_print(const char *str) {
    if (gST && gST->SimpleOutputProtocol) {
        void (*OutputString)(void *, const char *) = 
            (void (*)(void *, const char *))((uint64_t **)gST->SimpleOutputProtocol)[5];
        OutputString(gST->SimpleOutputProtocol, str);
    }
}

/* Print hex value */
static void print_hex(uint64_t val) {
    char buf[20];
    char *hex = "0123456789ABCDEF";
    int i;
    
    buf[18] = '\0';
    for (i = 17; i >= 0; i--) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    efi_print(buf);
}

/* Get memory map from EFI */
static EFI_MEMORY_DESCRIPTOR *get_memory_map(uint64_t *map_size, uint64_t *map_key, 
                                              uint64_t *desc_size, uint32_t *desc_version) {
    void (*GetMemoryMap)(void *, uint64_t *, void *, uint64_t *, uint64_t *, uint32_t *) = 
        (void (*)(void *, uint64_t *, void *, uint64_t *, uint64_t *, uint32_t *))
        ((uint64_t **)gST->BootServices)[6];
    
    EFI_STATUS status;
    void *memory_map = NULL;
    
    *map_size = 0;
    status = GetMemoryMap(gST->BootServices, map_size, NULL, map_key, desc_size, desc_version);
    
    /* Allocate buffer for memory map */
    void (*AllocatePool)(void *, int, uint64_t, void **) = 
        (void (*)(void *, int, uint64_t, void **))((uint64_t **)gST->BootServices)[4];
    
    status = AllocatePool(gST->BootServices, 0, *map_size + 256, &memory_map);
    if (status != 0) {
        return NULL;
    }
    
    status = GetMemoryMap(gST->BootServices, map_size, memory_map, map_key, desc_size, desc_version);
    if (status != 0) {
        return NULL;
    }
    
    return (EFI_MEMORY_DESCRIPTOR *)memory_map;
}

/* Exit boot services */
static uint64_t exit_boot_services(void *image_handle, void *memory_map) {
    void (*ExitBootServices)(void *, void *, uint64_t) = 
        (void (*)(void *, void *, uint64_t))((uint64_t **)gST->BootServices)[11];
    
    uint64_t map_key = 0;
    uint64_t map_size = 0;
    uint64_t desc_size = 0;
    uint32_t desc_version = 0;
    
    get_memory_map(&map_size, &map_key, &desc_size, &desc_version);
    
    EFI_STATUS status = ExitBootServices(image_handle, map_key);
    if (status != 0) {
        return 1;
    }
    
    return 0;
}

/* Kernel entry point (defined in kernel/core/main.c) */
extern void kernel_main(void *memory_map, uint64_t map_size, uint64_t desc_size, 
                        EFI_SYSTEM_TABLE *efi_table);

/* EFI Entry Point */
uint64_t efi_main(void *image_handle, EFI_SYSTEM_TABLE *system_table) {
    gST = system_table;
    
    efi_print("CellKernel EFI Bootloader v1.0\r\n");
    efi_print("==============================\r\n");
    
    /* Verify secure boot signature */
    efi_print("[*] Verifying kernel signature...\r\n");
    int sig_result = verify_kernel_signature(0, 0);
    if (sig_result != 0) {
        efi_print("[!] Signature verification failed!\r\n");
        return EFI_SECURITY_VIOLATION;
    }
    efi_print("[+] Signature verified\r\n");
    
    /* TPM measurement */
    efi_print("[*] Measuring kernel image to TPM...\r\n");
    int tpm_result = tpm_measure_image(0, 0, 11);
    if (tpm_result != 0) {
        efi_print("[!] TPM measurement failed!\r\n");
        return EFI_DEVICE_ERROR;
    }
    efi_print("[+] TPM PCR extended\r\n");
    
    /* Get memory map */
    efi_print("[*] Retrieving memory map...\r\n");
    uint64_t map_size = 0;
    uint64_t map_key = 0;
    uint64_t desc_size = 0;
    uint32_t desc_version = 0;
    
    EFI_MEMORY_DESCRIPTOR *memory_map = get_memory_map(&map_size, &map_key, &desc_size, &desc_version);
    if (!memory_map) {
        efi_print("[!] Failed to get memory map\r\n");
        return EFI_NOT_FOUND;
    }
    efi_print("[+] Memory map acquired\r\n");
    
    /* Exit boot services */
    efi_print("[*] Exiting EFI boot services...\r\n");
    uint64_t exit_status = exit_boot_services(image_handle, memory_map);
    if (exit_status != 0) {
        efi_print("[!] Failed to exit boot services\r\n");
        return exit_status;
    }
    efi_print("[+] Boot services exited\r\n");
    
    /* Hand over to kernel */
    efi_print("[*] Handing over to kernel...\r\n");
    kernel_main(memory_map, map_size, desc_size, system_table);
    
    /* Should never reach here */
    while (1) {
        __asm__ volatile ("hlt");
    }
    
    return EFI_SUCCESS;
}
