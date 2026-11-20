/**
 * @file diagnostics.h
 * @brief OrbisJVM Diagnostics and Logging API
 *
 * Provides logging, profiling, debugging, and diagnostic utilities.
 */

#ifndef ORBISJVM_DIAGNOSTICS_H
#define ORBISJVM_DIAGNOSTICS_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log level
 */
typedef enum {
    ORBISJVM_LOG_NONE    = 0,
    ORBISJVM_LOG_ERROR   = 1,
    ORBISJVM_LOG_WARN    = 2,
    ORBISJVM_LOG_INFO    = 3,
    ORBISJVM_LOG_DEBUG   = 4,
    ORBISJVM_LOG_TRACE   = 5,
} orbisjvm_log_level_t;

/**
 * @brief Log category
 */
typedef enum {
    ORBISJVM_LOG_CAT_GENERAL    = (1 << 0),
    ORBISJVM_LOG_CAT_MEMORY     = (1 << 1),
    ORBISJVM_LOG_CAT_THREADING  = (1 << 2),
    ORBISJVM_LOG_CAT_FILESYSTEM = (1 << 3),
    ORBISJVM_LOG_CAT_NETWORK    = (1 << 4),
    ORBISJVM_LOG_CAT_JNI        = (1 << 5),
    ORBISJVM_LOG_CAT_JVM        = (1 << 6),
    ORBISJVM_LOG_CAT_GC         = (1 << 7),
    ORBISJVM_LOG_CAT_CLASSLOAD  = (1 << 8),
    ORBISJVM_LOG_CAT_SECURITY   = (1 << 9),
    ORBISJVM_LOG_CAT_ALL        = 0xFFFFFFFF,
} orbisjvm_log_category_t;

/**
 * @brief Log entry structure
 */
typedef struct {
    orbisjvm_log_level_t level;
    orbisjvm_log_category_t category;
    uint64_t timestamp_ns;
    uint32_t thread_id;
    const char* file;
    int line;
    const char* func;
    const char* message;
} orbisjvm_log_entry_t;

/**
 * @brief Log callback function
 *
 * @param entry Log entry
 * @param user_data User data
 */
typedef void (*orbisjvm_log_callback_t)(const orbisjvm_log_entry_t* entry, void* user_data);

/**
 * @brief Initialize diagnostics subsystem
 *
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_diag_init(void);

/**
 * @brief Shutdown diagnostics subsystem
 */
void orbisjvm_diag_shutdown(void);

/**
 * @brief Set global log level
 *
 * @param level Log level
 */
void orbisjvm_diag_set_log_level(orbisjvm_log_level_t level);

/**
 * @brief Get global log level
 *
 * @return Current log level
 */
orbisjvm_log_level_t orbisjvm_diag_get_log_level(void);

/**
 * @brief Set log categories
 *
 * @param categories Category bitmask
 */
void orbisjvm_diag_set_categories(uint32_t categories);

/**
 * @brief Get log categories
 *
 * @return Current category bitmask
 */
uint32_t orbisjvm_diag_get_categories(void);

/**
 * @brief Register log callback
 *
 * @param callback Callback function
 * @param user_data User data passed to callback
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_diag_register_callback(orbisjvm_log_callback_t callback, void* user_data);

/**
 * @brief Unregister log callback
 *
 * @param callback Callback function to remove
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_diag_unregister_callback(orbisjvm_log_callback_t callback);

/**
 * @brief Log a message
 *
 * @param level Log level
 * @param category Log category
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void orbisjvm_log(orbisjvm_log_level_t level, orbisjvm_log_category_t category,
                  const char* format, ...) __attribute__((format(printf, 3, 4)));

/**
 * @brief Log a message with file/line/function info
 *
 * @param level Log level
 * @param category Log category
 * @param file Source file
 * @param line Source line
 * @param func Source function
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void orbisjvm_log_ex(orbisjvm_log_level_t level, orbisjvm_log_category_t category,
                     const char* file, int line, const char* func,
                     const char* format, ...) __attribute__((format(printf, 6, 7)));

/* Convenience macros */
#define orbisjvm_log_error(cat, fmt, ...) \
    orbisjvm_log_ex(ORBISJVM_LOG_ERROR, cat, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define orbisjvm_log_warn(cat, fmt, ...) \
    orbisjvm_log_ex(ORBISJVM_LOG_WARN, cat, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define orbisjvm_log_info(cat, fmt, ...) \
    orbisjvm_log_ex(ORBISJVM_LOG_INFO, cat, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define orbisjvm_log_debug(cat, fmt, ...) \
    orbisjvm_log_ex(ORBISJVM_LOG_DEBUG, cat, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define orbisjvm_log_trace(cat, fmt, ...) \
    orbisjvm_log_ex(ORBISJVM_LOG_TRACE, cat, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

/**
 * @brief Assertion macro (disabled in release builds)
 */
#ifdef NDEBUG
#define orbisjvm_assert(cond, fmt, ...) ((void)0)
#else
#define orbisjvm_assert(cond, fmt, ...) \
    do { \
        if (!(cond)) { \
            orbisjvm_log_ex(ORBISJVM_LOG_ERROR, ORBISJVM_LOG_CAT_GENERAL, \
                            __FILE__, __LINE__, __func__, \
                            "Assertion failed: " #cond ": " fmt, ##__VA_ARGS__); \
            orbisjvm_abort(); \
        } \
    } while (0)
#endif

/**
 * @brief Profiling support
 */

/**
 * @brief Profiler event type
 */
typedef enum {
    ORBISJVM_PROFILE_EVENT_ENTER = 0,
    ORBISJVM_PROFILE_EVENT_EXIT  = 1,
    ORBISJVM_PROFILE_EVENT_INSTANT = 2,
} orbisjvm_profile_event_type_t;

/**
 * @brief Profiler event
 */
typedef struct {
    orbisjvm_profile_event_type_t type;
    const char* name;
    const char* category;
    uint64_t timestamp_ns;
    uint32_t thread_id;
    uint64_t duration_ns;  ///< For EXIT events
    void* user_data;
} orbisjvm_profile_event_t;

/**
 * @brief Profiler callback
 */
typedef void (*orbisjvm_profile_callback_t)(const orbisjvm_profile_event_t* event, void* user_data);

/**
 * @brief Start profiling
 *
 * @param callback Profiling callback
 * @param user_data User data
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_profile_start(orbisjvm_profile_callback_t callback, void* user_data);

/**
 * @brief Stop profiling
 *
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_profile_stop(void);

/**
 * @brief Record profiling event (enter scope)
 *
 * @param name Event name
 * @param category Event category
 * @param user_data User data
 */
void orbisjvm_profile_enter(const char* name, const char* category, void* user_data);

/**
 * @brief Record profiling event (exit scope)
 *
 * @param name Event name
 * @param category Event category
 * @param user_data User data
 */
void orbisjvm_profile_exit(const char* name, const char* category, void* user_data);

/**
 * @brief Record instant profiling event
 *
 * @param name Event name
 * @param category Event category
 * @param user_data User data
 */
void orbisjvm_profile_instant(const char* name, const char* category, void* user_data);

/**
 * @brief Scoped profiling helper (use with orbisjvm_profile_scope_t)
 */
typedef struct {
    const char* name;
    const char* category;
    void* user_data;
    uint64_t start_ns;
} orbisjvm_profile_scope_t;

/**
 * @brief Begin profiling scope
 *
 * @param[out] scope Scope structure
 * @param name Scope name
 * @param category Scope category
 * @param user_data User data
 */
static inline void orbisjvm_profile_scope_begin(orbisjvm_profile_scope_t* scope,
                                                 const char* name,
                                                 const char* category,
                                                 void* user_data) {
    scope->name = name;
    scope->category = category;
    scope->user_data = user_data;
    scope->start_ns = orbisjvm_get_monotonic_ns();
    orbisjvm_profile_enter(name, category, user_data);
}

/**
 * @brief End profiling scope
 *
 * @param scope Scope structure
 */
static inline void orbisjvm_profile_scope_end(orbisjvm_profile_scope_t* scope) {
    uint64_t end_ns = orbisjvm_get_monotonic_ns();
    orbisjvm_profile_event_t event = {
        .type = ORBISJVM_PROFILE_EVENT_EXIT,
        .name = scope->name,
        .category = scope->category,
        .timestamp_ns = end_ns,
        .thread_id = (uint32_t)(uintptr_t)orbisjvm_thread_self(),
        .duration_ns = end_ns - scope->start_ns,
        .user_data = scope->user_data,
    };
    /* The callback is called internally */
    orbisjvm_profile_exit(scope->name, scope->category, scope->user_data);
}

/**
 * @brief Crash reporting
 */

/**
 * @brief Crash info structure
 */
typedef struct {
    int signal;
    uint64_t timestamp_ns;
    uint32_t thread_id;
    void* fault_address;
    void* instruction_pointer;
    void* stack_pointer;
    char thread_name[64];
    char* stack_trace;
    size_t stack_trace_len;
    char* registers;
    size_t registers_len;
} orbisjvm_crash_info_t;

/**
 * @brief Crash callback
 */
typedef void (*orbisjvm_crash_callback_t)(const orbisjvm_crash_info_t* info, void* user_data);

/**
 * @brief Register crash handler
 *
 * @param callback Crash callback
 * @param user_data User data
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_crash_register_handler(orbisjvm_crash_callback_t callback, void* user_data);

/**
 * @brief Unregister crash handler
 *
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_crash_unregister_handler(void);

/**
 * @brief Simulate crash for testing
 *
 * @param signal Signal to simulate
 */
void orbisjvm_crash_simulate(int signal);

/**
 * @brief Memory diagnostics
 */

/**
 * @brief Memory usage snapshot
 */
typedef struct {
    uint64_t total_allocated;
    uint64_t current_allocated;
    uint64_t peak_allocated;
    uint64_t total_freed;
    uint64_t allocation_count;
    uint64_t free_count;
    uint64_t leak_count;
    size_t largest_allocation;
    size_t average_allocation;
} orbisjvm_memory_snapshot_t;

/**
 * @brief Take memory snapshot
 *
 * @param[out] snapshot Snapshot structure
 */
void orbisjvm_memory_snapshot(orbisjvm_memory_snapshot_t* snapshot);

/**
 * @brief Dump memory allocations to file
 *
 * @param path Output file path
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_memory_dump_allocations(const char* path);

/**
 * @brief Thread diagnostics
 */

/**
 * @brief Thread snapshot
 */
typedef struct {
    uint32_t thread_id;
    char name[64];
    int state;  /* 0=running, 1=sleeping, 2=waiting, 3=stopped */
    uint64_t cpu_time_ns;
    size_t stack_size;
    size_t stack_used;
    uint64_t allocations;
    uint64_t frees;
} orbisjvm_thread_snapshot_t;

/**
 * @brief Get thread snapshot
 *
 * @param thread Thread handle
 * @param[out] snapshot Snapshot structure
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_thread_snapshot(orbisjvm_thread_t thread, orbisjvm_thread_snapshot_t* snapshot);

/**
 * @brief Dump all threads to file
 *
 * @param path Output file path
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_dump_all_threads(const char* path);

/**
 * @brief System diagnostics
 */

/**
 * @brief Generate system diagnostic report
 *
 * @param path Output file path
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_generate_diagnostic_report(const char* path);

/**
 * @brief Print diagnostic report to stdout
 */
void orbisjvm_print_diagnostic_report(void);

/**
 * @brief Performance counters
 */

typedef enum {
    ORBISJVM_PERF_CPU_CYCLES     = 0,
    ORBISJVM_PERF_INSTRUCTIONS   = 1,
    ORBISJVM_PERF_CACHE_REFERENCES = 2,
    ORBISJVM_PERF_CACHE_MISSES   = 3,
    ORBISJVM_PERF_BRANCH_INSTRUCTIONS = 4,
    ORBISJVM_PERF_BRANCH_MISSES  = 5,
    ORBISJVM_PERF_BUS_CYCLES     = 6,
    ORBISJVM_PERF_STALLED_CYCLES_FRONTEND = 7,
    ORBISJVM_PERF_STALLED_CYCLES_BACKEND = 8,
} orbisjvm_perf_counter_t;

/**
 * @brief Start performance counter
 *
 * @param counter Counter type
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_perf_start(orbisjvm_perf_counter_t counter);

/**
 * @brief Stop performance counter
 *
 * @param counter Counter type
 * @param[out] value Counter value
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_perf_stop(orbisjvm_perf_counter_t counter, uint64_t* value);

/**
 * @brief Read performance counter
 *
 * @param counter Counter type
 * @param[out] value Counter value
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_perf_read(orbisjvm_perf_counter_t counter, uint64_t* value);

#ifdef __cplusplus
}
#endif

#endif // ORBISJVM_DIAGNOSTICS_H