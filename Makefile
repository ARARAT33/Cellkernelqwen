# CellKernel Build System
# Copyright (c) 2024 CellKernel Project
# SPDX-License-Identifier: MIT + GPLv3 hybrid

# Architecture selection (x86_64, arm64, riscv64, loongarch64)
ARCH ?= x86_64

# Cross-compiler toolchain (use system compiler if cross-compiler not available)
ifeq ($(ARCH), x86_64)
    CROSS_COMPILE ?= x86_64-linux-gnu-
    ARCH_CFLAGS := -march=x86-64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2
    ARCH_LDFLAGS := -m elf_x86_64
    BOOT_ARCH := bios/x86_64
else ifeq ($(ARCH), arm64)
    CROSS_COMPILE ?= aarch64-linux-gnu-
    ARCH_CFLAGS := -march=armv8-a -mgeneral-regs-only
    ARCH_LDFLAGS := -m aarch64elf
    BOOT_ARCH := bios/arm64
else ifeq ($(ARCH), riscv64)
    CROSS_COMPILE ?= riscv64-linux-gnu-
    ARCH_CFLAGS := -march=rv64imafdc -mabi=lp64d
    ARCH_LDFLAGS := -m elf64lriscv
    BOOT_ARCH := bios/riscv64
else ifeq ($(ARCH), loongarch64)
    CROSS_COMPILE ?= loongarch64-linux-gnu-
    ARCH_CFLAGS := -march=loongarch64
    ARCH_LDFLAGS := -m elf64loongarch
    BOOT_ARCH := bios/loongarch64
endif

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
AS := $(CROSS_COMPILE)as
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump
STRIP := $(CROSS_COMPILE)strip

# Build directories
BUILD_DIR := build
KERNEL_DIR := kernel
BOOT_DIR := boot
SECURITY_DIR := security
DRIVERS_DIR := drivers

# Output files
KERNEL_BIN := $(BUILD_DIR)/cellkernel.bin
KERNEL_ELF := $(BUILD_DIR)/cellkernel.elf
KERNEL_ISO := $(BUILD_DIR)/cellkernel.iso
EFI_BIN := $(BUILD_DIR)/boot.efi

# C flags
CFLAGS := -std=c11 \
          -ffreestanding \
          -fno-stack-protector \
          -fno-builtin \
          -fno-pie \
          -Wall \
          -Wextra \
          -Werror \
          -Wno-unused-parameter \
          -O2 \
          -g \
          -I$(KERNEL_DIR)/include \
          $(ARCH_CFLAGS)

# Assembly flags
ASFLAGS := $(ARCH_CFLAGS)

# Linker flags
LDFLAGS := -T $(KERNEL_DIR)/linker.ld \
           $(ARCH_LDFLAGS) \
           -nostdlib \
           -static \
           -z max-page-size=0x1000 \
           -Map=$(BUILD_DIR)/kernel.map

# Source files - dynamically find all existing .c files
BOOT_SRCS := $(wildcard boot/secure/*.c)
KERNEL_SRCS := $(wildcard kernel/core/*.c) \
               $(wildcard kernel/memory/*.c) \
               $(wildcard kernel/ipc/*.c) \
               $(wildcard kernel/capabilities/*.c) \
               $(wildcard kernel/cells/*.c) \
               $(wildcard kernel/hal/$(ARCH)/*.c) \
               $(wildcard kernel/hal/generic/*.c) \
               $(wildcard filesystems/vfs/*.c)

# Object files
BOOT_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(BOOT_SRCS))
KERNEL_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_SRCS))
ALL_OBJS := $(BOOT_OBJS) $(KERNEL_OBJS)

# Default target
.PHONY: all
all: dirs $(KERNEL_BIN)

# Build target (alias for all)
.PHONY: build
build: all

# Create build directories
.PHONY: dirs
dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/boot/secure
	@mkdir -p $(BUILD_DIR)/kernel/core
	@mkdir -p $(BUILD_DIR)/kernel/memory
	@mkdir -p $(BUILD_DIR)/kernel/ipc
	@mkdir -p $(BUILD_DIR)/kernel/capabilities
	@mkdir -p $(BUILD_DIR)/kernel/cells
	@mkdir -p $(BUILD_DIR)/kernel/hal/$(ARCH)
	@mkdir -p $(BUILD_DIR)/kernel/hal/generic
	@mkdir -p $(BUILD_DIR)/filesystems/vfs

# Link kernel
$(KERNEL_ELF): $(ALL_OBJS) $(KERNEL_DIR)/linker.ld
	@echo "[LD] Linking kernel..."
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)

# Create binary
$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "[OBJCOPY] Creating binary..."
	$(OBJCOPY) -O binary $< $@
	@echo "[+] Kernel built: $(KERNEL_BIN)"

# Create ISO (for BIOS boot)
$(KERNEL_ISO): $(KERNEL_BIN)
	@echo "[ISO] Creating bootable ISO..."
	mkdir -p $(BUILD_DIR)/isodir/boot/grub
	cp $(KERNEL_BIN) $(BUILD_DIR)/isodir/boot/cellkernel.bin
	echo "set timeout=0" > $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	echo "set default=0" >> $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	echo "menuentry \"CellKernel\" {" >> $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	echo "  multiboot /boot/cellkernel.bin" >> $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	echo "  boot" >> $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	echo "}" >> $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(KERNEL_ISO) $(BUILD_DIR)/isodir 2>/dev/null || echo "[!] grub-mkrescue not available"

# Compile C files
$(BUILD_DIR)/%.o: %.c
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build
.PHONY: clean
clean:
	@echo "[CLEAN] Removing build directory..."
	rm -rf $(BUILD_DIR)

# Disassemble kernel
.PHONY: disasm
disasm: $(KERNEL_ELF)
	$(OBJDUMP) -d $(KERNEL_ELF) > $(BUILD_DIR)/kernel.asm

# Generate symbols
.PHONY: symbols
symbols: $(KERNEL_ELF)
	$(OBJDUMP) -t $(KERNEL_ELF) > $(BUILD_DIR)/kernel.sym

# Run in QEMU
.PHONY: run
run: $(KERNEL_BIN)
	qemu-system-x86_64 -kernel $(KERNEL_BIN) -serial stdio -m 512M

# Run with GDB
.PHONY: debug
debug: $(KERNEL_BIN)
	qemu-system-x86_64 -kernel $(KERNEL_BIN) -serial stdio -m 512M -s -S

# Install to EFI
.PHONY: install-efi
install-efi: $(EFI_BIN)
	@echo "[INSTALL] Installing to EFI partition..."
	cp $(EFI_BIN) /mnt/esp/EFI/Boot/bootx64.efi

# Help
.PHONY: help
help:
	@echo "CellKernel Build System"
	@echo ""
	@echo "Usage: make [target] [ARCH=<arch>]"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build kernel (default)"
	@echo "  clean      - Remove build files"
	@echo "  disasm     - Disassemble kernel"
	@echo "  symbols    - Generate symbol table"
	@echo "  run        - Run in QEMU"
	@echo "  debug      - Run in QEMU with GDB"
	@echo "  install-efi - Install to EFI partition"
	@echo ""
	@echo "Architectures:"
	@echo "  x86_64     - AMD64/Intel 64 (default)"
	@echo "  arm64      - ARM 64-bit"
	@echo "  riscv64    - RISC-V 64-bit"
	@echo "  loongarch64 - LoongArch 64-bit"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build for x86_64"
	@echo "  make ARCH=arm64         # Build for ARM64"
	@echo "  make run                # Run in QEMU"
	@echo "  make clean all          # Clean rebuild"
