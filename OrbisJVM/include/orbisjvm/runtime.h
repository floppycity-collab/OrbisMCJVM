/**
 * @file runtime.h
 * @brief OrbisJVM Runtime Initialization and Management
 *
 * Provides core runtime initialization, version information, and platform capability reporting.
 */

#ifndef ORBISJVM_RUNTIME_H
#define ORBISJVM_RUNTIME_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OrbisJVM version information structure
 */
typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    const char* version_string;
    const char* build_date;
    const char* build_commit;
} orbisjvm_version_info_t;

/**
 * @brief Platform capability flags
 */
typedef enum {
    ORBISJVM_CAP_MEMORY_BASIC        = (1 << 0),  ///< Basic malloc/free support
    ORBISJVM_CAP_MEMORY_PAGE_ALLOC   = (1 << 1),  ///< Page-aligned allocation
    ORBISJVM_CAP_MEMORY_PROTECT      = (1 << 2),  ///< Memory protection (mprotect equivalent)
    ORBISJVM_CAP_MEMORY_MAP          = (1 << 3),  ///< Memory mapping support
    ORBISJVM_CAP_THREADING_BASIC     = (1 << 4),  ///< Thread creation/join
    ORBISJVM_CAP_THREADING_MUTEX     = (1 << 5),  ///< Mutex support
    ORBISJVM_CAP_THREADING_CONDVAR   = (1 << 6),  ///< Condition variable support
    ORBISJVM_CAP_THREADING_TLS       = (1 << 7),  ///< Thread-local storage
    ORBISJVM_CAP_FILESYSTEM_BASIC    = (1 << 8),  ///< File open/read/write/close
    ORBISJVM_CAP_FILESYSTEM_DIR      = (1 << 9),  ///< Directory operations
    ORBISJVM_CAP_FILESYSTEM_META     = (1 << 10), ///< File metadata (stat)
    ORBISJVM_CAP_NETWORK_TCP         = (1 << 11), ///< TCP socket support
    ORBISJVM_CAP_NETWORK_UDP         = (1 << 12), ///< UDP socket support
    ORBISJVM_CAP_NETWORK_DNS         = (1 << 13), ///< DNS resolution
    ORBISJVM_CAP_TIME_MONOTONIC      = (1 << 14), ///< Monotonic clock
    ORBISJVM_CAP_TIME_WALLCLOCK      = (1 << 15), ///< Wall clock time
    ORBISJVM_CAP_TIME_HR_TIMER       = (1 << 16), ///< High-resolution timers
    ORBISJVM_CAP_ENV_VARS            = (1 << 17), ///< Environment variables
    ORBISJVM_CAP_PROCESS_INFO        = (1 << 18), ///< Process information
    ORBISJVM_CAP_JNI_BASIC           = (1 << 19), ///< Basic JNI support
    ORBISJVM_CAP_TLS                 = (1 << 20), ///< TLS/SSL support
} orbisjvm_capability_t;

/**
 * @brief Memory information structure
 */
typedef struct {
    uint64_t total_physical;
    uint64_t available_physical;
    uint64_t total_virtual;
    uint64_t used_virtual;
    uint32_t page_size;
} orbisjvm_memory_info_t;

/**
 * @brief CPU information structure
 */
typedef struct {
    uint32_t core_count;
    uint32_t thread_count;
    uint64_t frequency_hz;
    char vendor[64];
    char model[64];
    uint32_t features;  // Bitmask of CPU features
} orbisjvm_cpu_info_t;

/**
 * @brief Platform information structure
 */
typedef struct {
    char platform_name[64];
    char os_name[64];
    char os_version[64];
    char architecture[32];
    orbisjvm_memory_info_t memory;
    orbisjvm_cpu_info_t cpu;
    uint64_t capabilities;
} orbisjvm_platform_info_t;

/**
 * @brief Initialize the OrbisJVM runtime
 *
 * Must be called before any other OrbisJVM functions.
 *
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_init(void);

/**
 * @brief Shutdown the OrbisJVM runtime
 *
 * Releases all resources allocated by orbisjvm_init().
 */
void orbisjvm_shutdown(void);

/**
 * @brief Get OrbisJVM version information
 *
 * @param[out] info Pointer to version info structure to fill
 */
void orbisjvm_get_version(orbisjvm_version_info_t* info);

/**
 * @brief Get platform capabilities and information
 *
 * @param[out] info Pointer to platform info structure to fill
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_platform_info(orbisjvm_platform_info_t* info);

/**
 * @brief Check if a specific capability is available
 *
 * @param cap Capability to check
 * @return 1 if available, 0 if not available
 */
int orbisjvm_has_capability(orbisjvm_capability_t cap);

/**
 * @brief Print diagnostic information to stdout
 *
 * Outputs version, platform info, and capabilities.
 */
void orbisjvm_print_diagnostics(void);

/**
 * @brief Runtime configuration options
 */
typedef struct {
    int enable_diagnostics;      ///< Enable diagnostic logging
    int enable_memory_tracking;  ///< Track memory allocations
    int enable_thread_tracking;  ///< Track thread creation
    size_t max_heap_size;        ///< Maximum Java heap size (0 = default)
    size_t initial_heap_size;    ///< Initial Java heap size (0 = default)
    const char* temp_dir;        ///< Temporary directory path
    const char* config_dir;      ///< Configuration directory path
} orbisjvm_config_t;

/**
 * @brief Get default runtime configuration
 *
 * @param[out] config Pointer to config structure to fill with defaults
 */
void orbisjvm_get_default_config(orbisjvm_config_t* config);

/**
 * @brief Set runtime configuration
 *
 * Must be called before orbisjvm_init() to take effect.
 *
 * @param config Configuration to apply
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_set_config(const orbisjvm_config_t* config);

/**
 * @brief Get current runtime configuration
 *
 * @param[out] config Pointer to config structure to fill
 */
void orbisjvm_get_config(orbisjvm_config_t* config);

#ifdef __cplusplus
}
#endif

#endif // ORBISJVM_RUNTIME_H