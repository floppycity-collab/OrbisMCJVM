/**
 * @file environment.h
 * @brief OrbisJVM Environment API
 *
 * Provides environment variable handling, system information, and configuration.
 */

#ifndef ORBISJVM_ENVIRONMENT_H
#define ORBISJVM_ENVIRONMENT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get environment variable
 *
 * @param name Variable name
 * @return Variable value, or NULL if not set
 */
const char* orbisjvm_getenv(const char* name);

/**
 * @brief Set environment variable
 *
 * @param name Variable name
 * @param value Variable value
 * @param overwrite 1 to overwrite existing, 0 to keep existing
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_setenv(const char* name, const char* value, int overwrite);

/**
 * @brief Unset environment variable
 *
 * @param name Variable name
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_unsetenv(const char* name);

/**
 * @brief Get all environment variables
 *
 * @param[out] envp Array of "NAME=VALUE" strings (NULL-terminated)
 * @param max_entries Maximum entries to return
 * @param[out] count Actual number of entries
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_environment(char** envp, size_t max_entries, size_t* count);

/**
 * @brief Clear all environment variables
 *
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_clearenv(void);

/**
 * @brief System information structure
 */
typedef struct {
    char os_name[64];
    char os_version[64];
    char os_build[64];
    char kernel_version[64];
    char architecture[32];
    char hardware_model[64];
    char hardware_serial[64];
    uint64_t boot_time;        ///< Boot time (wall clock, nanoseconds since epoch)
    uint64_t uptime_ns;        ///< Current uptime in nanoseconds
    uint32_t cpu_count;        ///< Number of CPU cores
    uint64_t cpu_frequency_hz; ///< CPU frequency in Hz
    uint64_t total_memory;     ///< Total physical memory in bytes
    uint64_t available_memory; ///< Available physical memory in bytes
    uint32_t page_size;        ///< System page size
} orbisjvm_system_info_t;

/**
 * @brief Get system information
 *
 * @param[out] info System information structure
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_system_info(orbisjvm_system_info_t* info);

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
    uint32_t cache_line_size;
    uint32_t l1_cache_size;
    uint32_t l2_cache_size;
    uint32_t l3_cache_size;
} orbisjvm_cpu_info_t;

/**
 * @brief CPU feature flags
 */
#define ORBISJVM_CPU_FEATURE_FPU        (1 << 0)
#define ORBISJVM_CPU_FEATURE_SSE        (1 << 1)
#define ORBISJVM_CPU_FEATURE_SSE2       (1 << 2)
#define ORBISJVM_CPU_FEATURE_SSE3       (1 << 3)
#define ORBISJVM_CPU_FEATURE_SSSE3      (1 << 4)
#define ORBISJVM_CPU_FEATURE_SSE4_1     (1 << 5)
#define ORBISJVM_CPU_FEATURE_SSE4_2     (1 << 6)
#define ORBISJVM_CPU_FEATURE_AVX        (1 << 7)
#define ORBISJVM_CPU_FEATURE_AVX2       (1 << 8)
#define ORBISJVM_CPU_FEATURE_FMA        (1 << 9)
#define ORBISJVM_CPU_FEATURE_AES        (1 << 10)
#define ORBISJVM_CPU_FEATURE_RDRAND     (1 << 11)
#define ORBISJVM_CPU_FEATURE_RDSEED     (1 << 12)
#define ORBISJVM_CPU_FEATURE_SHA        (1 << 13)
#define ORBISJVM_CPU_FEATURE_BMI1       (1 << 14)
#define ORBISJVM_CPU_FEATURE_BMI2       (1 << 15)

/**
 * @brief Get CPU information
 *
 * @param[out] info CPU information structure
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_cpu_info(orbisjvm_cpu_info_t* info);

/**
 * @brief Process information structure
 */
typedef struct {
    uint32_t pid;
    uint32_t ppid;
    uint32_t uid;
    uint32_t gid;
    char name[64];
    char exe_path[256];
    char cwd[256];
    uint64_t start_time;       ///< Process start time (nanoseconds since epoch)
    uint64_t cpu_time_ns;      ///< Total CPU time used
    uint64_t memory_rss;       ///< Resident set size
    uint64_t memory_vsz;       ///< Virtual memory size
    int num_threads;
    int priority;
} orbisjvm_process_info_t;

/**
 * @brief Get current process information
 *
 * @param[out] info Process information structure
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_process_info(orbisjvm_process_info_t* info);

/**
 * @brief Get process ID
 *
 * @return Current process ID
 */
uint32_t orbisjvm_getpid(void);

/**
 * @brief Get parent process ID
 *
 * @return Parent process ID
 */
uint32_t orbisjvm_getppid(void);

/**
 * @brief Get user ID
 *
 * @return Current user ID
 */
uint32_t orbisjvm_getuid(void);

/**
 * @brief Get group ID
 *
 * @return Current group ID
 */
uint32_t orbisjvm_getgid(void);

/**
 * @brief Application paths structure
 */
typedef struct {
    char exe_path[256];       ///< Executable path
    char exe_dir[256];        ///< Executable directory
    char data_dir[256];       ///< Application data directory
    char config_dir[256];     ///< Configuration directory
    char cache_dir[256];      ///< Cache directory
    char temp_dir[256];       ///< Temporary directory
    char home_dir[256];       ///< Home directory
} orbisjvm_app_paths_t;

/**
 * @brief Get application paths
 *
 * @param[out] paths Application paths structure
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_app_paths(orbisjvm_app_paths_t* paths);

/**
 * @brief Runtime configuration structure
 */
typedef struct {
    int log_level;              ///< Log level (0=none, 1=error, 2=warn, 3=info, 4=debug)
    int enable_console_log;     ///< Log to console
    int enable_file_log;        ///< Log to file
    char log_file[256];         ///< Log file path
    size_t max_log_size;        ///< Maximum log file size
    int max_log_files;          ///< Maximum rotated log files
    size_t default_stack_size;  ///< Default thread stack size
    size_t max_heap_size;       ///< Maximum Java heap size
    size_t initial_heap_size;   ///< Initial Java heap size
    int enable_jit;             ///< Enable JIT compilation
    int enable_gc_verbose;      ///< Verbose GC logging
    int enable_class_load_log;  ///< Log class loading
    int network_timeout_ms;     ///< Default network timeout
    int dns_cache_ttl;          ///< DNS cache TTL in seconds
} orbisjvm_runtime_config_t;

/**
 * @brief Get default runtime configuration
 *
 * @param[out] config Configuration structure
 */
void orbisjvm_get_default_runtime_config(orbisjvm_runtime_config_t* config);

/**
 * @brief Load runtime configuration from file
 *
 * @param path Configuration file path
 * @param[out] config Configuration structure
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_load_runtime_config(const char* path, orbisjvm_runtime_config_t* config);

/**
 * @brief Save runtime configuration to file
 *
 * @param path Configuration file path
 * @param config Configuration structure
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_save_runtime_config(const char* path, const orbisjvm_runtime_config_t* config);

/**
 * @brief Apply runtime configuration
 *
 * @param config Configuration to apply
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_apply_runtime_config(const orbisjvm_runtime_config_t* config);

/**
 * @brief Get current runtime configuration
 *
 * @param[out] config Configuration structure
 */
void orbisjvm_get_runtime_config(orbisjvm_runtime_config_t* config);

/**
 * @brief Random number generation
 *
 * @param[out] buffer Output buffer
 * @param size Buffer size
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_getrandom(void* buffer, size_t size);

/**
 * @brief Generate UUID
 *
 * @param[out] uuid Output buffer (37 bytes for UUID string + null)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_generate_uuid(char* uuid);

/**
 * @brief Get command line arguments
 *
 * @param[out] argc Argument count
 * @param[out] argv Argument vector (array of strings, NULL-terminated)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_get_args(int* argc, char*** argv);

/**
 * @brief Set command line arguments
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_set_args(int argc, char** argv);

#ifdef __cplusplus
}
#endif

#endif // ORBISJVM_ENVIRONMENT_H