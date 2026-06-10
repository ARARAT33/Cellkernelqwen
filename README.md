# CellKernel - True Microkernel Operating System

![CellKernel](https://img.shields.io/badge/CellKernel-v1.0.0-blue)
![License](https://img.shields.io/badge/License-MIT%2BGPLv3-green)
![Architecture](https://img.shields.io/badge/Arch-x86_64%7Carm64%7Criscv64%7Cloongarch64-orange)

## Overview

CellKernel is a true microkernel operating system designed from the ground up with:

- **Zero-Trust Security Model** - Every component must prove its trustworthiness
- **Capability-Based Access Control** - Fine-grained security through capabilities
- **Multi-Architecture Support** - x86_64, ARM64, RISC-V 64, LoongArch 64
- **Formal Verification Ready** - Designed for mathematical proof of correctness
- **Post-Quantum Cryptography** - Ready for the quantum computing era

## Features

### Security
- Secure Boot with Ed25519 signature verification
- TPM 2.0 integration for measured boot
- IOMMU enforcement for DMA protection
- Kernel address space layout randomization (KASLR)
- Stack canaries and guard pages
- Capability-based isolation

### Architecture
- True microkernel design (< 10K LOC core)
- User-space drivers with IOMMU isolation
- Lock-free IPC with message passing
- O(1) scheduler with real-time support
- SMP support for multi-core systems

### Memory Management
- 4-level and 5-level paging
- Physical frame allocator (buddy system)
- SLAB/SLUB kernel allocator
- Copy-on-write support
- Memory encryption (SME/TSME)

## Building

### Prerequisites

```bash
# Install cross-compiler toolchain
# For x86_64:
sudo apt install gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu

# Or build from source:
git clone https://github.com/crossboot/crossgcc.git
cd crossgcc && ./build-all --arch x86_64
```

### Build Commands

```bash
# Build for default architecture (x86_64)
make

# Build for specific architecture
make ARCH=arm64
make ARCH=riscv64
make ARCH=loongarch64

# Clean build
make clean && make

# Run in QEMU
make run

# Debug with GDB
make debug
```

## Directory Structure

```
CellKernel/
├── boot/                    # Bootloaders (EFI, BIOS, Secure Boot)
│   ├── efi/                 # EFI boot implementation
│   ├── bios/                # Legacy BIOS boot
│   └── secure/              # Secure boot & TPM
├── kernel/                  # Microkernel core
│   ├── core/                # Main kernel logic
│   ├── memory/              # Memory management
│   ├── ipc/                 # Inter-process communication
│   ├── capabilities/        # Capability system
│   ├── cells/               # Cell isolation
│   └── hal/                 # Hardware abstraction
├── security/                # Security subsystems
│   ├── lsm/                 # Linux Security Modules style
│   ├── crypto/              # Cryptographic primitives
│   └── tpm/                 # TPM driver
├── drivers/                 # Device drivers
├── filesystems/             # Filesystem implementations
├── networking/              # Network stack
└── ui/                      # User interface
```

## License

CellKernel is dual-licensed under:
- MIT License (for user-space components)
- GPLv3 (for kernel components)

See LICENSE file for details.

## Contributing

Contributions are welcome! Please read CONTRIBUTING.md before submitting patches.

## Documentation

- [Architecture Overview](docs/architecture.md)
- [Security Model](docs/security.md)
- [IPC Protocol](docs/ipc.md)
- [Driver Development](docs/drivers.md)
- [Building Guide](docs/building.md)

## Acknowledgments

Inspired by:
- seL4 microkernel
- Minix 3
- Fuchsia OS
- Redox OS
- L4 microkernel family
