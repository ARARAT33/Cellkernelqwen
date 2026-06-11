# CellKernel Qwen OS

A modern micro-logic architecture operating system with cross-platform support.

## Features

- **Micro-kernel Architecture**: Modular design with separate subsystems
- **Cross-Platform UI**: GTK3 (Linux), Cocoa (macOS), WinUI (Windows)
- **Virtual Filesystem**: Complete VFS implementation
- **Network Stack**: TCP/IP networking support
- **Audio Subsystem**: Sound playback and recording
- **Python Integration**: Full Python scripting support

## Project Structure

```
Cellkernelqwen/
├── src/
│   ├── kernel/          # Core kernel implementation
│   ├── ui/              # User interface framework
│   ├── filesystem/      # Virtual filesystem
│   ├── network/         # Network stack
│   └── audio/           # Audio subsystem
├── tests/               # Unit and integration tests
├── docs/                # Documentation
├── iso_root/            # ISO build files
└── .github/workflows/   # CI/CD pipelines
```

## Building

### Prerequisites

- CMake 3.16+
- Python 3.8+
- Platform-specific dependencies:
  - Linux: GTK3, libx11, libasound2
  - macOS: Xcode command line tools
  - Windows: Visual Studio Build Tools

### Build Instructions

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)

# Test
cd build && ctest

# Install
sudo cmake --install build
```

## Running

```bash
# Run kernel directly
./build/bin/cellkernelqwen-kernel

# Run with Python scripts
python3 src/kernel/cellkernel.py
```

## Testing

```bash
# Run all tests
python3 -m pytest tests/

# Run specific test module
python3 tests/test_all.py
```

## CI/CD

The project uses GitHub Actions for continuous integration:

- Automated builds on Ubuntu, macOS, and Windows
- Unit testing and integration testing
- Artifact generation for releases
- Automatic release packaging

## License

MIT License - See LICENSE file for details

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests
5. Submit a pull request

## Version

1.0.0
