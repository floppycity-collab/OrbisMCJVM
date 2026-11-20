#!/bin/bash

# OrbisJVM Build Script
# Usage: ./build.sh [clean|debug|release|test|package]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_toolchain() {
    log_info "Checking OpenOrbis toolchain..."

    if [ -z "$OO_PS4_TOOLCHAIN" ]; then
        log_error "OO_PS4_TOOLCHAIN environment variable not set!"
        log_error "Please set it to your OpenOrbis-PS4-Toolchain installation directory."
        log_error "Example: export OO_PS4_TOOLCHAIN=/opt/OpenOrbis-PS4-Toolchain"
        return 1
    fi

    if [ ! -d "$OO_PS4_TOOLCHAIN" ]; then
        log_error "OpenOrbis toolchain not found at: $OO_PS4_TOOLCHAIN"
        return 1
    fi

    if [ ! -f "$OO_PS4_TOOLCHAIN/bin/clang" ]; then
        log_warn "clang not found in toolchain bin, using system clang"
    fi

    log_info "Toolchain found at: $OO_PS4_TOOLCHAIN"
    return 0
}

build_project() {
    local build_type="${1:-Release}"
    log_info "Building OrbisJVM ($build_type)..."

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    cmake -DCMAKE_BUILD_TYPE="$build_type" "$PROJECT_ROOT"

    local jobs=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    make -j"$jobs"

    log_info "Build completed successfully!"
}

run_tests() {
    log_info "Running tests..."

    cd "$BUILD_DIR"
    ctest --output-on-failure -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

    log_info "All tests passed!"
}

clean_build() {
    log_info "Cleaning build directory..."

    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        log_info "Build directory cleaned."
    else
        log_info "Build directory does not exist."
    fi
}

package_project() {
    log_info "Creating package..."

    cd "$BUILD_DIR"
    cpack -G TGZ

    log_info "Package created in $BUILD_DIR"
}

print_usage() {
    echo "OrbisJVM Build Script"
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  clean     Clean build directory"
    echo "  debug     Build with Debug configuration"
    echo "  release   Build with Release configuration (default)"
    echo "  test      Run tests"
    echo "  package   Create distribution package"
    echo "  all       Clean, build release, run tests, and package"
    echo "  help      Show this help"
    echo ""
    echo "Environment Variables:"
    echo "  OO_PS4_TOOLCHAIN  Path to OpenOrbis PS4 Toolchain (required)"
}

main() {
    local command="${1:-release}"

    case "$command" in
        clean)
            clean_build
            ;;
        debug)
            check_toolchain && build_project "Debug"
            ;;
        release)
            check_toolchain && build_project "Release"
            ;;
        test)
            check_toolchain && run_tests
            ;;
        package)
            check_toolchain && package_project
            ;;
        all)
            check_toolchain && clean_build && build_project "Release" && run_tests && package_project
            ;;
        help|--help|-h)
            print_usage
            ;;
        *)
            log_error "Unknown command: $command"
            print_usage
            exit 1
            ;;
    esac
}

main "$@"