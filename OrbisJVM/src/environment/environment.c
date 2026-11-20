/**
 * @file environment.c
 * @brief OrbisJVM Environment Implementation
 *
 * Implements environment variable handling, system information, and configuration.
 */

#include <orbisjvm/environment.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/filesystem.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>

/**
 * @brief Initialize environment subsystem
 */
int orbisjvm_environment_init(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Initializing environment subsystem");
    return 0;
}

/**
 * @brief Shutdown environment subsystem
 */
void orbisjvm_environment_shutdown(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Shutting down environment subsystem");
}

/* Environment variables */

const char* orbisjvm_getenv(const char* name) {
    if (!name) return NULL;
    return getenv(name);
}

int orbisjvm_setenv(const char* name, const char* value, int overwrite) {
    if (!name) return -1;
    return setenv(name, value ? value : "", overwrite);
}

int orbisjvm_unsetenv(const char* name) {
    if (!name) return -1;
    return unsetenv(name);
}

int orbisjvm_get_environment(char** envp, size_t max_entries, size_t* count) {
    if (!envp || !count || max_entries == 0) return -1;

    extern char** environ;
    size_t i = 0;
    for (char** p = environ; *p && i < max_entries; p++, i++) {
        envp[i] = *p;
    }
    *count = i;
    return 0;
}

int orbisjvm_clearenv(void) {
    extern char** environ;
    environ = NULL;
    return 0;
}

/* System information */

int orbisjvm_get_system_info(orbisjvm_system_info_t* info) {
    if (!info) return -1;

    memset(info, 0, sizeof(*info));

    struct utsname uts;
    if (uname(&uts) == 0) {
        strncpy(info->os_name, uts.sysname, sizeof(info->os_name) - 1);
        strncpy(info->os_version, uts.release, sizeof(info->os_version) - 1);
        strncpy(info->kernel_version, uts.version, sizeof(info->kernel_version) - 1);
        strncpy(info->architecture, uts.machine, sizeof(info->architecture) - 1);
    } else {
        strncpy(info->os_name, "Orbis OS", sizeof(info->os_name) - 1);
        strncpy(info->os_version, "9.00", sizeof(info->os_version) - 1);
        strncpy(info->architecture, "x86_64", sizeof(info->architecture) - 1);
    }

    strncpy(info->hardware_model, "PlayStation 4", sizeof(info->hardware_model) - 1);
    strncpy(info->hardware_serial, "unknown", sizeof(info->hardware_serial) - 1);

    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        info->total_memory = si.totalram * si.mem_unit;
        info->available_memory = si.freeram * si.mem_unit;
        info->uptime_ns = (uint64_t)si.uptime * 1000000000ULL;
        info->boot_time = orbisjvm_get_walltime_ns() - info->uptime_ns;
    } else {
        info->total_memory = 8ULL * 1024 * 1024 * 1024;
        info->available_memory = 6ULL * 1024 * 1024 * 1024;
    }

    info->page_size = sysconf(_SC_PAGESIZE);
    info->cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    info->cpu_frequency_hz = 1600000000ULL;

    return 0;
}

int orbisjvm_get_cpu_info(orbisjvm_cpu_info_t* info) {
    if (!info) return -1;

    memset(info, 0, sizeof(*info));

    info->core_count = sysconf(_SC_NPROCESSORS_ONLN);
    info->thread_count = info->core_count;
    info->frequency_hz = 1600000000ULL;

    strncpy(info->vendor, "AMD", sizeof(info->vendor) - 1);
    strncpy(info->model, "Jaguar", sizeof(info->model) - 1);

    info->features = ORBISJVM_CPU_FEATURE_FPU |
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

    info->cache_line_size = 64;
    info->l1_cache_size = 32 * 1024;
    info->l2_cache_size = 2 * 1024 * 1024;
    info->l3_cache_size = 0;

    return 0;
}

/* Process information */

int orbisjvm_get_process_info(orbisjvm_process_info_t* info) {
    if (!info) return -1;

    memset(info, 0, sizeof(*info));

    info->pid = getpid();
    info->ppid = getppid();
    info->uid = getuid();
    info->gid = getgid();

    /* Get executable path */
    char exe_path[256];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len > 0) {
        exe_path[len] = '\0';
        strncpy(info->exe_path, exe_path, sizeof(info->exe_path) - 1);
    }

    strncpy(info->name, "orbisjvm", sizeof(info->name) - 1);

    /* Get CWD */
    getcwd(info->cwd, sizeof(info->cwd) - 1);

    /* Get start time (approximate) */
    info->start_time = orbisjvm_get_walltime_ns() - (1000000000ULL * 60);  /* ~1 minute ago */

    return 0;
}

uint32_t orbisjvm_getpid(void) {
    return getpid();
}

uint32_t orbisjvm_getppid(void) {
    return getppid();
}

uint32_t orbisjvm_getuid(void) {
    return getuid();
}

uint32_t orbisjvm_getgid(void) {
    return getgid();
}

/* Application paths */

int orbisjvm_get_app_paths(orbisjvm_app_paths_t* paths) {
    if (!paths) return -1;

    memset(paths, 0, sizeof(*paths));

    char exe_path[256];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len > 0) {
        exe_path[len] = '\0';
        strncpy(paths->exe_path, exe_path, sizeof(paths->exe_path) - 1);

        /* Extract directory */
        char* last_slash = strrchr(exe_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            strncpy(paths->exe_dir, exe_path, sizeof(paths->exe_dir) - 1);
        }
    }

    orbisjvm_get_app_data_dir(paths->data_dir, sizeof(paths->data_dir));
    orbisjvm_get_app_config_dir(paths->config_dir, sizeof(paths->config_dir));
    orbisjvm_get_app_cache_dir(paths->cache_dir, sizeof(paths->cache_dir));
    orbisjvm_get_temp_dir(paths->temp_dir, sizeof(paths->temp_dir));
    orbisjvm_get_cwd(paths->home_dir, sizeof(paths->home_dir));

    return 0;
}

/* Runtime configuration */

void orbisjvm_get_default_runtime_config(orbisjvm_runtime_config_t* config) {
    if (!config) return;

    memset(config, 0, sizeof(*config));

    config->log_level = 3;  /* INFO */
    config->enable_console_log = 1;
    config->enable_file_log = 0;
    config->max_log_size = 10 * 1024 * 1024;  /* 10 MB */
    config->max_log_files = 5;
    config->default_stack_size = 1024 * 1024;  /* 1 MB */
    config->max_heap_size = 512 * 1024 * 1024;  /* 512 MB */
    config->initial_heap_size = 128 * 1024 * 1024;  /* 128 MB */
    config->enable_jit = 1;
    config->enable_gc_verbose = 0;
    config->enable_class_load_log = 0;
    config->network_timeout_ms = 30000;
    config->dns_cache_ttl = 300;
}

int orbisjvm_load_runtime_config(const char* path, orbisjvm_runtime_config_t* config) {
    if (!path || !config) return -1;

    /* Simple key=value file parser */
    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, path, ORBISJVM_O_RDONLY, 0);
    if (ret < 0) return ret;

    char line[512];
    size_t bytes_read;
    while ((ret = orbisjvm_file_read(file, line, sizeof(line) - 1, &bytes_read)) == 0 && bytes_read > 0) {
        line[bytes_read] = '\0';

        char* nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';

        char* key = line;
        char* value = eq + 1;

        if (strcmp(key, "log_level") == 0) config->log_level = atoi(value);
        else if (strcmp(key, "enable_console_log") == 0) config->enable_console_log = atoi(value);
        else if (strcmp(key, "enable_file_log") == 0) config->enable_file_log = atoi(value);
        else if (strcmp(key, "log_file") == 0) strncpy(config->log_file, value, sizeof(config->log_file) - 1);
        else if (strcmp(key, "max_log_size") == 0) config->max_log_size = atoll(value);
        else if (strcmp(key, "max_log_files") == 0) config->max_log_files = atoi(value);
        else if (strcmp(key, "default_stack_size") == 0) config->default_stack_size = atoll(value);
        else if (strcmp(key, "max_heap_size") == 0) config->max_heap_size = atoll(value);
        else if (strcmp(key, "initial_heap_size") == 0) config->initial_heap_size = atoll(value);
        else if (strcmp(key, "enable_jit") == 0) config->enable_jit = atoi(value);
        else if (strcmp(key, "enable_gc_verbose") == 0) config->enable_gc_verbose = atoi(value);
        else if (strcmp(key, "enable_class_load_log") == 0) config->enable_class_load_log = atoi(value);
        else if (strcmp(key, "network_timeout_ms") == 0) config->network_timeout_ms = atoi(value);
        else if (strcmp(key, "dns_cache_ttl") == 0) config->dns_cache_ttl = atoi(value);
    }

    orbisjvm_file_close(file);
    return 0;
}

int orbisjvm_save_runtime_config(const char* path, const orbisjvm_runtime_config_t* config) {
    if (!path || !config) return -1;

    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, path, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret < 0) return ret;

    char buffer[1024];
    int len = snprintf(buffer, sizeof(buffer),
        "log_level=%d\n"
        "enable_console_log=%d\n"
        "enable_file_log=%d\n"
        "log_file=%s\n"
        "max_log_size=%zu\n"
        "max_log_files=%d\n"
        "default_stack_size=%zu\n"
        "max_heap_size=%zu\n"
        "initial_heap_size=%zu\n"
        "enable_jit=%d\n"
        "enable_gc_verbose=%d\n"
        "enable_class_load_log=%d\n"
        "network_timeout_ms=%d\n"
        "dns_cache_ttl=%d\n",
        config->log_level,
        config->enable_console_log,
        config->enable_file_log,
        config->log_file,
        config->max_log_size,
        config->max_log_files,
        config->default_stack_size,
        config->max_heap_size,
        config->initial_heap_size,
        config->enable_jit,
        config->enable_gc_verbose,
        config->enable_class_load_log,
        config->network_timeout_ms,
        config->dns_cache_ttl);

    size_t written;
    ret = orbisjvm_file_write(file, buffer, len, &written);
    orbisjvm_file_close(file);

    return ret;
}

int orbisjvm_apply_runtime_config(const orbisjvm_runtime_config_t* config) {
    if (!config) return -1;

    orbisjvm_diag_set_log_level(config->log_level);

    /* Apply other settings as needed */
    return 0;
}

void orbisjvm_get_runtime_config(orbisjvm_runtime_config_t* config) {
    if (!config) return;

    config->log_level = orbisjvm_diag_get_log_level();
    /* Other values would be stored globally */
}

/* Random number generation */

int orbisjvm_getrandom(void* buffer, size_t size) {
    if (!buffer || size == 0) return -1;

    /* Use /dev/urandom or getrandom syscall */
    FILE* f = fopen("/dev/urandom", "rb");
    if (!f) return -1;

    size_t read = fread(buffer, 1, size, f);
    fclose(f);

    return read == size ? 0 : -1;
}

int orbisjvm_generate_uuid(char* uuid) {
    if (!uuid) return -1;

    uint8_t bytes[16];
    int ret = orbisjvm_getrandom(bytes, 16);
    if (ret < 0) return ret;

    /* Set version (4) and variant bits */
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    snprintf(uuid, 37,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0], bytes[1], bytes[2], bytes[3],
        bytes[4], bytes[5], bytes[6], bytes[7],
        bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);

    return 0;
}

int orbisjvm_random_bytes(void* buffer, size_t size) {
    return orbisjvm_getrandom(buffer, size);
}

/* Arguments */

static int g_argc = 0;
static char** g_argv = NULL;

int orbisjvm_get_args(int* argc, char*** argv) {
    if (!argc || !argv) return -1;
    *argc = g_argc;
    *argv = g_argv;
    return 0;
}

int orbisjvm_set_args(int argc, char** argv) {
    g_argc = argc;
    g_argv = argv;
    return 0;
}