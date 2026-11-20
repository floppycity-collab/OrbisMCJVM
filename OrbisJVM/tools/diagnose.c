/**
 * @file diagnose.c
 * @brief OrbisJVM Diagnostic Tool
 *
 * Command-line tool for diagnosing OrbisJVM platform capabilities and runtime status.
 */

#include <orbisjvm/runtime.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/threading.h>
#include <orbisjvm/filesystem.h>
#include <orbisjvm/networking.h>
#include <orbisjvm/time.h>
#include <orbisjvm/environment.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

static void print_usage(const char* progname) {
    printf("Usage: %s [OPTIONS]\n", progname);
    printf("\n");
    printf("OrbisJVM Diagnostic Tool\n");
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -v, --version           Show version information\n");
    printf("  -a, --all               Run all diagnostics\n");
    printf("  -p, --platform          Show platform information\n");
    printf("  -c, --capabilities      Show platform capabilities\n");
    printf("  -m, --memory            Show memory information\n");
    printf("  -t, --threading         Show threading information\n");
    printf("  -f, --filesystem        Show filesystem information\n");
    printf("  -n, --network           Show network information\n");
    printf("  -T, --time              Show time information\n");
    printf("  -e, --environment       Show environment information\n");
    printf("  -r, --report FILE       Generate full diagnostic report to FILE\n");
    printf("  -j, --json              Output in JSON format\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s --all                Run all diagnostics\n", progname);
    printf("  %s -p -c -m             Show platform, capabilities, and memory\n", progname);
    printf("  %s -r report.txt        Generate full report\n", progname);
}

static void print_version(void) {
    orbisjvm_version_info_t version;
    orbisjvm_get_version(&version);
    printf("OrbisJVM %s\n", version.version_string);
    printf("Build Date: %s\n", version.build_date);
    printf("Build Commit: %s\n", version.build_commit);
}

static void run_platform_diagnostic(int json) {
    orbisjvm_platform_info_t info;
    if (orbisjvm_get_platform_info(&info) < 0) {
        printf("Failed to get platform info\n");
        return;
    }

    if (json) {
        printf("{\n");
        printf("  \"platform\": \"%s\",\n", info.platform_name);
        printf("  \"os\": \"%s\",\n", info.os_name);
        printf("  \"os_version\": \"%s\",\n", info.os_version);
        printf("  \"architecture\": \"%s\",\n", info.architecture);
        printf("  \"memory\": {\n");
        printf("    \"total_physical\": %llu,\n", info.memory.total_physical);
        printf("    \"available_physical\": %llu,\n", info.memory.available_physical);
        printf("    \"page_size\": %u\n", info.memory.page_size);
        printf("  },\n");
        printf("  \"cpu\": {\n");
        printf("    \"vendor\": \"%s\",\n", info.cpu.vendor);
        printf("    \"model\": \"%s\",\n", info.cpu.model);
        printf("    \"core_count\": %u,\n", info.cpu.core_count);
        printf("    \"thread_count\": %u,\n", info.cpu.thread_count);
        printf("    \"frequency_hz\": %llu\n", info.cpu.frequency_hz);
        printf("  }\n");
        printf("}\n");
    } else {
        printf("=== Platform Information ===\n");
        printf("Platform: %s\n", info.platform_name);
        printf("OS: %s %s\n", info.os_name, info.os_version);
        printf("Architecture: %s\n", info.architecture);
        printf("\n");
        printf("Memory:\n");
        printf("  Total Physical: %llu MB\n", info.memory.total_physical / (1024*1024));
        printf("  Available Physical: %llu MB\n", info.memory.available_physical / (1024*1024));
        printf("  Page Size: %u bytes\n", info.memory.page_size);
        printf("\n");
        printf("CPU:\n");
        printf("  Vendor: %s\n", info.cpu.vendor);
        printf("  Model: %s\n", info.cpu.model);
        printf("  Cores: %u\n", info.cpu.core_count);
        printf("  Threads: %u\n", info.cpu.thread_count);
        printf("  Frequency: %.2f GHz\n", info.cpu.frequency_hz / 1e9);
        printf("\n");
    }
}

static void run_capabilities_diagnostic(int json) {
    if (json) {
        printf("{\n");
        printf("  \"capabilities\": [\n");
    } else {
        printf("=== Platform Capabilities ===\n");
    }

    struct {
        uint64_t cap;
        const char* name;
        const char* desc;
    } caps[] = {
        {ORBISJVM_CAP_MEMORY_BASIC, "MEMORY_BASIC", "Basic malloc/free support"},
        {ORBISJVM_CAP_MEMORY_PAGE_ALLOC, "MEMORY_PAGE_ALLOC", "Page-aligned allocation"},
        {ORBISJVM_CAP_MEMORY_PROTECT, "MEMORY_PROTECT", "Memory protection (mprotect)"},
        {ORBISJVM_CAP_MEMORY_MAP, "MEMORY_MAP", "Memory mapping (mmap)"},
        {ORBISJVM_CAP_THREADING_BASIC, "THREADING_BASIC", "Thread creation/join"},
        {ORBISJVM_CAP_THREADING_MUTEX, "THREADING_MUTEX", "Mutex support"},
        {ORBISJVM_CAP_THREADING_CONDVAR, "THREADING_CONDVAR", "Condition variable support"},
        {ORBISJVM_CAP_THREADING_TLS, "THREADING_TLS", "Thread-local storage"},
        {ORBISJVM_CAP_FILESYSTEM_BASIC, "FILESYSTEM_BASIC", "File I/O operations"},
        {ORBISJVM_CAP_FILESYSTEM_DIR, "FILESYSTEM_DIR", "Directory operations"},
        {ORBISJVM_CAP_FILESYSTEM_META, "FILESYSTEM_META", "File metadata (stat)"},
        {ORBISJVM_CAP_NETWORK_TCP, "NETWORK_TCP", "TCP socket support"},
        {ORBISJVM_CAP_NETWORK_UDP, "NETWORK_UDP", "UDP socket support"},
        {ORBISJVM_CAP_NETWORK_DNS, "NETWORK_DNS", "DNS resolution"},
        {ORBISJVM_CAP_TIME_MONOTONIC, "TIME_MONOTONIC", "Monotonic clock"},
        {ORBISJVM_CAP_TIME_WALLCLOCK, "TIME_WALLCLOCK", "Wall clock time"},
        {ORBISJVM_CAP_TIME_HR_TIMER, "TIME_HR_TIMER", "High-resolution timers"},
        {ORBISJVM_CAP_ENV_VARS, "ENV_VARS", "Environment variables"},
        {ORBISJVM_CAP_PROCESS_INFO, "PROCESS_INFO", "Process information"},
        {ORBISJVM_CAP_JNI_BASIC, "JNI_BASIC", "Basic JNI support"},
        {ORBISJVM_CAP_TLS, "TLS", "TLS/SSL support"},
    };

    for (size_t i = 0; i < sizeof(caps)/sizeof(caps[0]); i++) {
        int has = orbisjvm_has_capability(caps[i].cap);
        if (json) {
            printf("    {\"name\": \"%s\", \"description\": \"%s\", \"available\": %s}%s\n",
                   caps[i].name, caps[i].desc, has ? "true" : "false",
                   i == sizeof(caps)/sizeof(caps[0]) - 1 ? "" : ",");
        } else {
            printf("  [%c] %s - %s\n", has ? 'X' : ' ', caps[i].name, caps[i].desc);
        }
    }

    if (json) {
        printf("  ]\n");
        printf("}\n");
    }
}

static void run_memory_diagnostic(int json) {
    size_t current, total, peak;
    uint64_t allocs, frees;
    orbisjvm_get_allocation_stats(&current, &total, &peak, &allocs, &frees);

    uint64_t sys_total, sys_avail, sys_used;
    orbisjvm_get_memory_stats(&sys_total, &sys_avail, &sys_used);

    if (json) {
        printf("{\n");
        printf("  \"system\": {\n");
        printf("    \"total\": %llu,\n", sys_total);
        printf("    \"available\": %llu,\n", sys_avail);
        printf("    \"used\": %llu\n", sys_used);
        printf("  },\n");
        printf("  \"allocator\": {\n");
        printf("    \"current\": %zu,\n", current);
        printf("    \"total\": %zu,\n", total);
        printf("    \"peak\": %zu,\n", peak);
        printf("    \"allocations\": %llu,\n", allocs);
        printf("    \"frees\": %llu,\n", frees);
        printf("    \"leaks\": %llu\n", allocs - frees);
        printf("  }\n");
        printf("}\n");
    } else {
        printf("=== Memory Information ===\n");
        printf("System:\n");
        printf("  Total: %llu MB\n", sys_total / (1024*1024));
        printf("  Available: %llu MB\n", sys_avail / (1024*1024));
        printf("  Used: %llu MB\n", sys_used / (1024*1024));
        printf("\n");
        printf("Allocator:\n");
        printf("  Current Allocations: %zu\n", current);
        printf("  Total Allocated: %zu bytes\n", total);
        printf("  Peak Allocated: %zu bytes\n", peak);
        printf("  Total Allocations: %llu\n", allocs);
        printf("  Total Frees: %llu\n", frees);
        printf("  Potential Leaks: %llu\n", allocs - frees);
        printf("\n");
    }
}

static void run_threading_diagnostic(int json) {
    orbisjvm_thread_stats_t stats;
    orbisjvm_get_thread_stats(&stats);

    if (json) {
        printf("{\n");
        printf("  \"threads_created\": %llu,\n", stats.threads_created);
        printf("  \"threads_destroyed\": %llu,\n", stats.threads_destroyed);
        printf("  \"mutexes_created\": %llu,\n", stats.mutexes_created);
        printf("  \"mutexes_destroyed\": %llu,\n", stats.mutexes_destroyed);
        printf("  \"condvars_created\": %llu,\n", stats.condvars_created);
        printf("  \"condvars_destroyed\": %llu,\n", stats.condvars_destroyed);
        printf("  \"current_thread_count\": %u,\n", stats.current_thread_count);
        printf("  \"peak_thread_count\": %u\n", stats.peak_thread_count);
        printf("}\n");
    } else {
        printf("=== Threading Information ===\n");
        printf("  Threads Created: %llu\n", stats.threads_created);
        printf("  Threads Destroyed: %llu\n", stats.threads_destroyed);
        printf("  Mutexes Created: %llu\n", stats.mutexes_created);
        printf("  Mutexes Destroyed: %llu\n", stats.mutexes_destroyed);
        printf("  Condvars Created: %llu\n", stats.condvars_created);
        printf("  Condvars Destroyed: %llu\n", stats.condvars_destroyed);
        printf("  Current Threads: %u\n", stats.current_thread_count);
        printf("  Peak Threads: %u\n", stats.peak_thread_count);
        printf("\n");
    }
}

static void run_filesystem_diagnostic(int json) {
    char cwd[256];
    char data_dir[256];
    char cache_dir[256];
    char config_dir[256];
    char temp_dir[256];

    orbisjvm_get_cwd(cwd, sizeof(cwd));
    orbisjvm_get_app_data_dir(data_dir, sizeof(data_dir));
    orbisjvm_get_app_cache_dir(cache_dir, sizeof(cache_dir));
    orbisjvm_get_app_config_dir(config_dir, sizeof(config_dir));
    orbisjvm_get_temp_dir(temp_dir, sizeof(temp_dir));

    if (json) {
        printf("{\n");
        printf("  \"cwd\": \"%s\",\n", cwd);
        printf("  \"data_dir\": \"%s\",\n", data_dir);
        printf("  \"cache_dir\": \"%s\",\n", cache_dir);
        printf("  \"config_dir\": \"%s\",\n", config_dir);
        printf("  \"temp_dir\": \"%s\"\n", temp_dir);
        printf("}\n");
    } else {
        printf("=== Filesystem Information ===\n");
        printf("  Current Directory: %s\n", cwd);
        printf("  Data Directory: %s\n", data_dir);
        printf("  Cache Directory: %s\n", cache_dir);
        printf("  Config Directory: %s\n", config_dir);
        printf("  Temp Directory: %s\n", temp_dir);
        printf("\n");
    }
}

static void run_network_diagnostic(int json) {
    orbisjvm_net_stats_t stats;
    orbisjvm_get_net_stats(&stats);

    if (json) {
        printf("{\n");
        printf("  \"bytes_sent\": %llu,\n", stats.bytes_sent);
        printf("  \"bytes_received\": %llu,\n", stats.bytes_received);
        printf("  \"packets_sent\": %llu,\n", stats.packets_sent);
        printf("  \"packets_received\": %llu,\n", stats.packets_received);
        printf("  \"errors\": %llu,\n", stats.errors);
        printf("  \"sockets_created\": %u,\n", stats.sockets_created);
        printf("  \"sockets_closed\": %u,\n", stats.sockets_closed);
        printf("  \"current_sockets\": %u,\n", stats.current_sockets);
        printf("  \"peak_sockets\": %u\n", stats.peak_sockets);
        printf("}\n");
    } else {
        printf("=== Network Information ===\n");
        printf("  Bytes Sent: %llu\n", stats.bytes_sent);
        printf("  Bytes Received: %llu\n", stats.bytes_received);
        printf("  Packets Sent: %llu\n", stats.packets_sent);
        printf("  Packets Received: %llu\n", stats.packets_received);
        printf("  Errors: %llu\n", stats.errors);
        printf("  Sockets Created: %u\n", stats.sockets_created);
        printf("  Sockets Closed: %u\n", stats.sockets_closed);
        printf("  Current Sockets: %u\n", stats.current_sockets);
        printf("  Peak Sockets: %u\n", stats.peak_sockets);
        printf("\n");
    }
}

static void run_time_diagnostic(int json) {
    orbisjvm_time_stats_t stats;
    orbisjvm_get_time_stats(&stats);

    uint64_t now_ns = orbisjvm_get_monotonic_ns();
    uint64_t wall_ns = orbisjvm_get_walltime_ns();

    if (json) {
        printf("{\n");
        printf("  \"monotonic_ns\": %llu,\n", now_ns);
        printf("  \"walltime_ns\": %llu,\n", wall_ns);
        printf("  \"monotonic_freq_hz\": %llu,\n", stats.monotonic_freq_hz);
        printf("  \"monotonic_is_steady\": %s,\n", stats.monotonic_is_steady ? "true" : "false");
        printf("  \"timer_resolution_ns\": %llu\n", stats.timer_resolution_ns);
        printf("}\n");
    } else {
        printf("=== Time Information ===\n");
        printf("  Monotonic Time: %llu ns (%.3f s)\n", now_ns, now_ns / 1e9);
        printf("  Wall Clock Time: %llu ns (%.3f s)\n", wall_ns, wall_ns / 1e9);
        printf("  Monotonic Frequency: %llu Hz\n", stats.monotonic_freq_hz);
        printf("  Monotonic Steady: %s\n", stats.monotonic_is_steady ? "yes" : "no");
        printf("  Timer Resolution: %llu ns\n", stats.timer_resolution_ns);
        printf("\n");
    }
}

static void run_environment_diagnostic(int json) {
    orbisjvm_system_info_t sys;
    orbisjvm_cpu_info_t cpu;
    orbisjvm_app_paths_t paths;

    orbisjvm_get_system_info(&sys);
    orbisjvm_get_cpu_info(&cpu);
    orbisjvm_get_app_paths(&paths);

    if (json) {
        printf("{\n");
        printf("  \"system\": {\n");
        printf("    \"os_name\": \"%s\",\n", sys.os_name);
        printf("    \"os_version\": \"%s\",\n", sys.os_version);
        printf("    \"kernel_version\": \"%s\",\n", sys.kernel_version);
        printf("    \"architecture\": \"%s\",\n", sys.architecture);
        printf("    \"cpu_count\": %u,\n", sys.cpu_count);
        printf("    \"total_memory\": %llu,\n", sys.total_memory);
        printf("    \"available_memory\": %llu,\n", sys.available_memory);
        printf("    \"page_size\": %u\n", sys.page_size);
        printf("  },\n");
        printf("  \"cpu\": {\n");
        printf("    \"vendor\": \"%s\",\n", cpu.vendor);
        printf("    \"model\": \"%s\",\n", cpu.model);
        printf("    \"frequency_hz\": %llu\n", cpu.frequency_hz);
        printf("  },\n");
        printf("  \"paths\": {\n");
        printf("    \"exe_path\": \"%s\",\n", paths.exe_path);
        printf("    \"data_dir\": \"%s\",\n", paths.data_dir);
        printf("    \"config_dir\": \"%s\",\n", paths.config_dir);
        printf("    \"cache_dir\": \"%s\",\n", paths.cache_dir);
        printf("    \"temp_dir\": \"%s\"\n", paths.temp_dir);
        printf("  }\n");
        printf("}\n");
    } else {
        printf("=== Environment Information ===\n");
        printf("System:\n");
        printf("  OS: %s %s\n", sys.os_name, sys.os_version);
        printf("  Kernel: %s\n", sys.kernel_version);
        printf("  Architecture: %s\n", sys.architecture);
        printf("  CPUs: %u\n", sys.cpu_count);
        printf("  Total Memory: %llu MB\n", sys.total_memory / (1024*1024));
        printf("  Available Memory: %llu MB\n", sys.available_memory / (1024*1024));
        printf("  Page Size: %u bytes\n", sys.page_size);
        printf("\n");
        printf("CPU:\n");
        printf("  Vendor: %s\n", cpu.vendor);
        printf("  Model: %s\n", cpu.model);
        printf("  Frequency: %.2f GHz\n", cpu.frequency_hz / 1e9);
        printf("\n");
        printf("Paths:\n");
        printf("  Executable: %s\n", paths.exe_path);
        printf("  Data: %s\n", paths.data_dir);
        printf("  Config: %s\n", paths.config_dir);
        printf("  Cache: %s\n", paths.cache_dir);
        printf("  Temp: %s\n", paths.temp_dir);
        printf("\n");
    }
}

int main(int argc, char** argv) {
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"all", no_argument, 0, 'a'},
        {"platform", no_argument, 0, 'p'},
        {"capabilities", no_argument, 0, 'c'},
        {"memory", no_argument, 0, 'm'},
        {"threading", no_argument, 0, 't'},
        {"filesystem", no_argument, 0, 'f'},
        {"network", no_argument, 0, 'n'},
        {"time", no_argument, 0, 'T'},
        {"environment", no_argument, 0, 'e'},
        {"report", required_argument, 0, 'r'},
        {"json", no_argument, 0, 'j'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    int run_all = 0;
    int run_platform = 0;
    int run_capabilities = 0;
    int run_memory = 0;
    int run_threading = 0;
    int run_filesystem = 0;
    int run_network = 0;
    int run_time = 0;
    int run_environment = 0;
    int json_output = 0;
    const char* report_file = NULL;

    while ((opt = getopt_long(argc, argv, "hvapcmftnTer:j", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'v':
                print_version();
                return 0;
            case 'a':
                run_all = 1;
                break;
            case 'p':
                run_platform = 1;
                break;
            case 'c':
                run_capabilities = 1;
                break;
            case 'm':
                run_memory = 1;
                break;
            case 't':
                run_threading = 1;
                break;
            case 'f':
                run_filesystem = 1;
                break;
            case 'n':
                run_network = 1;
                break;
            case 'T':
                run_time = 1;
                break;
            case 'e':
                run_environment = 1;
                break;
            case 'r':
                report_file = optarg;
                break;
            case 'j':
                json_output = 1;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    /* Initialize runtime */
    if (orbisjvm_init() < 0) {
        fprintf(stderr, "Failed to initialize OrbisJVM runtime\n");
        return 1;
    }

    /* If no specific options, run all */
    if (!run_all && !run_platform && !run_capabilities && !run_memory &&
        !run_threading && !run_filesystem && !run_network && !run_time && !run_environment) {
        run_all = 1;
    }

    if (run_all) {
        run_platform = run_capabilities = run_memory = run_threading =
        run_filesystem = run_network = run_time = run_environment = 1;
    }

    if (json_output && (run_platform || run_capabilities || run_memory ||
                        run_threading || run_filesystem || run_network ||
                        run_time || run_environment)) {
        printf("{\n");
    }

    int first = 1;
    if (run_platform) {
        if (json_output && !first) printf(",\n");
        first = 0;
        if (json_output) printf("  \"platform\": ");
        run_platform_diagnostic(json_output);
    }
    if (run_capabilities) {
        if (json_output && !first) printf(",\n");
        first = 0;
        if (json_output) printf("  \"capabilities\": ");
        run_capabilities_diagnostic(json_output);
    }
    if (run_memory) {
        if (json_output && !first) printf(",\n");
        first = 0;
        if (json_output) printf("  \"memory\": ");
        run_memory_diagnostic(json_output);
    }
    if (run_threading) {
        if (json_output && !first) printf(",\n");
        first = 0;
        if (json_output) printf("  \"threading\": ");
        run_threading_diagnostic(json_output);
    }
    if (run_filesystem) {
        if (json_output && !first) printf(",\n");
        first = 0;
        if (json_output) printf("  \"filesystem\": ");
        run_filesystem_diagnostic(json_output);
    }
    if (run_network) {
        if (json_output && !first) printf(",\n");
        first = 0;
        if (json_output) printf("  \"network\": ");
        run_network_diagnostic(json_output);
    }
    if (run_time) {
        if (json_output && !first) printf(",\n");
        first = 0;
        if (json_output) printf("  \"time\": ");
        run_time_diagnostic(json_output);
    }
    if (run_environment) {
        if (json_output && !first) printf(",\n");
        first = 0;
        if (json_output) printf("  \"environment\": ");
        run_environment_diagnostic(json_output);
    }

    if (json_output && (run_platform || run_capabilities || run_memory ||
                        run_threading || run_filesystem || run_network ||
                        run_time || run_environment)) {
        printf("\n}\n");
    }

    /* Generate full report if requested */
    if (report_file) {
        if (orbisjvm_generate_diagnostic_report(report_file) == 0) {
            printf("\nDiagnostic report saved to: %s\n", report_file);
        } else {
            fprintf(stderr, "Failed to generate diagnostic report\n");
        }
    }

    orbisjvm_shutdown();
    return 0;
}