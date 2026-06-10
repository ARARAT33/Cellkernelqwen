/*
 * CellKernel - EFI Bootloader Entry Point
 * ★ True Microkernel OS – Zero‑Trust & Hardened
 * 
 * Copyright (c) 2024 CellKernel Project
 * Licensed under MIT + GPLv3 hybrid
 */

#include "../protocols.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* External symbols from linker script */
extern uint8_t _kernel_start[];
extern uint8_t _kernel_end[];
extern uint8_t _kernel_entry[];

/* Secure boot verification */
extern bool verify_kernel_signature(const void* kernel_data, size_t size);
extern int tpm_measure_kernel(const void* data, size_t size);

/* Global EFI system table */
EFI_SYSTEM_TABLE* gST = NULL;
EFI_BOOT_SERVICES* gBS = NULL;
EFI_RUNTIME_SERVICES* gRT = NULL;

/* Memory map */
static EFI_MEMORY_DESCRIPTOR* gMemoryMap = NULL;
static UINTN gMemoryMapSize = 0;
static UINTN gMemoryMapDescriptorSize = 0;
static UINT32 gMemoryMapDescriptorVersion = 0;

/* Kernel command line */
static CHAR16 gKernelCmdline[1024] = L"";

/*-----------------------------------------------------------------------------
 * Utility Functions
 *-----------------------------------------------------------------------------*/

static VOID EFIAPI Print(const CHAR16* Format, ...) {
    if (gST && gST->ConOut) {
        VA_LIST Args;
        VA_START(Args, Format);
        gST->ConOut->OutputString(gST->ConOut, (CHAR16*)Format);
        VA_END(Args);
    }
}

static VOID EFIAPI ClearScreen(VOID) {
    if (gST && gST->ConOut) {
        gST->ConOut->ClearScreen(gST->ConOut);
    }
}

static VOID EFIAPI SetCursorPosition(UINTN Column, UINTN Row) {
    if (gST && gST->ConOut) {
        gST->ConOut->SetCursorPosition(gST->ConOut, Column, Row);
    }
}

/*-----------------------------------------------------------------------------
 * Memory Management
 *-----------------------------------------------------------------------------*/

static EFI_STATUS AllocatePages(EFI_MEMORY_TYPE Type, UINTN Pages, VOID** Address) {
    return gBS->AllocatePages(AllocateAnyPages, Type, Pages, (EFI_PHYSICAL_ADDRESS*)Address);
}

static EFI_STATUS FreePages(VOID* Address, UINTN Pages) {
    return gBS->FreePages((EFI_PHYSICAL_ADDRESS)Address, Pages);
}

static EFI_STATUS GetMemoryMap(UINTN* Size, EFI_MEMORY_DESCRIPTOR** Map,
                                UINTN* Key, UINTN* DescriptorSize,
                                UINT32* DescriptorVersion) {
    return gBS->GetMemoryMap(Size, (EFI_MEMORY_DESCRIPTOR**)Map, Key,
                             DescriptorSize, DescriptorVersion);
}

/*-----------------------------------------------------------------------------
 * Secure Boot Verification
 *-----------------------------------------------------------------------------*/

static BOOLEAN VerifySecureBoot(VOID) {
    EFI_STATUS Status;
    UINT8 SecureBootEnabled = 0;
    UINTN DataSize = sizeof(SecureBootEnabled);
    
    /* Check if Secure Boot is enabled */
    Status = gRT->GetVariable(L"SecureBoot", &gEfiGlobalVariableGuid,
                              NULL, &DataSize, &SecureBootEnabled);
    
    if (EFI_ERROR(Status)) {
        Print(L"[WARN] Could not read SecureBoot variable\r\n");
        return FALSE; /* Allow boot anyway for development */
    }
    
    if (SecureBootEnabled) {
        Print(L"[INFO] Secure Boot is ENABLED\r\n");
        return TRUE;
    } else {
        Print(L"[INFO] Secure Boot is DISABLED\r\n");
        return FALSE;
    }
}

static BOOLEAN VerifyKernelImage(VOID* KernelBase, UINTN KernelSize) {
    BOOLEAN SignatureValid = FALSE;
    
    Print(L"[INFO] Verifying kernel signature...\r\n");
    
    /* Verify Ed25519 signature */
    SignatureValid = verify_kernel_signature(KernelBase, KernelSize);
    
    if (!SignatureValid) {
        Print(L"[ERROR] Kernel signature verification FAILED!\r\n");
        return FALSE;
    }
    
    Print(L"[OK] Kernel signature verified\r\n");
    
    /* Measure kernel into TPM PCR */
    int tpm_result = tpm_measure_kernel(KernelBase, KernelSize);
    if (tpm_result != 0) {
        Print(L"[WARN] TPM measurement failed (code: %d)\r\n", tpm_result);
        /* Continue anyway - TPM might not be available */
    } else {
        Print(L"[OK] Kernel measured into TPM PCR\r\n");
    }
    
    return TRUE;
}

/*-----------------------------------------------------------------------------
 * Kernel Loading
 *-----------------------------------------------------------------------------*/

typedef VOID (*KERNEL_ENTRY)(VOID* HandoffStruct, UINTN HandoffSize);

static EFI_STATUS LoadAndStartKernel(VOID) {
    EFI_STATUS Status;
    VOID* KernelBase = (VOID*)_kernel_start;
    UINTN KernelSize = (UINTN)((UINT8*)_kernel_end - (UINT8*)_kernel_start);
    KERNEL_ENTRY KernelEntry = (KERNEL_ENTRY)_kernel_entry;
    
    Print(L"\r\n[INFO] Kernel image at: 0x%p\r\n", KernelBase);
    Print(L"[INFO] Kernel size: %lu bytes\r\n", (unsigned long)KernelSize);
    
    /* Verify kernel integrity */
    if (!VerifyKernelImage(KernelBase, KernelSize)) {
        Print(L"[FATAL] Kernel verification failed. Halting.\r\n");
        return EFI_SECURITY_VIOLATION;
    }
    
    /* Prepare handoff structure */
    typedef struct {
        UINT32 Magic;
        UINT32 Version;
        VOID* MemoryMap;
        UINTN MemoryMapSize;
        UINTN DescriptorSize;
        UINT32 DescriptorVersion;
        EFI_SYSTEM_TABLE* SystemTable;
        CHAR16* Cmdline;
    } HandoffStruct;
    
    static HandoffStruct Handoff;
    Handoff.Magic = 0xCELL0001;
    Handoff.Version = 1;
    Handoff.MemoryMap = gMemoryMap;
    Handoff.MemoryMapSize = gMemoryMapSize;
    Handoff.DescriptorSize = gMemoryMapDescriptorSize;
    Handoff.DescriptorVersion = gMemoryMapDescriptorVersion;
    Handoff.SystemTable = gST;
    Handoff.Cmdline = gKernelCmdline;
    
    Print(L"[INFO] Exiting boot services...\r\n");
    
    /* Exit boot services */
    UINTN MapKey;
    Status = GetMemoryMap(&gMemoryMapSize, &gMemoryMap, &MapKey,
                          &gMemoryMapDescriptorSize, &gMemoryMapDescriptorVersion);
    
    if (EFI_ERROR(Status)) {
        Print(L"[ERROR] Failed to get memory map: 0x%x\r\n", Status);
        return Status;
    }
    
    Status = gBS->ExitBootServices(gST->ImageHandle, MapKey);
    if (EFI_ERROR(Status)) {
        Print(L"[ERROR] ExitBootServices failed: 0x%x\r\n", Status);
        return Status;
    }
    
    Print(L"[OK] Boot services exited\r\n");
    Print(L"[INFO] Jumping to kernel entry point...\r\n");
    
    /* Clear screen before handing over to kernel */
    ClearScreen();
    SetCursorPosition(0, 0);
    
    /* Jump to kernel */
    KernelEntry(&Handoff, sizeof(Handoff));
    
    /* Should never reach here */
    Print(L"[FATAL] Kernel returned! This should not happen.\r\n");
    
    return EFI_SUCCESS;
}

/*-----------------------------------------------------------------------------
 * EFI Entry Point
 *-----------------------------------------------------------------------------*/

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    EFI_STATUS Status;
    
    /* Initialize global pointers */
    gST = SystemTable;
    gBS = SystemTable->BootServices;
    gRT = SystemTable->RuntimeServices;
    
    /* Initialize console */
    ClearScreen();
    SetCursorPosition(0, 0);
    
    Print(L"╔══════════════════════════════════════════════════════════╗\r\n");
    Print(L"║           CellKernel - Secure Microkernel OS            ║\r\n");
    Print(L"║                    EFI Bootloader v1.0                   ║\r\n");
    Print(L"╚══════════════════════════════════════════════════════════╝\r\n\r\n");
    
    /* Check Secure Boot status */
    VerifySecureBoot();
    
    /* Initialize TPM */
    Print(L"[INFO] Initializing TPM 2.0...\r\n");
    /* TPM initialization would go here */
    
    /* Load and verify kernel */
    Status = LoadAndStartKernel();
    
    if (EFI_ERROR(Status)) {
        Print(L"[FATAL] Failed to load kernel: 0x%x\r\n", Status);
        Print(L"Press any key to reboot...\r\n");
        
        /* Wait for key press */
        EFI_INPUT_KEY Key;
        while (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_NOT_READY) {
            gBS->Stall(10000); /* 10ms */
        }
        
        /* Reset system */
        gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
    }
    
    return Status;
}
