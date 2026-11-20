/**
 * @file remote_api.h
 * @brief OrbisMC Remote API
 *
 * REST API for remote server management via HTTP/HTTPS.
 */

#ifndef ORBISMC_REMOTE_API_H
#define ORBISMC_REMOTE_API_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct server_manager server_manager_t;
typedef struct console console_t;
typedef struct player_manager player_manager_t;
typedef struct config_manager config_manager_t;

/* Remote API */
typedef struct remote_api remote_api_t;

/* Create remote API */
remote_api_t* remote_api_create(config_manager_t* config, server_manager_t* server, console_t* console, player_manager_t* player_mgr);

/* Destroy remote API */
void remote_api_destroy(remote_api_t* api);

/* Start API server */
int remote_api_start(remote_api_t* api);

/* Stop API server */
int remote_api_stop(remote_api_t* api);

/* Check if running */
bool remote_api_is_running(const remote_api_t* api);

/* Get bound port */
int remote_api_get_port(const remote_api_t* api);

/* Set authentication token */
void remote_api_set_token(remote_api_t* api, const char* token);

/* API Endpoints: */

/* GET /api/v1/status - Server status */
typedef struct {
    bool running;
    char state[32];
    uint64_t uptime_ms;
    int players_online;
    int players_max;
    double tps;
    uint64_t memory_used_mb;
    uint64_t memory_max_mb;
    double cpu_percent;
    char version[64];
} api_status_t;

/* GET /api/v1/players - Player list */
typedef struct {
    char name[64];
    char uuid[64];
    int ping;
    bool online;
    bool op;
    int gamemode;
} api_player_t;

/* POST /api/v1/command - Execute command */
typedef struct {
    char command[512];
} api_command_request_t;

typedef struct {
    bool success;
    char output[2048];
} api_command_response_t;

/* POST /api/v1/player/kick - Kick player */
typedef struct {
    char name[64];
    char reason[256];
} api_kick_request_t;

/* POST /api/v1/player/ban - Ban player */
typedef struct {
    char name[64];
    char reason[256];
} api_ban_request_t;

/* POST /api/v1/player/teleport - Teleport player */
typedef struct {
    char name[64];
    double x, y, z;
} api_teleport_request_t;

/* GET /api/v1/worlds - World list */
typedef struct {
    char name[128];
    uint64_t size_bytes;
    uint64_t last_modified;
} api_world_t;

/* POST /api/v1/world/backup - Backup world */
typedef struct {
    char name[128];
} api_backup_request_t;

/* GET /api/v1/console - Console output */
typedef struct {
    char lines[4096];
    int count;
} api_console_t;

/* POST /api/v1/server/start - Start server */
typedef struct {
    bool success;
    char message[256];
} api_start_response_t;

/* POST /api/v1/server/stop - Stop server */
typedef struct {
    bool success;
    char message[256];
} api_stop_response_t;

/* POST /api/v1/server/restart - Restart server */
typedef struct {
    bool success;
    char message[256];
} api_restart_response_t;

/* WebSocket for real-time updates */
void* remote_api_websocket_thread(void* arg);

/* Rate limiting */
void remote_api_set_rate_limit(remote_api_t* api, int requests_per_minute);

/* CORS */
void remote_api_enable_cors(remote_api_t* api, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* ORBISMC_REMOTE_API_H */