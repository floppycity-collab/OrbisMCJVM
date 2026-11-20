/**
 * @file diagnostics.c
 * @brief OrbisJVM Diagnostics Implementation
 *
 * Implements logging, profiling, crash reporting, and diagnostic utilities.
 */

#include <orbisjvm/diagnostics.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/threading.h>
#include <orbisjvm/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>

/* Global state */
static orbisjvm_log_level_t g_log_level = ORBISJVM_LOG_INFO;
static uint32_t g_log_categories = ORBISJVM_LOG_CAT_ALL;
static orbisjvm_log_callback_t g_log_callbacks[16] = {0};
static void* g_log_callback_data[16] = {0};
static int g_log_callback_count = 0;
static orbisjvm_mutex_t g_log_mutex;

static orbisjvm_profile_callback_t g_profile_callback = NULL;
static void* g_profile_callback_data = NULL;
static int g_profiling_active = 0;
static orbisjvm_mutex_t g_profile_mutex;

static orbisjvm_crash_callback_t g_crash_callback = NULL;
static void* g_crash_callback_data = NULL;
static orbisjvm_mutex_t g_crash_mutex;

/**
 * @brief Initialize diagnostics subsystem
 */
int orbisjvm_diag_init(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Initializing diagnostics subsystem");

    int ret = orbisjvm_mutex_create(&g_log_mutex, NULL);
    if (ret < 0) return ret;

    ret = orbisjvm_mutex_create(&g_profile_mutex, NULL);
    if (ret < 0) return ret;

    ret = orbisjvm_mutex_create(&g_crash_mutex, NULL);
    if (ret < 0) return ret;

    /* Install crash handler */
    signal(SIGSEGV, crash_signal_handler);
    signal(SIGABRT, crash_signal_handler);
    signal(SIGFPE, crash_signal_handler);
    signal(SIGILL, crash_signal_handler);
    signal(SIGBUS, crash_signal_handler);

    return 0;
}

/**
 * @brief Shutdown diagnostics subsystem
 */
void orbisjvm_diag_shutdown(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Shutting down diagnostics subsystem");

    orbisjvm_profile_stop();
    orbisjvm_crash_unregister_handler();

    orbisjvm_mutex_destroy(g_log_mutex);
    orbisjvm_mutex_destroy(g_profile_mutex);
    orbisjvm_mutex_destroy(g_crash_mutex);
}

/* Signal handler for crashes */
static void crash_signal_handler(int signum) {
    orbisjvm_crash_info_t info = {0};
    info.signal = signum;
    info.timestamp_ns = orbisjvm_get_monotonic_ns();
    info.thread_id = (uint32_t)(uintptr_t)orbisjvm_thread_self();

    /* Get stack trace */
    void* buffer[64];
    int nptrs = backtrace(buffer, 64);
    char** strings = backtrace_symbols(buffer, nptrs);

    size_t trace_len = 0;
    for (int i = 0; i < nptrs; i++) {
        trace_len += strlen(strings[i]) + 1;
    }

    info.stack_trace = orbisjvm_malloc(trace_len + 1, ORBISJVM_MEM_ZERO);
    info.stack_trace_len = trace_len;

    char* p = info.stack_trace;
    for (int i = 0; i < nptrs; i++) {
        size_t len = strlen(strings[i]);
        memcpy(p, strings[i], len);
        p += len;
        *p++ = '\n';
    }
    *p = '\0';

    free(strings);

    /* Call crash callback */
    orbisjvm_mutex_lock(g_crash_mutex);
    if (g_crash_callback) {
        g_crash_callback(&info, g_crash_callback_data);
    }
    orbisjvm_mutex_unlock(g_crash_mutex);

    /* Print to stderr */
    fprintf(stderr, "=== CRASH DETECTED ===\n");
    fprintf(stderr, "Signal: %d\n", signum);
    fprintf(stderr, "Thread: %u\n", info.thread_id);
    fprintf(stderr, "Stack trace:\n%s", info.stack_trace ? info.stack_trace : "(none)");
    fprintf(stderr, "========================\n");

    if (info.stack_trace) orbisjvm_free(info.stack_trace);

    /* Re-raise signal with default handler */
    signal(signum, SIG_DFL);
    raise(signum);
}

/* Log level/category functions */

void orbisjvm_diag_set_log_level(orbisjvm_log_level_t level) {
    g_log_level = level;
}

orbisjvm_log_level_t orbisjvm_diag_get_log_level(void) {
    return g_log_level;
}

void orbisjvm_diag_set_categories(uint32_t categories) {
    g_log_categories = categories;
}

uint32_t orbisjvm_diag_get_categories(void) {
    return g_log_categories;
}

/* Callback functions */

int orbisjvm_diag_register_callback(orbisjvm_log_callback_t callback, void* user_data) {
    if (!callback) return -1;

    orbisjvm_mutex_lock(&g_log_mutex);
    if (g_log_callback_count >= 16) {
        orbisjvm_mutex_unlock(&g_log_mutex);
        return -1;
    }

    g_log_callbacks[g_log_callback_count] = callback;
    g_log_callback_data[g_log_callback_count] = user_data;
    g_log_callback_count++;
    orbisjvm_mutex_unlock(&g_log_mutex);

    return 0;
}

int orbisjvm_diag_unregister_callback(orbisjvm_log_callback_t callback) {
    if (!callback) return -1;

    orbisjvm_mutex_lock(&g_log_mutex);
    for (int i = 0; i < g_log_callback_count; i++) {
        if (g_log_callbacks[i] == callback) {
            for (int j = i; j < g_log_callback_count - 1; j++) {
                g_log_callbacks[j] = g_log_callbacks[j + 1];
                g_log_callback_data[j] = g_log_callback_data[j + 1];
            }
            g_log_callback_count--;
            break;
        }
    }
    orbisjvm_mutex_unlock(&g_log_mutex);

    return 0;
}

/* Logging functions */

void orbisjvm_log(orbisjvm_log_level_t level, orbisjvm_log_category_t category,
                  const char* format, ...) {
    if (level > g_log_level) return;
    if ((category & g_log_categories) == 0) return;

    va_list args;
    va_start(args, format);

    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    buffer[sizeof(buffer) - 1] = '\0';

    va_end(args);

    orbisjvm_log_entry_t entry = {
        .level = level,
        .category = category,
        .timestamp_ns = orbisjvm_get_walltime_ns(),
        .thread_id = (uint32_t)(uintptr_t)orbisjvm_thread_self(),
        .file = NULL,
        .line = 0,
        .func = NULL,
        .message = buffer,
    };

    orbisjvm_mutex_lock(&g_log_mutex);
    for (int i = 0; i < g_log_callback_count; i++) {
        if (g_log_callbacks[i]) {
            g_log_callbacks[i](&entry, g_log_callback_data[i]);
        }
    }
    orbisjvm_mutex_unlock(&g_log_mutex);
}

void orbisjvm_log_ex(orbisjvm_log_level_t level, orbisjvm_log_category_t category,
                     const char* file, int line, const char* func,
                     const char* format, ...) {
    if (level > g_log_level) return;
    if ((category & g_log_categories) == 0) return;

    va_list args;
    va_start(args, format);

    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    buffer[sizeof(buffer) - 1] = '\0';

    va_end(args);

    orbisjvm_log_entry_t entry = {
        .level = level,
        .category = category,
        .timestamp_ns = orbisjvm_get_walltime_ns(),
        .thread_id = (uint32_t)(uintptr_t)orbisjvm_thread_self(),
        .file = file,
        .line = line,
        .func = func,
        .message = buffer,
    };

    orbisjvm_mutex_lock(&g_log_mutex);
    for (int i = 0; i < g_log_callback_count; i++) {
        if (g_log_callbacks[i]) {
            g_log_callbacks[i](&entry, g_log_callback_data[i]);
        }
    }
    orbisjvm_mutex_unlock(&g_log_mutex);
}

/* Profiling */

int orbisjvm_profile_start(orbisjvm_profile_callback_t callback, void* user_data) {
    if (!callback) return -1;

    orbisjvm_mutex_lock(&g_profile_mutex);
    if (g_profiling_active) {
        orbisjvm_mutex_unlock(&g_profile_mutex);
        return -1;
    }

    g_profile_callback = callback;
    g_profile_callback_data = user_data;
    g_profiling_active = 1;
    orbisjvm_mutex_unlock(&g_profile_mutex);

    return 0;
}

int orbisjvm_profile_stop(void) {
    orbisjvm_mutex_lock(&g_profile_mutex);
    g_profiling_active = 0;
    g_profile_callback = NULL;
    g_profile_callback_data = NULL;
    orbisjvm_mutex_unlock(&g_profile_mutex);

    return 0;
}

void orbisjvm_profile_enter(const char* name, const char* category, void* user_data) {
    if (!g_profiling_active || !g_profile_callback) return;

    orbisjvm_profile_event_t event = {
        .type = ORBISJVM_PROFILE_EVENT_ENTER,
        .name = name,
        .category = category,
        .timestamp_ns = orbisjvm_get_monotonic_ns(),
        .thread_id = (uint32_t)(uintptr_t)orbisjvm_thread_self(),
        .duration_ns = 0,
        .user_data = user_data,
    };

    g_profile_callback(&event, g_profile_callback_data);
}

void orbisjvm_profile_exit(const char* name, const char* category, void* user_data) {
    if (!g_profiling_active || !g_profile_callback) return;

    orbisjvm_profile_event_t event = {
        .type = ORBISJVM_PROFILE_EVENT_EXIT,
        .name = name,
        .category = category,
        .timestamp_ns = orbisjvm_get_monotonic_ns(),
        .thread_id = (uint32_t)(uintptr_t)orbisjvm_thread_self(),
        .duration_ns = 0,  /* Duration would be calculated by callback */
        .user_data = user_data,
    };

    g_profile_callback(&event, g_profile_callback_data);
}

void orbisjvm_profile_instant(const char* name, const char* category, void* user_data) {
    if (!g_profiling_active || !g_profile_callback) return;

    orbisjvm_profile_event_t event = {
        .type = ORBISJVM_PROFILE_EVENT_INSTANT,
        .name = name,
        .category = category,
        .timestamp_ns = orbisjvm_get_monotonic_ns(),
        .thread_id = (uint32_t)(uintptr_t)orbisjvm_thread_self(),
        .duration_ns = 0,
        .user_data = user_data,
    };

    g_profile_callback(&event, g_profile_callback_data);
}

/* Crash handling */

int orbisjvm_crash_register_handler(orbisjvm_crash_callback_t callback, void* user_data) {
    if (!callback) return -1;

    orbisjvm_mutex_lock(&g_crash_mutex);
    g_crash_callback = callback;
    g_crash_callback_data = user_data;
    orbisjvm_mutex_unlock(&g_crash_mutex);

    return 0;
}

int orbisjvm_crash_unregister_handler(void) {
    orbisjvm_mutex_lock(&g_crash_mutex);
    g_crash_callback = NULL;
    g_crash_callback_data = NULL;
    orbisjvm_mutex_unlock(&g_crash_mutex);

    return 0;
}

void orbisjvm_crash_simulate(int signal) {
    raise(signal);
}

/* Memory diagnostics */

void orbisjvm_memory_snapshot(orbisjvm_memory_snapshot_t* snapshot) {
    if (!snapshot) return;

    size_t current, total, peak;
    uint64_t allocs, frees;
    orbisjvm_get_allocation_stats(&current, &total, &peak, &allocs, &frees);

    snapshot->total_allocated = total;
    snapshot->current_allocated = total;
    snapshot->peak_allocated = peak;
    snapshot->total_freed = frees;
    snapshot->allocation_count = allocs;
    snapshot->free_count = frees;
    snapshot->leak_count = allocs - frees;
    snapshot->largest_allocation = 0;  /* Would need tracker iteration */
    snapshot->average_allocation = allocs > 0 ? total / allocs : 0;
}

int orbisjvm_memory_dump_allocations(const char* path) {
    if (!path) return -1;

    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, path, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret < 0) return ret;

    /* Write header */
    char header[256];
    int len = snprintf(header, sizeof(header),
        "# OrbisJVM Memory Allocation Dump\n"
        "# Generated at %llu ns\n\n",
        orbisjvm_get_walltime_ns());
    size_t written;
    orbisjvm_file_write(file, header, len, &written);

    /* Dump allocations */
    orbisjvm_dump_allocations(dump_allocation_callback, file);

    orbisjvm_file_close(file);
    return 0;
}

static void dump_allocation_callback(const orbisjvm_alloc_tracker_t* entry, void* user_data) {
    orbisjvm_file_t file = (orbisjvm_file_t)user_data;
    char buffer[512];
    int len = snprintf(buffer, sizeof(buffer),
        "ptr=%p size=%zu flags=0x%x file=%s:%d func=%s time=%llu\n",
        entry->ptr, entry->size, entry->flags,
        entry->file ? entry->file : "unknown", entry->line,
        entry->func ? entry->func : "unknown", entry->timestamp);
    size_t written;
    orbisjvm_file_write(file, buffer, len, &written);
}

/* Thread diagnostics */

int orbisjvm_thread_snapshot(orbisjvm_thread_t thread, orbisjvm_thread_snapshot_t* snapshot) {
    if (!thread || !snapshot) return -1;

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->thread_id = (uint32_t)(uintptr_t)thread->pthread;

    char name[32];
    orbisjvm_thread_get_name(thread, name, sizeof(name));
    strncpy(snapshot->name, name, sizeof(snapshot->name) - 1);

    snapshot->state = 0;  /* Running */
    snapshot->cpu_time_ns = 0;  /* Not available */
    snapshot->stack_size = 1024 * 1024;  /* Default */

    return 0;
}

int orbisjvm_dump_all_threads(const char* path) {
    if (!path) return -1;

    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, path, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret < 0) return ret;

    char header[256];
    int len = snprintf(header, sizeof(header),
        "# OrbisJVM Thread Dump\n"
        "# Generated at %llu ns\n\n",
        orbisjvm_get_walltime_ns());
    size_t written;
    orbisjvm_file_write(file, header, len, &written);

    /* Current thread */
    char buffer[512];
    len = snprintf(buffer, sizeof(buffer),
        "Thread %u: %s\n", (uint32_t)(uintptr_t)orbisjvm_thread_self(), "main");
    orbisjvm_file_write(file, buffer, len, &written);

    orbisjvm_file_close(file);
    return 0;
}

/* System diagnostics */

int orbisjvm_generate_diagnostic_report(const char* path) {
    if (!path) return -1;

    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, path, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret < 0) return ret;

    char buffer[4096];
    size_t written;

    orbisjvm_version_info_t version;
    orbisjvm_get_version(&version);

    int len = snprintf(buffer, sizeof(buffer),
        "=== OrbisJVM Diagnostic Report ===\n"
        "Version: %s\n"
        "Build Date: %s\n"
        "Build Commit: %s\n"
        "Timestamp: %llu ns\n\n",
        version.version_string, version.build_date, version.build_commit,
        orbisjvm_get_walltime_ns());
    orbisjvm_file_write(file, buffer, len, &written);

    /* Platform info */
    orbisjvm_platform_info_t platform;
    orbisjvm_get_platform_info(&platform);

    len = snprintf(buffer, sizeof(buffer),
        "Platform: %s %s (%s)\n"
        "CPU: %s (%u cores, %llu Hz)\n"
        "Memory: %llu MB total, %llu MB available\n"
        "Page Size: %u bytes\n\n",
        platform.os_name, platform.os_version, platform.architecture,
        platform.cpu.vendor, platform.cpu.core_count, platform.cpu.frequency_hz,
        platform.memory.total_physical / (1024*1024),
        platform.memory.available_physical / (1024*1024),
        platform.memory.page_size);
    orbisjvm_file_write(file, buffer, len, &written);

    /* Capabilities */
    len = snprintf(buffer, sizeof(buffer), "Capabilities:\n");
    orbisjvm_file_write(file, buffer, len, &written);

    for (int i = 0; i < 32; i++) {
        uint64_t cap = (1ULL << i);
        if (platform.capabilities & cap) {
            const char* names[] = {
                "MEMORY_BASIC", "MEMORY_PAGE_ALLOC", "MEMORY_PROTECT", "MEMORY_MAP",
                "THREADING_BASIC", "THREADING_MUTEX", "THREADING_CONDVAR", "THREADING_TLS",
                "FILESYSTEM_BASIC", "FILESYSTEM_DIR", "FILESYSTEM_META",
                "NETWORK_TCP", "NETWORK_UDP", "NETWORK_DNS",
                "TIME_MONOTONIC", "TIME_WALLCLOCK", "TIME_HR_TIMER",
                "ENV_VARS", "PROCESS_INFO", "JNI_BASIC", "TLS"
            };
            if (i < 21) {
                len = snprintf(buffer, sizeof(buffer), "  [X] %s\n", names[i]);
                orbisjvm_file_write(file, buffer, len, &written);
            }
        }
    }

    /* Memory stats */
    orbisjvm_memory_snapshot_t mem;
    orbisjvm_memory_snapshot(&mem);

    len = snprintf(buffer, sizeof(buffer),
        "\nMemory Stats:\n"
        "  Current Allocated: %zu bytes\n"
        "  Peak Allocated: %zu bytes\n"
        "  Total Allocations: %llu\n"
        "  Total Frees: %llu\n"
        "  Leaks: %llu\n\n",
        mem.current_allocated, mem.peak_allocated,
        mem.allocation_count, mem.free_count, mem.leak_count);
    orbisjvm_file_write(file, buffer, len, &written);

    /* Network stats */
    orbisjvm_net_stats_t net;
    orbisjvm_get_net_stats(&net);

    len = snprintf(buffer, sizeof(buffer),
        "Network Stats:\n"
        "  Bytes Sent: %llu\n"
        "  Bytes Received: %llu\n"
        "  Packets Sent: %llu\n"
        "  Packets Received: %llu\n"
        "  Errors: %llu\n"
        "  Current Sockets: %u\n"
        "  Peak Sockets: %u\n\n",
        net.bytes_sent, net.bytes_received,
        net.packets_sent, net.packets_received,
        net.errors, net.current_sockets, net.peak_sockets);
    orbisjvm_file_write(file, buffer, len, &written);

    /* Thread stats */
    orbisjvm_thread_stats_t thread_stats;
    orbisjvm_get_thread_stats(&thread_stats);

    len = snprintf(buffer, sizeof(buffer),
        "Thread Stats:\n"
        "  Created: %llu\n"
        "  Destroyed: %llu\n"
        "  Current: %u\n"
        "  Peak: %u\n\n",
        thread_stats.threads_created, thread_stats.threads_destroyed,
        thread_stats.current_thread_count, thread_stats.peak_thread_count);
    orbisjvm_file_write(file, buffer, len, &written);

    orbisjvm_file_close(file);
    return 0;
}

void orbisjvm_print_diagnostic_report(void) {
    char path[256];
    snprintf(path, sizeof(path), "/tmp/orbisjvm_diag_%llu.txt", orbisjvm_get_walltime_ns());

    if (orbisjvm_generate_diagnostic_report(path) == 0) {
        orbisjvm_file_t file;
        if (orbisjvm_file_open(&file, path, ORBISJVM_O_RDONLY, 0) == 0) {
            char buffer[4096];
            size_t read;
            while (orbisjvm_file_read(file, buffer, sizeof(buffer) - 1, &read) == 0 && read > 0) {
                buffer[read] = '\0';
                printf("%s", buffer);
            }
            orbisjvm_file_close(file);
        }
    }
}

/* Performance counters (stub) */

int orbisjvm_perf_start(orbisjvm_perf_counter_t counter) {
    (void)counter;
    return -ENOSYS;
}

int orbisjvm_perf_stop(orbisjvm_perf_counter_t counter, uint64_t* value) {
    (void)counter; (void)value;
    return -ENOSYS;
}

int orbisjvm_perf_read(orbisjvm_perf_counter_t counter, uint64_t* value) {
    (void)counter; (void)value;
    return -ENOSYS;
}