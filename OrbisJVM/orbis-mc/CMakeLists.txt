# OrbisMC - PS4 Minecraft Server Management Application
# CMake build configuration

cmake_minimum_required(VERSION 3.16)

project(OrbisMC
    VERSION 0.1.0
    LANGUAGES C CXX
    DESCRIPTION "PS4 Minecraft Server Management Application"
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OrbisJVM (parent project)
# OrbisJVM build artifacts are at ../../build from orbis-mc/build
find_library(ORBISJVM_LIB orbisjvm
    PATHS
        ${CMAKE_SOURCE_DIR}/../../build
        ${CMAKE_SOURCE_DIR}/../build
        ${CMAKE_BINARY_DIR}/../../build
        ${CMAKE_BINARY_DIR}/../build
        ${CMAKE_SOURCE_DIR}/build
        ${CMAKE_BINARY_DIR}
        $ENV{OO_PS4_TOOLCHAIN}/lib
)
find_path(ORBISJVM_INCLUDE orbisjvm/runtime.h
    PATHS
        ${CMAKE_SOURCE_DIR}/../../include
        ${CMAKE_SOURCE_DIR}/../include
        ${CMAKE_BINARY_DIR}/../../include
        ${CMAKE_SOURCE_DIR}/include
        $ENV{OO_PS4_TOOLCHAIN}/include
)

if(ORBISJVM_LIB AND ORBISJVM_INCLUDE)
    add_library(orbisjvm INTERFACE IMPORTED)
    set_target_properties(orbisjvm PROPERTIES
        IMPORTED_LOCATION ${ORBISJVM_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${ORBISJVM_INCLUDE}
    )
    message(STATUS "Found OrbisJVM: ${ORBISJVM_LIB}")
else()
    message(WARNING "OrbisJVM not found. Build OrbisJVM first.")
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/ui
    ${CMAKE_SOURCE_DIR}/launcher
)

if(ORBISJVM_INCLUDE)
    include_directories(${ORBISJVM_INCLUDE})
endif()

# App sources
set(APP_SOURCES
    app/main.c
    app/server_manager.c
    app/world_manager.c
    app/config_manager.c
    app/console.c
    app/player_manager.c
    app/remote_api.c
)

# UI sources
set(UI_SOURCES
    ui/ui.c
)

# Launcher sources
set(LAUNCHER_SOURCES
    launcher/launcher.c
)

# Main application executable
add_executable(orbis-mc ${APP_SOURCES} ${UI_SOURCES})
target_link_libraries(orbis-mc PRIVATE orbisjvm)

# Launcher executable
add_executable(orbis-mc-launcher ${LAUNCHER_SOURCES})
target_link_libraries(orbis-mc-launcher PRIVATE orbisjvm)

# Compiler options for PS4
target_compile_options(orbis-mc PRIVATE
    -Wall -Wextra -Wpedantic
    -DORBISMC_VERSION=\"${PROJECT_VERSION}\"
)

target_compile_options(orbis-mc-launcher PRIVATE
    -Wall -Wextra -Wpedantic
    -DORBISMC_VERSION=\"${PROJECT_VERSION}\"
)

# Installation
install(TARGETS orbis-mc orbis-mc-launcher
    RUNTIME DESTINATION bin
)

# Package configuration for OrbisMC
set(CPACK_PACKAGE_NAME "OrbisMC")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "PS4 Minecraft Server Management Application")
set(CPACK_PACKAGE_VENDOR "OrbisMC Project")
set(CPACK_GENERATOR "TGZ")
include(CPack)