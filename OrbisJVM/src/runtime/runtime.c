/**
 * @file runtime.c
 * @brief OrbisJVM Runtime Implementation
 *
 * Core runtime initialization, version management, and platform capability detection.
 */

#include <orbisjvm/runtime.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/threading.h>
#include <orbisjvm/filesystem.h>
#include <orbisjvm/networking.h>
#include <orbisjvm/time.h>
#include <orbisjvm/environment.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/security.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Platform headers - will be replaced with actual OpenOrbis includes */
#include <orbis/SystemService.h>
#include <orbis/SysUtil.h>
#include <orbis/Rtc.h>

/* Internal state */
static int g_initialized = 0;
static orbisjvm_config_t g_config = {0};
static orbisjvm_platform_info_t g_platform_info = {0};
static uint64_t g_capabilities = 0;

/* Build information */
static const char* g_build_date = __DATE__ " " __TIME__;
static const char* g_build_commit = "unknown";

/* Forward declarations */
static int detect_platform_capabilities(void);
static int init_platform_info(void);
static void print_capability(uint64_t cap, const char* name);

/**
 * @brief Initialize the OrbisJVM runtime
 */
int orbisjvm_init(void) {
    if (g_initialized) {
        orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "Runtime already initialized");
        return 0;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Initializing OrbisJVM runtime v%s", ORBISJVM_VERSION_STRING);

    /* Initialize diagnostics first */
    if (orbisjvm_diag_init() < 0) {
        fprintf(stderr, "Failed to initialize diagnostics\n");
        return -1;
    }

    /* Set default configuration */
    orbisjvm_get_default_config(&g_config);

    /* Detect platform capabilities */
    if (detect_platform_capabilities() < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to detect platform capabilities");
        return -2;
    }

    /* Initialize platform info */
    if (init_platform_info() < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to initialize platform info");
        return -3;
    }

    /* Initialize subsystems */
    if (orbisjvm_memory_init() < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_MEMORY, "Failed to initialize memory subsystem");
        return -4;
    }

    if (orbisjvm_threading_init() < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_THREADING, "Failed to initialize threading subsystem");
        return -5;
    }

    if (orbisjvm_filesystem_init() < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_FILESYSTEM, "Failed to initialize filesystem subsystem");
        return -6;
    }

    if (orbisjvm_networking_init() < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "Failed to initialize networking subsystem");
        return -7;
    }

    if (orbisjvm_time_init() < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to initialize time subsystem");
        return -8;
    }

    if (orbisjvm_environment_init() < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to initialize environment subsystem");
        return -9;
    }

    if (orbisjvm_security_init() < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_SECURITY, "Failed to initialize security subsystem");
        return -10;
    }

    g_initialized = 1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "OrbisJVM runtime initialized successfully");
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Platform: %s %s (%s)", g_platform_info.os_name, g_platform_info.os_version, g_platform_info.architecture);
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "CPU: %s (%u cores, %llu Hz)", g_platform_info.cpu.vendor, g_platform_info.cpu.core_count, g_platform_info.cpu.frequency_hz);
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Memory: %llu MB total, %llu MB available",
                      g_platform_info.memory.total_physical / (1024*1024),
                      g_platform_info.memory.available_physical / (1024*1024));

    return 0;
}

/**
 * @brief Shutdown the OrbisJVM runtime
 */
void orbisjvm_shutdown(void) {
    if (!g_initialized) {
        return;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Shutting down OrbisJVM runtime");

    /* Shutdown subsystems in reverse order */
    orbisjvm_security_shutdown();
    orbisjvm_environment_shutdown();
    orbisjvm_time_shutdown();
    orbisjvm_networking_shutdown();
    orbisjvm_filesystem_shutdown();
    orbisjvm_threading_shutdown();
    orbisjvm_memory_shutdown();

    orbisjvm_diag_shutdown();

    g_initialized = 0;
    memset(&g_config, 0, sizeof(g_config));
    memset(&g_platform_info, 0, sizeof(g_platform_info));
    g_capabilities = 0;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "OrbisJVM runtime shutdown complete");
}

/**
 * @brief Get OrbisJVM version information
 */
void orbisjvm_get_version(orbisjvm_version_info_t* info) {
    if (!info) return;

    info->major = ORBISJVM_VERSION_MAJOR;
    info->minor = ORBISJVM_VERSION_MINOR;
    info->patch = ORBISJVM_VERSION_PATCH;
    info->version_string = ORBISJVM_VERSION_STRING;
    info->build_date = g_build_date;
    info->build_commit = g_build_commit;
}

/**
 * @brief Get platform capabilities and information
 */
int orbisjvm_get_platform_info(orbisjvm_platform_info_t* info) {
    if (!info) return -1;
    if (!g_initialized) return -2;

    *info = g_platform_info;
    info->capabilities = g_capabilities;
    return 0;
}

/**
 * @brief Check if a specific capability is available
 */
int orbisjvm_has_capability(orbisjvm_capability_t cap) {
    return (g_capabilities & cap) != 0;
}

/**
 * @brief Print diagnostic information to stdout
 */
void orbisjvm_print_diagnostics(void) {
    orbisjvm_version_info_t version;
    orbisjvm_get_version(&version);

    printf("=== OrbisJVM Diagnostics ===\n");
    printf("Version: %s\n", version.version_string);
    printf("Build Date: %s\n", version.build_date);
    printf("Build Commit: %s\n", version.build_commit);
    printf("\n");

    if (g_initialized) {
        printf("Platform: %s %s (%s)\n", g_platform_info.os_name, g_platform_info.os_version, g_platform_info.architecture);
        printf("CPU: %s (%u cores, %llu Hz)\n", g_platform_info.cpu.vendor, g_platform_info.cpu.core_count, g_platform_info.cpu.frequency_hz);
        printf("Memory: %llu MB total, %llu MB available\n",
               g_platform_info.memory.total_physical / (1024*1024),
               g_platform_info.memory.available_physical / (1024*1024));
        printf("Page Size: %u bytes\n", g_platform_info.memory.page_size);
        printf("\n");

        printf("Capabilities:\n");
        print_capability(ORBISJVM_CAP_MEMORY_BASIC, "Memory Basic");
        print_capability(ORBISJVM_CAP_MEMORY_PAGE_ALLOC, "Memory Page Alloc");
        print_capability(ORBISJVM_CAP_MEMORY_PROTECT, "Memory Protect");
        print_capability(ORBISJVM_CAP_MEMORY_MAP, "Memory Map");
        print_capability(ORBISJVM_CAP_THREADING_BASIC, "Threading Basic");
        print_capability(ORBISJVM_CAP_THREADING_MUTEX, "Threading Mutex");
        print_capability(ORBISJVM_CAP_THREADING_CONDVAR, "Threading CondVar");
        print_capability(ORBISJVM_CAP_THREADING_TLS, "Threading TLS");
        print_capability(ORBISJVM_CAP_FILESYSTEM_BASIC, "Filesystem Basic");
        print_capability(ORBISJVM_CAP_FILESYSTEM_DIR, "Filesystem Directory");
        print_capability(ORBISJVM_CAP_FILESYSTEM_META, "Filesystem Metadata");
        print_capability(ORBISJVM_CAP_NETWORK_TCP, "Network TCP");
        print_capability(ORBISJVM_CAP_NETWORK_UDP, "Network UDP");
        print_capability(ORBISJVM_CAP_NETWORK_DNS, "Network DNS");
        print_capability(ORBISJVM_CAP_TIME_MONOTONIC, "Time Monotonic");
        print_capability(ORBISJVM_CAP_TIME_WALLCLOCK, "Time Wall Clock");
        print_capability(ORBISJVM_CAP_TIME_HR_TIMER, "Time High-Res Timer");
        print_capability(ORBISJVM_CAP_ENV_VARS, "Environment Variables");
        print_capability(ORBISJVM_CAP_PROCESS_INFO, "Process Info");
        print_capability(ORBISJVM_CAP_JNI_BASIC, "JNI Basic");
        print_capability(ORBISJVM_CAP_TLS, "TLS/SSL");
    } else {
        printf("Runtime not initialized\n");
    }
    printf("============================\n");
}

/**
 * @brief Get default runtime configuration
 */
void orbisjvm_get_default_config(orbisjvm_config_t* config) {
    if (!config) return;

    config->enable_diagnostics = 1;
    config->enable_memory_tracking = 1;
    config->enable_thread_tracking = 1;
    config->max_heap_size = 512 * 1024 * 1024;  /* 512 MB default */
    config->initial_heap_size = 128 * 1024 * 1024;  /* 128 MB default */

    /* Get standard directories */
    char temp_dir[256];
    char config_dir[256];
    orbisjvm_get_temp_dir(temp_dir, sizeof(temp_dir));
    orbisjvm_get_app_config_dir(config_dir, sizeof(config_dir));

    config->temp_dir = strdup(temp_dir);
    config->config_dir = strdup(config_dir);
}

/**
 * @brief Set runtime configuration
 */
int orbisjvm_set_config(const orbisjvm_config_t* config) {
    if (!config) return -1;
    if (g_initialized) {
        orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "Configuration change after init may not take full effect");
    }

    g_config = *config;
    return 0;
}

/**
 * @brief Get current runtime configuration
 */
void orbisjvm_get_config(orbisjvm_config_t* config) {
    if (!config) return;
    *config = g_config;
}

/* Internal functions */

static int detect_platform_capabilities(void) {
    uint64_t caps = 0;

    /* Memory capabilities */
    caps |= ORBISJVM_CAP_MEMORY_BASIC;  /* malloc/free always available */
    caps |= ORBISJVM_CAP_MEMORY_PAGE_ALLOC;  /* Page allocation via syscalls */
    caps |= ORBISJVM_CAP_MEMORY_PROTECT;  /* mprotect equivalent */

    /* Threading capabilities */
    caps |= ORBISJVM_CAP_THREADING_BASIC;  /* Thread create/join via pthreads */
    caps |= ORBISJVM_CAP_THREADING_MUTEX;  /* Mutex support */
    caps |= ORBISJVM_CAP_THREADING_CONDVAR;  /* Condition variables */
    caps |= ORBISJVM_CAP_THREADING_TLS;  /* Thread-local storage */

    /* Filesystem capabilities */
    caps |= ORBISJVM_CAP_FILESYSTEM_BASIC;  /* File I/O via FIOS2 */
    caps |= ORBISJVM_CAP_FILESYSTEM_DIR;  /* Directory operations */
    caps |= ORBISJVM_CAP_FILESYSTEM_META;  /* File metadata */

    /* Network capabilities */
    caps |= ORBISJVM_CAP_NETWORK_TCP;  /* TCP via sceNet */
    caps |= ORBISJVM_CAP_NETWORK_UDP;  /* UDP via sceNet */
    caps |= ORBISJVM_CAP_NETWORK_DNS;  /* DNS resolution */

    /* Time capabilities */
    caps |= ORBISJVM_CAP_TIME_MONOTONIC;  /* Monotonic clock via RTC */
    caps |= ORBISJVM_CAP_TIME_WALLCLOCK;  /* Wall clock via RTC */
    caps |= ORBISJVM_CAP_TIME_HR_TIMER;  /* High-res timers */

    /* Environment capabilities */
    caps |= ORBISJVM_CAP_ENV_VARS;  /* Environment variables */
    caps |= ORBISJVM_CAP_PROCESS_INFO;  /* Process info */

    /* JNI - not yet implemented */
    /* caps |= ORBISJVM_CAP_JNI_BASIC; */

    /* TLS - depends on OpenSSL port */
    /* caps |= ORBISJVM_CAP_TLS; */

    g_capabilities = caps;
    return 0;
}

static int init_platform_info(void) {
    /* Set platform info */
    strncpy(g_platform_info.platform_name, "PlayStation 4", sizeof(g_platform_info.platform_name) - 1);
    strncpy(g_platform_info.os_name, "Orbis OS", sizeof(g_platform_info.os_name) - 1);
    strncpy(g_platform_info.os_version, "9.00", sizeof(g_platform_info.os_version) - 1);
    strncpy(g_platform_info.architecture, "x86_64", sizeof(g_platform_info.architecture) - 1);

    /* Memory info - use actual syscalls when available */
    g_platform_info.memory.total_physical = 8ULL * 1024 * 1024 * 1024;  /* 8 GB */
    g_platform_info.memory.available_physical = 6ULL * 1024 * 1024 * 1024;  /* ~6 GB for apps */
    g_platform_info.memory.total_virtual = 256ULL * 1024 * 1024 * 1024;  /* 256 TB virtual */
    g_platform_info.memory.used_virtual = 0;
    g_platform_info.memory.page_size = 4096;

    /* CPU info */
    g_platform_info.cpu.core_count = 8;
    g_platform_info.cpu.thread_count = 8;
    g_platform_info.cpu.frequency_hz = 1600000000ULL;  /* 1.6 GHz base */
    strncpy(g_platform_info.cpu.vendor, "AMD", sizeof(g_platform_info.cpu.vendor) - 1);
    strncpy(g_platform_info.cpu.model, "Jaguar", sizeof(g_platform_info.cpu.model) - 1);
    g_platform_info.cpu.features = ORBISJVM_CPU_FEATURE_FPU |
                                   ORBISJVM_CPU_FEATURE_SSE |
                                   ORBISJVM_CPU_FEATURE_SSE2 |
                                   ORBISJVM_CPU_FEATURE_SSE3 |
                                   ORBISJVM_CPU_FEATURE_SSSE3 |
                                   ORBISJVM_CPU_FEATURE_SSE4_1 |
                                   ORBISJVM_CPU_FEATURE_SSE4_2 |
                                   ORBISJVM_CPU_FEATURE_AVX |
                                   ORBISJVM_CPU_FEATURE_FMA |
                                   ORBISJVM_CPU_FEATURE_AES |
                                   ORBISJVM_CPU_FEATURE_RDRAND;

    return 0;
}

static void print_capability(uint64_t cap, const char* name) {
    int has = (g_capabilities & cap) != 0;
    printf("  [%c] %s\n", has ? 'X' : ' ', name);
}

/* Subsystem init/shutdown stubs - to be implemented in respective modules */
int orbisjvm_memory_init(void) { return 0; }
void orbisjvm_memory_shutdown(void) {}
int orbisjvm_threading_init(void) { return 0; }
void orbisjvm_threading_shutdown(void) {}
int orbisjvm_filesystem_init(void) { return 0; }
void orbisjvm_filesystem_shutdown(void) {}
int orbisjvm_networking_init(void) { return 0; }
void orbisjvm_networking_shutdown(void) {}
int orbisjvm_time_init(void) { return 0; }
void orbisjvm_time_shutdown(void) {}
int orbisjvm_environment_init(void) { return 0; }
void orbisjvm_environment_shutdown(void) {}
int orbisjvm_security_init(void) { return 0; }
void orbisjvm_security_shutdown(void) {}