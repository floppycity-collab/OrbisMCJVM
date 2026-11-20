#!/bin/bash

# OrbisJVM Package Script
# Creates a distribution package for PS4 deployment

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
DIST_DIR="$PROJECT_ROOT/dist"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Get version from CMakeLists.txt
get_version() {
    grep -E 'project\(OrbisJVM.*VERSION' "$PROJECT_ROOT/CMakeLists.txt" | \
        sed -E 's/.*VERSION ([0-9.]+).*/\1/'
}

main() {
    local version=$(get_version)
    local package_name="OrbisJVM-${version}"
    local package_dir="$DIST_DIR/$package_name"

    log_info "Creating package: $package_name"

    # Clean and create dist directory
    rm -rf "$DIST_DIR"
    mkdir -p "$package_dir"/{include,lib,bin,docs,examples}

    # Copy headers
    log_info "Copying headers..."
    cp -r "$PROJECT_ROOT/include/orbisjvm" "$package_dir/include/"

    # Copy libraries (if built)
    if [ -d "$BUILD_DIR" ]; then
        log_info "Copying libraries..."
        find "$BUILD_DIR" -name "*.a" -o -name "*.so" -o -name "*.dylib" | \
            while read lib; do
                cp "$lib" "$package_dir/lib/"
            done

        log_info "Copying tools..."
        find "$BUILD_DIR" -name "orbisjvm-diagnose" -type f -executable | \
            while read tool; do
                cp "$tool" "$package_dir/bin/"
            done
    fi

    # Copy documentation
    log_info "Copying documentation..."
    cp -r "$PROJECT_ROOT/docs"/* "$package_dir/docs/" 2>/dev/null || true
    cp "$PROJECT_ROOT/README.md" "$package_dir/" 2>/dev/null || true
    cp "$PROJECT_ROOT/LICENSE" "$package_dir/" 2>/dev/null || true

    # Copy examples
    log_info "Copying examples..."
    cp -r "$PROJECT_ROOT/examples" "$package_dir/" 2>/dev/null || true

    # Create PKG info file
    cat > "$package_dir/ORBISJVM_INFO.txt" <<EOF
OrbisJVM Package Information
============================
Version: $version
Build Date: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
Platform: PlayStation 4 (Orbis OS)
Architecture: x86_64
Toolchain: OpenOrbis PS4 Toolchain

Contents:
- include/orbisjvm/     : Public API headers
- lib/                  : Static libraries (.a)
- bin/                  : Diagnostic tool
- docs/                 : Documentation
- examples/             : Example applications

Installation:
1. Copy include/ to your project's include path
2. Copy lib/ to your project's library path
3. Link against -lorbisjvm -lorbis2 -lorbisnet -lorbisfios2 -lorbisrtc -lmuslc -lcxx

Usage:
#include <orbisjvm/runtime.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/threading.h>
// ... other headers

int main() {
    orbisjvm_init();
    // ... use OrbisJVM APIs
    orbisjvm_shutdown();
    return 0;
}

Environment Variables Required:
- OO_PS4_TOOLCHAIN: Path to OpenOrbis PS4 Toolchain

Build with CMake:
cmake -DCMAKE_BUILD_TYPE=Release ..
make

Run Diagnostic Tool:
./bin/orbisjvm-diagnose --all
EOF

    # Create tarball
    log_info "Creating tarball..."
    cd "$DIST_DIR"
    tar -czf "${package_name}.tar.gz" "$package_name"

    # Create checksums
    sha256sum "${package_name}.tar.gz" > "${package_name}.tar.gz.sha256"
    md5sum "${package_name}.tar.gz" > "${package_name}.tar.gz.md5"

    log_info "Package created successfully!"
    log_info "Location: $DIST_DIR/${package_name}.tar.gz"
    log_info "SHA256: $(cat "${package_name}.tar.gz.sha256")"
    log_info "MD5: $(cat "${package_name}.tar.gz.md5")"
}

main "$@"