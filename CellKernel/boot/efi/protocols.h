/*
 * CellKernel - EFI Protocol Definitions
 * ★ True Microkernel OS – Zero‑Trust & Hardened
 * 
 * Copyright (c) 2024 CellKernel Project
 * Licensed under MIT + GPLv3 hybrid
 */

#ifndef _EFI_PROTOCOLS_H
#define _EFI_PROTOCOLS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*-----------------------------------------------------------------------------
 * Basic EFI Types
 *-----------------------------------------------------------------------------*/

#ifndef VOID
#define VOID void
#endif

typedef uint8_t BOOLEAN;
typedef int8_t INT8;
typedef uint8_t UINT8;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int64_t INT64;
typedef uint64_t UINT64;
typedef uintptr_t UINTN;
typedef intptr_t INTN;
typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;
typedef UINT32 EFI_STATUS;
typedef VOID* EFI_HANDLE;
typedef VOID* EFI_EVENT;

/*-----------------------------------------------------------------------------
 * EFI Constants
 *-----------------------------------------------------------------------------*/

#define TRUE    1
#define FALSE   0

#define EFIAPI __attribute__((ms_abi))

/* Status codes */
#define EFI_SUCCESS             0
#define EFI_LOAD_ERROR          (1 | (1UL << 31))
#define EFI_INVALID_PARAMETER   (2 | (1UL << 31))
#define EFI_UNSUPPORTED         (3 | (1UL << 31))
#define EFI_BAD_BUFFER_SIZE     (4 | (1UL << 31))
#define EFI_BUFFER_TOO_SMALL    (5 | (1UL << 31))
#define EFI_NOT_READY           (6 | (1UL << 31))
#define EFI_DEVICE_ERROR        (7 | (1UL << 31))
#define EFI_WRITE_PROTECTED     (8 | (1UL << 31))
#define EFI_OUT_OF_RESOURCES    (9 | (1UL << 31))
#define EFI_VOLUME_CORRUPTED    (10 | (1UL << 31))
#define EFI_VOLUME_FULL         (11 | (1UL << 31))
#define EFI_NO_MEDIA            (12 | (1UL << 31))
#define EFI_MEDIA_CHANGED       (13 | (1UL << 31))
#define EFI_NOT_FOUND           (14 | (1UL << 31))
#define EFI_ACCESS_DENIED       (15 | (1UL << 31))
#define EFI_NO_RESPONSE         (16 | (1UL << 31))
#define EFI_NO_MAPPING          (17 | (1UL << 31))
#define EFI_TIMEOUT             (18 | (1UL << 31))
#define EFI_NOT_STARTED         (19 | (1UL << 31))
#define EFI_ALREADY_STARTED     (20 | (1UL << 31))
#define EFI_ABORTED             (21 | (1UL << 31))
#define EFI_ICMP_ERROR          (22 | (1UL << 31))
#define EFI_TFTP_ERROR          (23 | (1UL << 31))
#define EFI_PROTOCOL_ERROR      (24 | (1UL << 31))
#define EFI_INCOMPATIBLE_VERSION (25 | (1UL << 31))
#define EFI_SECURITY_VIOLATION  (26 | (1UL << 31))
#define EFI_CRC_ERROR           (27 | (1UL << 31))
#define EFI_END_OF_MEDIA        (28 | (1UL << 31))
#define EFI_END_OF_FILE         (31 | (1UL << 31))
#define EFI_INVALID_LANGUAGE    (32 | (1UL << 31))
#define EFI_COMPROMISED_DATA    (33 | (1UL << 31))

#define EFI_ERROR(Status)       (((INTN)(Status)) < 0)

/*-----------------------------------------------------------------------------
 * EFI Memory Types
 *-----------------------------------------------------------------------------*/

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

/*-----------------------------------------------------------------------------
 * EFI Memory Descriptor
 *-----------------------------------------------------------------------------*/

typedef struct {
    UINT32 Type;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_VIRTUAL_ADDRESS VirtualStart;
    UINT64 NumberOfPages;
    UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;

/* Memory attributes */
#define EFI_MEMORY_UC           0x0000000000000001ULL
#define EFI_MEMORY_WC           0x0000000000000002ULL
#define EFI_MEMORY_WT           0x0000000000000004ULL
#define EFI_MEMORY_WB           0x0000000000000008ULL
#define EFI_MEMORY_UCE          0x0000000000000010ULL
#define EFI_MEMORY_WP           0x0000000000001000ULL
#define EFI_MEMORY_RP           0x0000000000002000ULL
#define EFI_MEMORY_XP           0x0000000000004000ULL
#define EFI_MEMORY_RUNTIME      0x0000000000008000ULL
#define EFI_MEMORY_RUNTIME_A    0x0000000000010000ULL
#define EFI_MEMORY_MORE_RELIABLE 0x0000000000020000ULL
#define EFI_MEMORY_RO           0x0000000000040000ULL

/*-----------------------------------------------------------------------------
 * Allocate Type
 *-----------------------------------------------------------------------------*/

typedef enum {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
} EFI_ALLOCATE_TYPE;

/*-----------------------------------------------------------------------------
 * Reset Type
 *-----------------------------------------------------------------------------*/

typedef enum {
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} EFI_RESET_TYPE;

/*-----------------------------------------------------------------------------
 * Console Input
 *-----------------------------------------------------------------------------*/

typedef struct {
    UINT16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

/* Scan codes */
#define SCAN_NULL       0x0000
#define SCAN_UP         0x0001
#define SCAN_DOWN       0x0002
#define SCAN_RIGHT      0x0003
#define SCAN_LEFT       0x0004
#define SCAN_HOME       0x0005
#define SCAN_END        0x0006
#define SCAN_INSERT     0x0007
#define SCAN_DELETE     0x0008
#define SCAN_PAGE_UP    0x0009
#define SCAN_PAGE_DOWN  0x000A
#define SCAN_F1         0x000B
#define SCAN_F2         0x000C
#define SCAN_F3         0x000D
#define SCAN_F4         0x000E
#define SCAN_F5         0x000F
#define SCAN_F6         0x0010
#define SCAN_F7         0x0011
#define SCAN_F8         0x0012
#define SCAN_F9         0x0013
#define SCAN_F10        0x0014
#define SCAN_F11        0x0015
#define SCAN_F12        0x0016

/*-----------------------------------------------------------------------------
 * Simple Text Output Protocol
 *-----------------------------------------------------------------------------*/

typedef struct _SIMPLE_TEXT_OUTPUT_MODE {
    INT32 MaxMode;
    INT32 Mode;
    INT32 Attribute;
    INT32 CursorColumn;
    INT32 CursorRow;
    BOOLEAN CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

typedef struct _SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_STATUS (EFIAPI *Reset)(struct _SIMPLE_TEXT_OUTPUT_PROTOCOL* This, BOOLEAN ExtendedVerification);
    EFI_STATUS (EFIAPI *OutputString)(struct _SIMPLE_TEXT_OUTPUT_PROTOCOL* This, const CHAR16* String);
    EFI_STATUS (EFIAPI *TestString)(struct _SIMPLE_TEXT_OUTPUT_PROTOCOL* This, const CHAR16* String);
    EFI_STATUS (EFIAPI *QueryMode)(struct _SIMPLE_TEXT_OUTPUT_PROTOCOL* This, UINTN ModeNumber, UINTN* Columns, UINTN* Rows);
    EFI_STATUS (EFIAPI *SetMode)(struct _SIMPLE_TEXT_OUTPUT_PROTOCOL* This, UINTN ModeNumber);
    EFI_STATUS (EFIAPI *SetAttribute)(struct _SIMPLE_TEXT_OUTPUT_PROTOCOL* This, UINTN Attribute);
    EFI_STATUS (EFIAPI *ClearScreen)(struct _SIMPLE_TEXT_OUTPUT_PROTOCOL* This);
    EFI_STATUS (EFIAPI *SetCursorPosition)(struct _SIMPLE_TEXT_OUTPUT_PROTOCOL* This, UINTN Column, UINTN Row);
    EFI_STATUS (EFIAPI *EnableCursor)(struct _SIMPLE_TEXT_OUTPUT_PROTOCOL* This, BOOLEAN Visible);
    SIMPLE_TEXT_OUTPUT_MODE* Mode;
} SIMPLE_TEXT_OUTPUT_PROTOCOL;

/*-----------------------------------------------------------------------------
 * Simple Text Input Protocol
 *-----------------------------------------------------------------------------*/

typedef struct _SIMPLE_INPUT_INTERFACE {
    EFI_STATUS (EFIAPI *Reset)(struct _SIMPLE_INPUT_INTERFACE* This, BOOLEAN ExtendedVerification);
    EFI_STATUS (EFIAPI *ReadKeyStroke)(struct _SIMPLE_INPUT_INTERFACE* This, EFI_INPUT_KEY* Key);
    EFI_EVENT WaitForKey;
} SIMPLE_INPUT_INTERFACE;

/*-----------------------------------------------------------------------------
 * Boot Services Table
 *-----------------------------------------------------------------------------*/

typedef struct {
    EFI_TABLE_HEADER Hdr;
    
    /* Task Priority Services */
    VOID* RaiseTPL;
    VOID* RestoreTPL;
    
    /* Memory Services */
    EFI_STATUS (EFIAPI *AllocatePages)(EFI_ALLOCATE_TYPE Type, EFI_MEMORY_TYPE MemoryType,
                                        UINTN Pages, EFI_PHYSICAL_ADDRESS* Address);
    EFI_STATUS (EFIAPI *FreePages)(EFI_PHYSICAL_ADDRESS Address, UINTN Pages);
    EFI_STATUS (EFIAPI *GetMemoryMap)(UINTN* MemoryMapSize, EFI_MEMORY_DESCRIPTOR* MemoryMap,
                                       UINTN* MapKey, UINTN* DescriptorSize,
                                       UINT32* DescriptorVersion);
    EFI_STATUS (EFIAPI *AllocatePool)(EFI_MEMORY_TYPE PoolType, UINTN Size, VOID** Buffer);
    EFI_STATUS (EFIAPI *FreePool)(VOID* Buffer);
    
    /* Event & Timer Services */
    VOID* CreateEvent;
    VOID* SetTimer;
    VOID* WaitForEvent;
    VOID* SignalEvent;
    VOID* CloseEvent;
    VOID* CheckEvent;
    
    /* Protocol Handler Services */
    VOID* InstallProtocolInterface;
    VOID* ReinstallProtocolInterface;
    VOID* UninstallProtocolInterface;
    VOID* HandleProtocol;
    VOID* Reserved;
    VOID* RegisterProtocolNotify;
    VOID* LocateHandle;
    VOID* LocateDevicePath;
    VOID* InstallConfigurationTable;
    
    /* Image Services */
    EFI_STATUS (EFIAPI *LoadImage)(BOOLEAN BootPolicy, EFI_HANDLE ParentImage,
                                    EFI_DEVICE_PATH_PROTOCOL* DevicePath,
                                    VOID* SourceBuffer, UINTN SourceSize,
                                    EFI_HANDLE* ImageHandle);
    EFI_STATUS (EFIAPI *StartImage)(EFI_HANDLE ImageHandle, UINTN* ExitDataSize,
                                     CHAR16** ExitData);
    EFI_STATUS (EFIAPI *Exit)(EFI_HANDLE ImageHandle, EFI_STATUS ExitStatus,
                               UINTN ExitDataSize, CHAR16* ExitData);
    EFI_STATUS (EFIAPI *UnloadImage)(EFI_HANDLE ImageHandle);
    EFI_STATUS (EFIAPI *ExitBootServices)(EFI_HANDLE ImageHandle, UINTN MapKey);
    
    /* Miscellaneous Services */
    VOID* GetNextMonotonicCount;
    VOID* Stall;
    EFI_STATUS (EFIAPI *SetWatchdogTimer)(UINTN Timeout, UINT64 WatchdogCode,
                                           UINTN DataSize, CHAR16* WatchdogData);
    
    /* DriverSupport Services */
    VOID* ConnectController;
    VOID* DisconnectController;
    
    /* Open and Close Protocol Services */
    VOID* OpenProtocol;
    VOID* CloseProtocol;
    VOID* OpenProtocolInformation;
    
    /* Library Services */
    VOID* ProtocolsPerHandle;
    VOID* LocateHandleBuffer;
    VOID* LocateProtocol;
    VOID* InstallMultipleProtocolInterfaces;
    VOID* UninstallMultipleProtocolInterfaces;
    
    /* CRC Services */
    VOID* CalculateCrc32;
    
    /* Miscellaneous Services */
    VOID* CopyMem;
    VOID* SetMem;
    VOID* CreateEventEx;
} EFI_BOOT_SERVICES;

/*-----------------------------------------------------------------------------
 * Runtime Services Table
 *-----------------------------------------------------------------------------*/

typedef struct {
    EFI_TABLE_HEADER Hdr;
    
    /* Time Services */
    VOID* GetTime;
    VOID* SetTime;
    VOID* GetWakeupTime;
    VOID* SetWakeupTime;
    
    /* Virtual Memory Services */
    VOID* SetVirtualAddressMap;
    VOID* ConvertPointer;
    
    /* Variable Services */
    EFI_STATUS (EFIAPI *GetVariable)(CHAR16* VariableName, EFI_GUID* VendorGuid,
                                      UINT32* Attributes, UINTN* DataSize, VOID* Data);
    VOID* GetNextVariableName;
    VOID* SetVariable;
    
    /* Miscellaneous Services */
    VOID* GetNextHighMonotonicCount;
    VOID* ResetSystem;
    VOID* UpdateCapsule;
    VOID* QueryCapsuleCapabilities;
    VOID* QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

/*-----------------------------------------------------------------------------
 * System Table
 *-----------------------------------------------------------------------------*/

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16* FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    SIMPLE_INPUT_INTERFACE* ConIn;
    EFI_HANDLE ConsoleOutHandle;
    SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut;
    EFI_HANDLE StandardErrorHandle;
    SIMPLE_TEXT_OUTPUT_PROTOCOL* StdErr;
    EFI_RUNTIME_SERVICES* RuntimeServices;
    EFI_BOOT_SERVICES* BootServices;
    UINTN NumberOfTableEntries;
    VOID* ConfigurationTable;
} EFI_SYSTEM_TABLE;

/*-----------------------------------------------------------------------------
 * GUID Definition
 *-----------------------------------------------------------------------------*/

typedef struct {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8 Data4[8];
} EFI_GUID;

/* Global variable GUID */
extern EFI_GUID gEfiGlobalVariableGuid;

/*-----------------------------------------------------------------------------
 * Device Path Protocol
 *-----------------------------------------------------------------------------*/

typedef struct {
    UINT8 Type;
    UINT8 SubType;
    UINT8 Length[2];
} EFI_DEVICE_PATH_PROTOCOL;

/*-----------------------------------------------------------------------------
 * Table Header
 *-----------------------------------------------------------------------------*/

typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

/* EFI System Table signature */
#define EFI_SYSTEM_TABLE_SIGNATURE  0x5453595320494249ULL
#define EFI_2_10_SYSTEM_TABLE_REVISION ((2 << 16) | (10))

/*-----------------------------------------------------------------------------
 * VA_LIST for Print
 *-----------------------------------------------------------------------------*/

typedef __builtin_va_list VA_LIST;
#define VA_START(Marker, Parameter)  __builtin_va_start(Marker, Parameter)
#define VA_END(Marker)               __builtin_va_end(Marker)
#define VA_ARG(Marker, TYPE)         __builtin_va_arg(Marker, TYPE)

/*-----------------------------------------------------------------------------
 * External Declarations
 *-----------------------------------------------------------------------------*/

extern EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable);

#endif /* _EFI_PROTOCOLS_H */
