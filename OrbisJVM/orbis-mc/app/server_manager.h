/**
 * @file server_manager.h
 * @brief OrbisMC Server Manager
 *
 * Manages Minecraft server process lifecycle: start, stop, restart, monitoring.
 */

#ifndef ORBISMC_SERVER_MANAGER_H
#define ORBISMC_SERVER_MANAGER_H

#include <orbisjvm/process.h>
#include <orbisjvm/environment.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct config_manager config_manager_t;
typedef struct world_manager world_manager_t;

/* Server state */
typedef enum {
    SERVER_STATE_STOPPED = 0,
    SERVER_STATE_STARTING,
    SERVER_STATE_RUNNING,
    SERVER_STATE_STOPPING,
    SERVER_STATE_CRASHED,
    SERVER_STATE_ERROR,
} server_state_t;

/* Server manager */
typedef struct server_manager server_manager_t;

/* Create server manager */
server_manager_t* server_manager_create(config_manager_t* config, world_manager_t* world_mgr);

/* Destroy server manager */
void server_manager_destroy(server_manager_t* mgr);

/* Start server */
int server_manager_start(server_manager_t* mgr);

/* Stop server gracefully */
int server_manager_stop(server_manager_t* mgr, int timeout_ms);

/* Restart server */
int server_manager_restart(server_manager_t* mgr);

/* Force kill server */
int server_manager_kill(server_manager_t* mgr);

/* Get current server state */
server_state_t server_manager_get_state(const server_manager_t* mgr);

/* Check if server is running */
bool server_manager_is_running(const server_manager_t* mgr);

/* Check if server has crashed */
bool server_manager_has_crashed(const server_manager_t* mgr);

/* Get server uptime in milliseconds */
uint64_t server_manager_get_uptime(const server_manager_t* mgr);

/* Get server PID */
uint32_t server_manager_get_pid(const server_manager_t* mgr);

/* Execute server command */
int server_manager_execute_command(server_manager_t* mgr, const char* command);

/* Get server console output (last N lines) */
int server_manager_get_console_output(const server_manager_t* mgr, char* buffer, size_t size, int lines);

/* Update server status (call periodically) */
void server_manager_update(server_manager_t* mgr);

/* Perform health check */
void server_manager_check_health(server_manager_t* mgr);

/* Get server statistics */
typedef struct {
    server_state_t state;
    uint64_t uptime_ms;
    uint32_t pid;
    uint64_t memory_used_mb;
    uint64_t cpu_percent;
    int player_count;
    int max_players;
    double tps;
    bool has_crashed;
    int restart_count;
    uint64_t last_crash_time;
} server_stats_t;

int server_manager_get_stats(const server_manager_t* mgr, server_stats_t* stats);

/* Server configuration */
typedef struct {
    char java_path[256];
    char server_jar[256];
    char server_dir[256];
    char world_name[64];
    size_t heap_max_mb;
    size_t heap_initial_mb;
    char java_flags[1024];
    int port;
    bool auto_restart;
    int restart_delay_sec;
    int graceful_stop_timeout_sec;
} server_config_t;

int server_manager_get_config(const server_manager_t* mgr, server_config_t* config);
int server_manager_set_config(server_manager_t* mgr, const server_config_t* config);

/* Callback for server output */
typedef void (*server_output_callback_t)(const char* line, void* user_data);
void server_manager_set_output_callback(server_manager_t* mgr, server_output_callback_t callback, void* user_data);

/* Callback for state changes */
typedef void (*server_state_callback_t)(server_state_t old_state, server_state_t new_state, void* user_data);
void server_manager_set_state_callback(server_manager_t* mgr, server_state_callback_t callback, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* ORBISMC_SERVER_MANAGER_H */