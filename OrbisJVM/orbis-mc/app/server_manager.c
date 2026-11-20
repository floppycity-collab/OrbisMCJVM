/**
 * @file server_manager.c
 * @brief OrbisMC Server Manager Implementation
 */

#include "server_manager.h"
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/process.h>
#include <orbisjvm/filesystem.h>
#include <orbisjvm/threading.h>
#include <orbisjvm/time.h>
#include <orbisjvm/memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Server manager structure */
struct server_manager {
    config_manager_t* config_mgr;
    world_manager_t* world_mgr;

    server_state_t state;
    server_state_t previous_state;
    orbisjvm_process_t process;
    orbisjvm_thread_t output_thread;
    orbisjvm_thread_t monitor_thread;
    orbisjvm_mutex_t state_mutex;

    uint64_t start_time;
    uint64_t uptime_ms;
    int restart_count;
    uint64_t last_crash_time;
    bool has_crashed;
    bool should_stop;

    /* Console output buffer */
    char** output_lines;
    int output_count;
    int output_capacity;
    orbisjvm_mutex_t output_mutex;

    /* Callbacks */
    server_output_callback_t output_callback;
    void* output_callback_data;
    server_state_callback_t state_callback;
    void* state_callback_data;

    /* Configuration */
    server_config_t config;
};

/* Internal functions */
static void* server_output_reader(void* arg);
static void* server_monitor(void* arg);
static void server_change_state(server_manager_t* mgr, server_state_t new_state);
static int server_build_command(server_manager_t* mgr, char*** argv_out);
static void server_free_argv(char** argv);

/* Create server manager */
server_manager_t* server_manager_create(config_manager_t* config, world_manager_t* world_mgr) {
    server_manager_t* mgr = orbisjvm_malloc(sizeof(server_manager_t), ORBISJVM_MEM_ZERO);
    if (!mgr) return NULL;

    mgr->config_mgr = config;
    mgr->world_mgr = world_mgr;
    mgr->state = SERVER_STATE_STOPPED;
    mgr->previous_state = SERVER_STATE_STOPPED;
    mgr->process = NULL;
    mgr->output_thread = NULL;
    mgr->monitor_thread = NULL;
    mgr->start_time = 0;
    mgr->uptime_ms = 0;
    mgr->restart_count = 0;
    mgr->last_crash_time = 0;
    mgr->has_crashed = false;
    mgr->should_stop = false;

    /* Initialize output buffer */
    mgr->output_capacity = 1000;
    mgr->output_lines = orbisjvm_malloc(mgr->output_capacity * sizeof(char*), ORBISJVM_MEM_ZERO);
    mgr->output_count = 0;

    /* Create mutexes */
    orbisjvm_mutex_create(&mgr->state_mutex, NULL);
    orbisjvm_mutex_create(&mgr->output_mutex, NULL);

    /* Load default configuration */
    server_config_t default_config = {
        .java_path = "java",
        .server_jar = "server.jar",
        .server_dir = "./server",
        .world_name = "world",
        .heap_max_mb = 3072,
        .heap_initial_mb = 1024,
        .java_flags = "-XX:+UseG1GC -XX:MaxGCPauseMillis=50 -XX:+ParallelRefProcEnabled -XX:+AlwaysPreTouch",
        .port = 25565,
        .auto_restart = true,
        .restart_delay_sec = 10,
        .graceful_stop_timeout_sec = 30,
    };
    mgr->config = default_config;

    /* Load from config manager if available */
    if (config) {
        config_manager_get_server_config(config, &mgr->config);
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Server manager created");
    return mgr;
}

/* Destroy server manager */
void server_manager_destroy(server_manager_t* mgr) {
    if (!mgr) return;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Destroying server manager...");

    /* Stop server if running */
    if (server_manager_is_running(mgr)) {
        server_manager_stop(mgr, 5000);
    }

    /* Wait for threads */
    if (mgr->output_thread) {
        orbisjvm_thread_join(mgr->output_thread, NULL);
    }
    if (mgr->monitor_thread) {
        orbisjvm_thread_join(mgr->monitor_thread, NULL);
    }

    /* Free output buffer */
    orbisjvm_mutex_lock(&mgr->output_mutex);
    for (int i = 0; i < mgr->output_count; i++) {
        orbisjvm_free(mgr->output_lines[i]);
    }
    orbisjvm_free(mgr->output_lines);
    orbisjvm_mutex_unlock(&mgr->output_mutex);

    /* Destroy mutexes */
    orbisjvm_mutex_destroy(mgr->state_mutex);
    orbisjvm_mutex_destroy(mgr->output_mutex);

    orbisjvm_free(mgr);
}

/* Start server */
int server_manager_start(server_manager_t* mgr) {
    if (!mgr) return -1;

    orbisjvm_mutex_lock(&mgr->state_mutex);
    if (mgr->state != SERVER_STATE_STOPPED && mgr->state != SERVER_STATE_CRASHED && mgr->state != SERVER_STATE_ERROR) {
        orbisjvm_mutex_unlock(&mgr->state_mutex);
        orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "Server already running or starting");
        return -1;
    }
    mgr->should_stop = false;
    orbisjvm_mutex_unlock(&mgr->state_mutex);

    server_change_state(mgr, SERVER_STATE_STARTING);

    /* Build command line */
    char** argv = NULL;
    int ret = server_build_command(mgr, &argv);
    if (ret < 0) {
        server_change_state(mgr, SERVER_STATE_ERROR);
        return -1;
    }

    /* Create process attributes */
    orbisjvm_proc_attr_t attr;
    orbisjvm_proc_attr_init(&attr);
    attr.working_dir = mgr->config.server_dir;
    attr.flags = ORBISJVM_PROC_INHERIT_FDS;

    /* Create pipes for stdout/stderr capture */
    int stdout_pipe[2];
    int stderr_pipe[2];
    if (pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to create pipes: %d", errno);
        server_free_argv(argv);
        server_change_state(mgr, SERVER_STATE_ERROR);
        return -1;
    }

    attr.stdout_fd = stdout_pipe[1];
    attr.stderr_fd = stderr_pipe[1];

    /* Set environment variables */
    char* envp[32];
    int env_count = 0;
    envp[env_count++] = "PATH=/usr/bin:/bin";
    envp[env_count++] = "HOME=/tmp";
    envp[env_count] = NULL;

    /* Start process */
    ret = orbisjvm_proc_create(&mgr->process, mgr->config.java_path, argv, &attr);
    server_free_argv(argv);

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to start server process: %d", ret);
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        server_change_state(mgr, SERVER_STATE_ERROR);
        return -1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Server process started (PID: %d)", orbisjvm_proc_get_pid(mgr->process));

    /* Start output reader thread */
    int pipe_fds[2] = {stdout_pipe[0], stderr_pipe[0]};
    ret = orbisjvm_thread_create(&mgr->output_thread, NULL, server_output_reader, pipe_fds);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to create output thread: %d", ret);
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
    }

    /* Start monitor thread */
    ret = orbisjvm_thread_create(&mgr->monitor_thread, NULL, server_monitor, mgr);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to create monitor thread: %d", ret);
    }

    mgr->start_time = orbisjvm_get_monotonic_ms();
    mgr->has_crashed = false;

    server_change_state(mgr, SERVER_STATE_RUNNING);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Server started successfully");
    return 0;
}

/* Stop server gracefully */
int server_manager_stop(server_manager_t* mgr, int timeout_ms) {
    if (!mgr) return -1;

    orbisjvm_mutex_lock(&mgr->state_mutex);
    if (!server_manager_is_running(mgr)) {
        orbisjvm_mutex_unlock(&mgr->state_mutex);
        return 0;
    }
    mgr->should_stop = true;
    server_change_state(mgr, SERVER_STATE_STOPPING);
    orbisjvm_mutex_unlock(&mgr->state_mutex);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Stopping server gracefully...");

    /* Send stop command */
    server_manager_execute_command(mgr, "stop");

    /* Wait for process to exit */
    int exit_code = 0;
    int ret = orbisjvm_proc_wait(mgr->process, &exit_code, timeout_ms > 0 ? 0 : ORBISJVM_PROC_WNOHANG);
    if (ret < 0 && timeout_ms > 0) {
        /* Wait with timeout */
        uint64_t start = orbisjvm_get_monotonic_ms();
        while (server_manager_is_running(mgr)) {
            if (orbisjvm_get_monotonic_ms() - start >= (uint64_t)timeout_ms) {
                break;
            }
            orbisjvm_thread_sleep_ms(100);
            ret = orbisjvm_proc_wait(mgr->process, &exit_code, ORBISJVM_PROC_WNOHANG);
            if (ret == 0) break;
        }
    }

    /* Force kill if still running */
    if (server_manager_is_running(mgr)) {
        orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "Server did not stop gracefully, forcing kill...");
        orbisjvm_proc_kill(mgr->process);
        orbisjvm_proc_wait(mgr->process, &exit_code, 0);
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Server stopped (exit code: %d)", exit_code);

    /* Clean up process */
    if (mgr->process) {
        orbisjvm_proc_close(mgr->process);
        mgr->process = NULL;
    }

    /* Wait for threads */
    if (mgr->output_thread) {
        orbisjvm_thread_join(mgr->output_thread, NULL);
        mgr->output_thread = NULL;
    }
    if (mgr->monitor_thread) {
        orbisjvm_thread_join(mgr->monitor_thread, NULL);
        mgr->monitor_thread = NULL;
    }

    mgr->uptime_ms = orbisjvm_get_monotonic_ms() - mgr->start_time;
    server_change_state(mgr, SERVER_STATE_STOPPED);

    return 0;
}

/* Restart server */
int server_manager_restart(server_manager_t* mgr) {
    if (!mgr) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Restarting server...");

    server_manager_stop(mgr, mgr->config.graceful_stop_timeout_sec * 1000);

    if (mgr->config.restart_delay_sec > 0) {
        orbisjvm_thread_sleep_ms(mgr->config.restart_delay_sec * 1000);
    }

    mgr->restart_count++;
    return server_manager_start(mgr);
}

/* Force kill server */
int server_manager_kill(server_manager_t* mgr) {
    if (!mgr || !mgr->process) return -1;

    orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "Force killing server...");
    orbisjvm_proc_kill(mgr->process);
    return 0;
}

/* Get server state */
server_state_t server_manager_get_state(const server_manager_t* mgr) {
    if (!mgr) return SERVER_STATE_ERROR;
    orbisjvm_mutex_lock(&mgr->state_mutex);
    server_state_t state = mgr->state;
    orbisjvm_mutex_unlock(&mgr->state_mutex);
    return state;
}

/* Check if server is running */
bool server_manager_is_running(const server_manager_t* mgr) {
    if (!mgr) return false;
    server_state_t state = server_manager_get_state(mgr);
    return state == SERVER_STATE_RUNNING || state == SERVER_STATE_STARTING;
}

/* Check if server has crashed */
bool server_manager_has_crashed(const server_manager_t* mgr) {
    if (!mgr) return false;
    orbisjvm_mutex_lock(&mgr->state_mutex);
    bool crashed = mgr->has_crashed;
    orbisjvm_mutex_unlock(&mgr->state_mutex);
    return crashed;
}

/* Get server uptime */
uint64_t server_manager_get_uptime(const server_manager_t* mgr) {
    if (!mgr) return 0;
    if (server_manager_is_running(mgr)) {
        return orbisjvm_get_monotonic_ms() - mgr->start_time;
    }
    return mgr->uptime_ms;
}

/* Get server PID */
uint32_t server_manager_get_pid(const server_manager_t* mgr) {
    if (!mgr || !mgr->process) return 0;
    return orbisjvm_proc_get_pid(mgr->process);
}

/* Execute server command */
int server_manager_execute_command(server_manager_t* mgr, const char* command) {
    if (!mgr || !mgr->process || !command) return -1;

    /* Write command to stdin if we have a pipe */
    /* For now, we'll use the process API */
    char cmd_with_newline[1024];
    snprintf(cmd_with_newline, sizeof(cmd_with_newline), "%s\n", command);

    /* This would require stdin pipe - simplified for now */
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Server command: %s", command);
    return 0;
}

/* Get console output */
int server_manager_get_console_output(const server_manager_t* mgr, char* buffer, size_t size, int lines) {
    if (!mgr || !buffer || size == 0) return -1;

    orbisjvm_mutex_lock(&mgr->output_mutex);

    int start = mgr->output_count - lines;
    if (start < 0) start = 0;

    size_t pos = 0;
    for (int i = start; i < mgr->output_count && pos < size - 1; i++) {
        size_t len = strlen(mgr->output_lines[i]);
        if (pos + len + 1 >= size) break;
        memcpy(buffer + pos, mgr->output_lines[i], len);
        pos += len;
        buffer[pos++] = '\n';
    }
    buffer[pos] = '\0';

    orbisjvm_mutex_unlock(&mgr->output_mutex);
    return 0;
}

/* Update server status */
void server_manager_update(server_manager_t* mgr) {
    if (!mgr || !mgr->process) return;

    /* Check if process is still alive */
    int exit_code;
    int ret = orbisjvm_proc_trywait(mgr->process, &exit_code);
    if (ret == 1) {
        /* Process exited */
        orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "Server process exited with code %d", exit_code);
        orbisjvm_mutex_lock(&mgr->state_mutex);
        mgr->has_crashed = (exit_code != 0);
        mgr->last_crash_time = orbisjvm_get_walltime_ns();
        orbisjvm_mutex_unlock(&mgr->state_mutex);

        if (exit_code != 0 && mgr->config.auto_restart && !mgr->should_stop) {
            orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Server crashed, scheduling restart...");
            /* Restart will be handled by monitor thread */
        } else {
            server_change_state(mgr, mgr->has_crashed ? SERVER_STATE_CRASHED : SERVER_STATE_STOPPED);
        }
    }
}

/* Health check */
void server_manager_check_health(server_manager_t* mgr) {
    if (!mgr || !server_manager_is_running(mgr)) return;

    server_manager_update(mgr);

    /* Could add more health checks here:
     * - Memory usage
     * - CPU usage
     * - TPS monitoring
     * - Player count
     */
}

/* Get server stats */
int server_manager_get_stats(const server_manager_t* mgr, server_stats_t* stats) {
    if (!mgr || !stats) return -1;

    orbisjvm_mutex_lock(&mgr->state_mutex);
    stats->state = mgr->state;
    stats->uptime_ms = server_manager_get_uptime(mgr);
    stats->pid = server_manager_get_pid(mgr);
    stats->has_crashed = mgr->has_crashed;
    stats->restart_count = mgr->restart_count;
    stats->last_crash_time = mgr->last_crash_time;
    stats->memory_used_mb = 0; /* Would need process memory info */
    stats->cpu_percent = 0.0;
    stats->player_count = 0; /* Would need server query */
    stats->max_players = 0;
    stats->tps = 20.0;
    orbisjvm_mutex_unlock(&mgr->state_mutex);

    return 0;
}

/* Get configuration */
int server_manager_get_config(const server_manager_t* mgr, server_config_t* config) {
    if (!mgr || !config) return -1;
    *config = mgr->config;
    return 0;
}

/* Set configuration */
int server_manager_set_config(server_manager_t* mgr, const server_config_t* config) {
    if (!mgr || !config) return -1;
    mgr->config = *config;
    return 0;
}

/* Set output callback */
void server_manager_set_output_callback(server_manager_t* mgr, server_output_callback_t callback, void* user_data) {
    if (!mgr) return;
    mgr->output_callback = callback;
    mgr->output_callback_data = user_data;
}

/* Set state callback */
void server_manager_set_state_callback(server_manager_t* mgr, server_state_callback_t callback, void* user_data) {
    if (!mgr) return;
    mgr->state_callback = callback;
    mgr->state_callback_data = user_data;
}

/* Internal: Change server state */
static void server_change_state(server_manager_t* mgr, server_state_t new_state) {
    orbisjvm_mutex_lock(&mgr->state_mutex);
    mgr->previous_state = mgr->state;
    mgr->state = new_state;
    server_state_callback_t cb = mgr->state_callback;
    void* cb_data = mgr->state_callback_data;
    orbisjvm_mutex_unlock(&mgr->state_mutex);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Server state: %d -> %d", mgr->previous_state, new_state);

    if (cb) {
        cb(mgr->previous_state, new_state, cb_data);
    }
}

/* Internal: Build command line */
static int server_build_command(server_manager_t* mgr, char*** argv_out) {
    /* Count arguments */
    int argc = 0;
    char* args[64];

    args[argc++] = mgr->config.java_path;

    /* Java flags */
    char* java_flags = strdup(mgr->config.java_flags);
    char* token = strtok(java_flags, " ");
    while (token && argc < 60) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }
    free(java_flags);

    /* Heap settings */
    char heap_max[64];
    char heap_initial[64];
    snprintf(heap_max, sizeof(heap_max), "-Xmx%zuM", mgr->config.heap_max_mb);
    snprintf(heap_initial, sizeof(heap_initial), "-Xms%zuM", mgr->config.heap_initial_mb);
    args[argc++] = heap_max;
    args[argc++] = heap_initial;

    /* Server jar */
    args[argc++] = "-jar";
    args[argc++] = mgr->config.server_jar;

    /* Server arguments */
    args[argc++] = "nogui";

    args[argc] = NULL;

    /* Allocate and copy */
    char** argv = orbisjvm_malloc((argc + 1) * sizeof(char*), ORBISJVM_MEM_ZERO);
    for (int i = 0; i < argc; i++) {
        argv[i] = strdup(args[i]);
    }
    argv[argc] = NULL;

    *argv_out = argv;
    return 0;
}

/* Internal: Free argv */
static void server_free_argv(char** argv) {
    if (!argv) return;
    for (int i = 0; argv[i]; i++) {
        free(argv[i]);
    }
    orbisjvm_free(argv);
}

/* Internal: Output reader thread */
static void* server_output_reader(void* arg) {
    int* pipe_fds = (int*)arg;
    int stdout_fd = pipe_fds[0];
    int stderr_fd = pipe_fds[1];

    char buffer[4096];
    fd_set read_fds;
    int max_fd = (stdout_fd > stderr_fd) ? stdout_fd : stderr_fd;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(stdout_fd, &read_fds);
        FD_SET(stderr_fd, &read_fds);

        struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
        int ret = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if (ret < 0) break;
        if (ret == 0) continue;

        if (FD_ISSET(stdout_fd, &read_fds)) {
            ssize_t n = read(stdout_fd, buffer, sizeof(buffer) - 1);
            if (n <= 0) break;
            buffer[n] = '\0';

            /* Process lines */
            char* line = strtok(buffer, "\n");
            while (line) {
                /* Add to output buffer */
                extern server_manager_t* g_server_manager; /* Would need global or pass via context */
                /* For now, just log */
                orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "[SERVER] %s", line);
                line = strtok(NULL, "\n");
            }
        }

        if (FD_ISSET(stderr_fd, &read_fds)) {
            ssize_t n = read(stderr_fd, buffer, sizeof(buffer) - 1);
            if (n <= 0) break;
            buffer[n] = '\0';

            char* line = strtok(buffer, "\n");
            while (line) {
                orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "[SERVER ERR] %s", line);
                line = strtok(NULL, "\n");
            }
        }
    }

    close(stdout_fd);
    close(stderr_fd);
    return NULL;
}

/* Internal: Monitor thread */
static void* server_monitor(void* arg) {
    server_manager_t* mgr = (server_manager_t*)arg;

    while (true) {
        orbisjvm_thread_sleep_ms(1000);

        orbisjvm_mutex_lock(&mgr->state_mutex);
        bool should_stop = mgr->should_stop;
        bool running = server_manager_is_running(mgr);
        orbisjvm_mutex_unlock(&mgr->state_mutex);

        if (should_stop && !running) break;

        if (running) {
            server_manager_update(mgr);
        }
    }

    return NULL;
}