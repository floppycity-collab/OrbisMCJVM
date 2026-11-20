# OrbisJVM - Native Java Runtime Compatibility Layer for PS4
# CMake build configuration for OpenOrbis PS4 Toolchain

cmake_minimum_required(VERSION 3.16)

project(OrbisJVM
    VERSION 0.1.0
    LANGUAGES C CXX
    DESCRIPTION "Native Java runtime compatibility layer for jailbroken PlayStation 4 consoles using OpenOrbis PS4 Toolchain"
)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# OpenOrbis Toolchain Configuration
# The toolchain requires clang and lld
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_LINKER lld)

# PS4 target architecture
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# OpenOrbis SDK path - set via environment variable OO_PS4_TOOLCHAIN
if(DEFINED ENV{OO_PS4_TOOLCHAIN})
    set(OO_PS4_TOOLCHAIN $ENV{OO_PS4_TOOLCHAIN})
    message(STATUS "OpenOrbis toolchain found at: ${OO_PS4_TOOLCHAIN}")
else()
    message(WARNING "OO_PS4_TOOLCHAIN environment variable not set. Please set it to your OpenOrbis installation directory.")
    message(WARNING "Example: export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis-PS4-Toolchain")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(OO_PS4_TOOLCHAIN)
    include_directories(${OO_PS4_TOOLCHAIN}/include)
    link_directories(${OO_PS4_TOOLCHAIN}/lib)
endif()

# Compiler flags for PS4 target
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target x86_64-scei-ps4 -fPIC -nostdlib -nostdinc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target x86_64-scei-ps4 -fPIC -nostdlib -nostdinc")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -target x86_64-scei-ps4 -fuse-ld=lld -Wl,--no-undefined")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -target x86_64-scei-ps4 -fuse-ld=lld -Wl,--no-undefined")

# Warning flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Werror=implicit-function-declaration")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Define OrbisJVM version
add_compile_definitions(
    ORBISJVM_VERSION_MAJOR=0
    ORBISJVM_VERSION_MINOR=1
    ORBISJVM_VERSION_PATCH=0
    ORBISJVM_VERSION_STRING="0.1.0"
)

# Platform-specific definitions
add_compile_definitions(
    __ORBIS__=1
    __PS4__=1
    _SCE_
)

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build the main OrbisJVM library
add_library(orbisjvm STATIC
    src/runtime/runtime.c
    src/memory/memory.c
    src/threading/threading.c
    src/filesystem/filesystem.c
    src/networking/networking.c
    src/synchronization/synchronization.c
    src/time/time.c
    src/environment/environment.c
    src/process/process.c
    src/security/security.c
    src/jni/jni.c
    src/diagnostics/diagnostics.c
)

target_include_directories(orbisjvm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link against OpenOrbis libraries
if(OO_PS4_TOOLCHAIN)
    target_link_libraries(orbisjvm PUBLIC
        orbis2
        orbisnet
        orbisfios2
        orbisrtc
        orbiskernel
        orbissysutil
        orbissysmodule
        muslc
        libcxx
    )
endif()

# Diagnostic tool executable
add_executable(orbisjvm-diagnose
    tools/diagnose.c
)

target_link_libraries(orbisjvm-diagnose PRIVATE orbisjvm)

# Tests
enable_testing()

# Add test subdirectories
add_subdirectory(tests/memory)
add_subdirectory(tests/threading)
add_subdirectory(tests/filesystem)
add_subdirectory(tests/networking)
add_subdirectory(tests/runtime)

# Installation
install(TARGETS orbisjvm
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(TARGETS orbisjvm-diagnose
    RUNTIME DESTINATION bin
)

# Package configuration
set(CPACK_PACKAGE_NAME "OrbisJVM")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Native Java runtime compatibility layer for PS4")
set(CPACK_PACKAGE_VENDOR "OrbisJVM Project")
set(CPACK_GENERATOR "TGZ")
include(CPack)
