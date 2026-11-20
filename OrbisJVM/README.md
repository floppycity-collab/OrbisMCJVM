# OrbisJVM

**Native Java Runtime Compatibility Layer for PlayStation 4**

OrbisJVM is a compatibility layer that enables running Java applications natively on jailbroken PlayStation 4 consoles using the OpenOrbis PS4 Toolchain. The ultimate goal is to run Minecraft Java Edition dedicated servers directly on PS4 hardware.

## Project Overview

- **Project Name**: OrbisJVM
- **Related Application**: OrbisMC (server management UI)
- **Target Platform**: PlayStation 4 (Orbis OS) on jailbroken consoles
- **Toolchain**: OpenOrbis PS4 Toolchain
- **Primary Languages**: C, C++, CMake, Shell

## Architecture

```
OrbisJVM/
├── include/orbisjvm/     # Public API headers
├── src/                  # Implementation
│   ├── runtime/          # Core runtime initialization
│   ├── memory/           # Memory management
│   ├── threading/        # Threading and synchronization
│   ├── filesystem/       # File and directory operations
│   ├── networking/       # TCP/UDP networking (critical for Minecraft)
│   ├── synchronization/  # Advanced synchronization primitives
│   ├── time/             # Time and timer functions
│   ├── environment/      # Environment variables and system info
│   ├── process/          # Process management
│   ├── security/         # Cryptographic operations
│   ├── jni/              # JNI compatibility layer
│   └── diagnostics/      # Logging, profiling, crash reporting
├── jvm/                  # JVM integration (OpenJDK/HotSpot)
├── tests/                # Unit tests
├── examples/             # Example applications
├── tools/                # Diagnostic and build tools
├── docs/                 # Documentation
└── orbis-mc/             # OrbisMC server management application
```

## Current Status

This is an early-stage project. The following components have been implemented:

- ✅ CMake build configuration for OpenOrbis toolchain
- ✅ Core runtime initialization and platform detection
- ✅ Memory management API (malloc, mmap, page allocation)
- ✅ Threading API (threads, mutexes, condition variables, TLS)
- ✅ Filesystem API (FIOS2-compatible file/directory operations)
- ✅ Networking API (TCP/UDP sockets, DNS resolution)
- ✅ Synchronization primitives (semaphores, RW locks, barriers, atomics)
- ✅ Time API (monotonic/wall clocks, high-resolution timers)
- ✅ Environment API (env vars, system info, process info)
- ✅ Process management API
- ✅ Security API (hashing, HMAC, AES, secure random)
- ✅ JNI compatibility layer (stub implementation)
- ✅ Diagnostics API (logging, profiling, crash reporting)
- ✅ Diagnostic command-line tool

## Requirements

- **OpenOrbis PS4 Toolchain** - See [BUILDING.md](docs/BUILDING.md)
- **clang + lld** - For PS4 target compilation
- **OpenSSL** - For cryptographic operations
- **CMake 3.16+** - Build system

## Building

```bash
# Set up OpenOrbis toolchain
export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis-PS4-Toolchain

# Configure and build
mkdir build && cd build
cmake ..
make

# Run diagnostic tool
./tools/orbisjvm-diagnose --all
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md) - Runtime architecture and module responsibilities
- [Building](docs/BUILDING.md) - Build instructions and toolchain setup
- [Compatibility](docs/COMPATIBILITY.md) - Supported/unsupported APIs and platform limitations
- [JVM Porting](docs/JVM_PORTING.md) - OpenJDK integration strategy
- [Minecraft Support](docs/MINECRAFT_SUPPORT.md) - Minecraft server compatibility

## Development Stages

1. **Stage 1**: Inspect development environment ✅
2. **Stage 2**: Create native compatibility layer ✅ (in progress)
3. **Stage 3**: Implement and test memory management
4. **Stage 4**: Implement and test threading
5. **Stage 5**: Implement filesystem compatibility
6. **Stage 6**: Implement TCP, UDP and DNS compatibility
7. **Stage 7**: Investigate and integrate OpenJDK
8. **Stage 8**: Run simple Java programs
9. **Stage 9**: Run Java networking and threading tests
10. **Stage 10**: Attempt Minecraft server execution
11. **Stage 11**: Develop OrbisMC application
12. **Stage 12**: Implement server management and monitoring
13. **Stage 13**: Optimize and test long-running hosting

## License

GPLv3 - See [LICENSE](LICENSE) for details.

## Credits

Built on the excellent work of the OpenOrbis team:
- OpenOrbis PS4 Toolchain
- musl libc for PS4
- libcxx for PS4
- SDL-PS4

## Contributing

This is a research project. Contributions welcome for:
- Platform API implementations
- JVM integration strategies
- Performance optimizations
- Testing on actual PS4 hardware

## Disclaimer

This project is for educational and research purposes. It requires a jailbroken PS4 console running a user-authorized homebrew environment. No firmware bypasses or exploit development is included.