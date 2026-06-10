/*
 * CellKernel - EFI Protocols Header
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 */

#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#include <stdint.h>

/* EFI GUID Structure */
typedef struct {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} EFI_GUID;

/* EFI Status Type */
typedef int64_t EFI_STATUS;

/* EFI Handle */
typedef void *EFI_HANDLE;

/* EFI Event */
typedef void *EFI_EVENT;

/* EFI Physical Address */
typedef uint64_t EFI_PHYSICAL_ADDRESS;

/* EFI Virtual Address */
typedef uint64_t EFI_VIRTUAL_ADDRESS;

/* EFI LBA */
typedef uint64_t EFI_LBA;

/* EFI Boolean */
typedef uint8_t BOOLEAN;

/* EFI Char16 */
typedef uint16_t CHAR16;

/* EFI Time Structure */
typedef struct {
    uint16_t Year;
    uint8_t Month;
    uint8_t Day;
    uint8_t Hour;
    uint8_t Minute;
    uint8_t Second;
    uint8_t Pad1;
    uint32_t Nanosecond;
    int16_t TimeZone;
    uint8_t Daylight;
    uint8_t Pad2;
} EFI_TIME;

/* EFI Time Capabilities */
typedef struct {
    uint32_t Resolution;
    uint32_t Accuracy;
    BOOLEAN SetsToZero;
} EFI_TIME_CAPABILITIES;

/* EFI Input Key */
typedef struct {
    uint16_t ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

/* Simple Text Input Protocol */
typedef struct {
    EFI_STATUS (*Reset)(void *, BOOLEAN);
    EFI_STATUS (*ReadKeyStroke)(void *, EFI_INPUT_KEY *);
    EFI_EVENT WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

/* Simple Text Output Protocol */
typedef struct {
    EFI_STATUS (*Reset)(void *, BOOLEAN);
    EFI_STATUS (*OutputString)(void *, const CHAR16 *);
    EFI_STATUS (*TestString)(void *, const CHAR16 *);
    EFI_STATUS (*QueryMode)(void *, uint64_t, uint64_t *, uint64_t *);
    EFI_STATUS (*SetMode)(void *, uint64_t);
    EFI_STATUS (*SetAttribute)(void *, uint64_t);
    EFI_STATUS (*ClearScreen)(void *);
    EFI_STATUS (*SetCursorPosition)(void *, uint64_t, uint64_t);
    EFI_STATUS (*EnableCursor)(void *, BOOLEAN);
    EFI_SIMPLE_TEXT_OUTPUT_MODE *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

/* Simple Text Output Mode */
typedef struct {
    int32_t MaxMode;
    int32_t Mode;
    int32_t Attribute;
    int32_t CursorColumn;
    int32_t CursorRow;
    BOOLEAN CursorVisible;
} EFI_SIMPLE_TEXT_OUTPUT_MODE;

/* Boot Services Table */
typedef struct {
    EFI_GUID Hdr;
    void *RaisePriority;
    void *RestorePriority;
    EFI_STATUS (*AllocatePages)(int, int, uint64_t, EFI_PHYSICAL_ADDRESS *);
    EFI_STATUS (*FreePages)(EFI_PHYSICAL_ADDRESS, uint64_t);
    EFI_STATUS (*GetMemoryMap)(uint64_t *, void *, uint64_t *, uint64_t *, uint32_t *);
    EFI_STATUS (*AllocatePool)(int, uint64_t, void **);
    EFI_STATUS (*FreePool)(void *);
    void *CreateEvent;
    void *SetTimer;
    void *WaitForEvent;
    void *SignalEvent;
    void *CloseEvent;
    void *CheckEvent;
    void *InstallProtocolInterface;
    void *ReinstallProtocolInterface;
    void *UninstallProtocolInterface;
    EFI_STATUS (*HandleProtocol)(EFI_HANDLE, EFI_GUID *, void **);
    void *Reserved;
    void *RegisterProtocolNotify;
    EFI_STATUS (*LocateHandle)(int, EFI_GUID *, void *, uint64_t *, EFI_HANDLE *);
    void *LocateDevicePath;
    void *InstallConfigurationTable;
    EFI_STATUS (*LoadImage)(BOOLEAN, EFI_HANDLE, void *, void *, uint64_t, EFI_HANDLE *);
    EFI_STATUS (*StartImage)(EFI_HANDLE, uint64_t *, CHAR16 **);
    EFI_STATUS (*Exit)(EFI_HANDLE, EFI_STATUS, uint64_t, CHAR16 *);
    EFI_STATUS (*UnloadImage)(EFI_HANDLE);
    EFI_STATUS (*ExitBootServices)(EFI_HANDLE, uint64_t);
    void *GetNextMonotonicCount;
    EFI_STATUS (*Stall)(uint64_t);
    EFI_STATUS (*SetWatchdogTimer)(uint64_t, uint64_t, uint64_t, CHAR16 *);
    void *ConnectController;
    void *DisconnectController;
    void *OpenProtocol;
    void *CloseProtocol;
    void *OpenProtocolInformation;
    void *ProtocolsPerHandle;
    void *LocateHandleBuffer;
    EFI_STATUS (*LocateProtocol)(EFI_GUID *, void *, void **);
    void *InstallMultipleProtocolInterfaces;
    void *UninstallMultipleProtocolInterfaces;
    EFI_STATUS (*CalculateCrc32)(void *, uint64_t, uint32_t *);
    void *CopyMem;
    void *SetMem;
    void *CreateEventEx;
} EFI_BOOT_SERVICES;

/* Runtime Services Table */
typedef struct {
    EFI_GUID Hdr;
    EFI_STATUS (*GetTime)(EFI_TIME *, EFI_TIME_CAPABILITIES *);
    EFI_STATUS (*SetTime)(EFI_TIME *);
    EFI_STATUS (*GetWakeupTime)(BOOLEAN *, BOOLEAN *, EFI_TIME *);
    EFI_STATUS (*SetWakeupTime)(BOOLEAN, EFI_TIME *);
    void *SetVirtualAddressMap;
    void *ConvertPointer;
    EFI_STATUS (*GetVariable)(CHAR16 *, EFI_GUID *, uint32_t *, uint64_t *, void *);
    void *GetNextVariableName;
    EFI_STATUS (*SetVariable)(CHAR16 *, EFI_GUID *, uint32_t, uint64_t, void *);
    void *GetNextHighMonotonicCount;
    EFI_STATUS (*Reset)(int, int, uint64_t, void *);
    void *UpdateCapsule;
    void *QueryCapsuleCapabilities;
    EFI_STATUS (*QueryVariableInfo)(uint32_t, uint64_t *, uint64_t *, uint64_t *);
} EFI_RUNTIME_SERVICES;

/* EFI System Table */
typedef struct {
    EFI_GUID Hdr;
    CHAR16 *FirmwareVendor;
    uint32_t FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    EFI_RUNTIME_SERVICES *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    uint64_t NumberOfTableEntries;
    void *ConfigurationTable;
} EFI_SYSTEM_TABLE;

/* Loaded Image Protocol GUID */
#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    {0x5B1B31A1, 0x9562, 0x11d2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}}

/* System Table GUID */
#define EFI_SYSTEM_TABLE_GUID \
    {0x54505459, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}

/* Device Path Protocol GUID */
#define EFI_DEVICE_PATH_PROTOCOL_GUID \
    {0x09576E91, 0x6D39, 0x11d2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}}

#endif /* PROTOCOLS_H */
