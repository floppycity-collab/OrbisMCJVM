# Building OrbisJVM

## Prerequisites

### OpenOrbis PS4 Toolchain

The OpenOrbis toolchain is required to build for PS4. Follow these steps:

#### Option 1: Docker (Recommended)

```bash
# Pull the latest toolchain image
docker pull openorbisofficial/toolchain:latest

# Build using Docker
docker run --rm -w /workspace -v "$(pwd)":/workspace openorbisofficial/toolchain:latest make
```

#### Option 2: Native Installation

1. **Install clang and lld**:
   ```bash
   # Ubuntu/Debian
   sudo apt update
   sudo apt install clang lld

   # Arch
   sudo pacman -S clang lld

   # macOS (using Homebrew)
   brew install llvm
   ```

2. **Download OpenOrbis Toolchain**:
   ```bash
   # Download from releases
   wget https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/releases/latest/download/OpenOrbis-PS4-Toolchain-linux.tar.gz
   tar -xzf OpenOrbis-PS4-Toolchain-linux.tar.gz
   export OO_PS4_TOOLCHAIN=$(pwd)/OpenOrbis-PS4-Toolchain
   ```

3. **Set environment variable**:
   ```bash
   # Add to ~/.bashrc or ~/.zshrc
   export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis-PS4-Toolchain
   export PATH=$OO_PS4_TOOLCHAIN/bin:$PATH
   ```

### Additional Dependencies

- **CMake 3.16+**
- **OpenSSL development headers** (for security module)
- **Python 3** (for build scripts)

## Build Instructions

### Standard Build

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)

# Run tests
make test

# Install (optional)
make install
```

### Cross-Compilation for PS4

The CMakeLists.txt is configured for PS4 cross-compilation by default:

```bash
# Verify toolchain detection
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build with verbose output
make VERBOSE=1
```

### Build Configuration Options

```bash
# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Enable/disable features
cmake .. -DORBISJVM_ENABLE_JNI=ON
cmake .. -DORBISJVM_ENABLE_TESTS=ON
cmake .. -DORBISJVM_ENABLE_EXAMPLES=ON
```

### Using the Build Script

```bash
# Simple build
./tools/build.sh

# Clean build
./tools/build.sh clean

# Build with specific configuration
./tools/build.sh release
./tools/build.sh debug
```

## Toolchain Details

### Compiler Flags

The build uses these PS4-specific flags:
- `-target x86_64-scei-ps4` - PS4 target triple
- `-fPIC` - Position independent code
- `-nostdlib -nostdinc` - No standard library/headers
- `-fuse-ld=lld` - Use LLVM linker

### Linker Flags

- `-Wl,--no-undefined` - Fail on undefined symbols
- Links against OpenOrbis libraries:
  - `orbis2` - Core system library
  - `orbisnet` - Network API
  - `orbisfios2` - Filesystem I/O
  - `orbisrtc` - Real-time clock
  - `orbiskernel` - Kernel interfaces
  - `orbissysutil` - System utilities
  - `orbissysmodule` - System modules
  - `muslc` - musl libc for PS4
  - `libcxx` - C++ standard library

## Troubleshooting

### Toolchain Not Found

```
CMake Warning: OO_PS4_TOOLCHAIN environment variable not set.
```

Solution: Set the environment variable to your OpenOrbis installation path.

### Missing OpenOrbis Libraries

```
ld.lld: error: cannot find -lorbis2
```

Solution: Ensure OpenOrbis libraries are built and in `$OO_PS4_TOOLCHAIN/lib`.

### Compilation Errors

If you see errors about missing PS4 headers:
1. Verify `$OO_PS4_TOOLCHAIN/include` exists
2. Check that the toolchain version matches your CMakeLists.txt expectations

## Testing

### Host-Side Tests

Run tests on the build machine (Linux/macOS):

```bash
# Build and run tests
make test

# Run specific test
./tests/memory/test_memory
./tests/threading/test_threading
```

### PS4 Testing

For testing on actual PS4 hardware:

1. Build the PKG:
   ```bash
   # After building, create PKG
   ./tools/package.sh
   ```

2. Install on PS4 via:
   - Package installer
   - FTP transfer
   - USB storage

3. Run diagnostic tool:
   ```bash
   # On PS4
   ./orbisjvm-diagnose --all
   ```

## Packaging

Create a distribution package:

```bash
# Using CPack (configured in CMakeLists.txt)
make package

# Or use the package script
./tools/package.sh
```

This creates a `.tar.gz` archive with headers, libraries, and the diagnostic tool.

## Development Workflow

### Adding New Modules

1. Create header in `include/orbisjvm/`
2. Create implementation in `src/<module>/`
3. Add to `CMakeLists.txt` library sources
4. Add tests in `tests/<module>/`
5. Update documentation

### Code Style

- C11 standard
- C++17 standard
- Use `orbisjvm_` prefix for all public APIs
- Document with Doxygen-style comments
- Error handling: return negative errno on failure

### Debugging

Enable debug logging:

```bash
# Set log level at runtime
export ORBISJVM_LOG_LEVEL=4  # DEBUG
```

Or in code:
```c
orbisjvm_diag_set_log_level(ORBISJVM_LOG_DEBUG);
```

## CI/CD

GitHub Actions workflow (`.github/workflows/build.yml`):

```yaml
name: Build
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Setup OpenOrbis
        uses: OpenOrbis/toolchain-action@main
        with:
          version: latest
      - name: Build
        run: |
          mkdir build && cd build
          cmake ..
          make -j4
      - name: Test
        run: make test
```

## Common Issues

| Issue | Solution |
|-------|----------|
| `clang: not found` | Install clang package |
| `lld: not found` | Install lld package |
| `OO_PS4_TOOLCHAIN not set` | Export environment variable |
| `orbis2 not found` | Build OpenOrbis libraries first |
| `C++ headers not found` | Install libcxx from OpenOrbis |
| `pthread not found` | Link against muslc |

## Resources

- [OpenOrbis Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain)
- [OpenOrbis Wiki](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/wiki)
- [PS4 Homebrew Documentation](https://github.com/OpenOrbis)
- [musl for PS4](https://github.com/OpenOrbis/musl)