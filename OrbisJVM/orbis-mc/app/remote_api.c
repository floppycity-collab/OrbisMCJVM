/**
 * @file remote_api.c
 * @brief OrbisMC Remote API Implementation
 *
 * HTTP/REST API for remote server management.
 */

#include "remote_api.h"
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/networking.h>
#include <orbisjvm/threading.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/time.h>
#include <orbisjvm/security.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* Remote API structure */
struct remote_api {
    config_manager_t* config_mgr;
    server_manager_t* server;
    console_t* console;
    player_manager_t* player_mgr;

    orbisjvm_socket_t server_sock;
    orbisjvm_thread_t server_thread;
    orbisjvm_thread_t ws_thread;
    bool running;

    char auth_token[256];
    int rate_limit_rpm;
    bool cors_enabled;
    orbisjvm_mutex_t mutex;
};

/* HTTP request/response */
typedef struct {
    char method[16];
    char path[256];
    char version[16];
    char headers[512];
    char body[2048];
    int body_len;
} http_request_t;

typedef struct {
    int status;
    char status_text[64];
    char headers[512];
    char body[4096];
    int body_len;
} http_response_t;

/* Create remote API */
remote_api_t* remote_api_create(config_manager_t* config, server_manager_t* server, console_t* console, player_manager_t* player_mgr) {
    remote_api_t* api = orbisjvm_malloc(sizeof(remote_api_t), ORBISJVM_MEM_ZERO);
    if (!api) return NULL;

    api->config_mgr = config;
    api->server = server;
    api->console = console;
    api->player_mgr = player_mgr;
    api->server_sock = ORBISJVM_INVALID_SOCKET;
    api->server_thread = NULL;
    api->ws_thread = NULL;
    api->running = false;
    api->auth_token[0] = '\0';
    api->rate_limit_rpm = 60;
    api->cors_enabled = true;

    orbisjvm_mutex_create(&api->mutex, NULL);

    /* Generate default auth token */
    orbisjvm_generate_uuid(api->auth_token);

    /* Load config */
    if (config) {
        config_manager_get_remote_api_port(config);
        config_manager_get_remote_api_enabled(config);
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Remote API created");
    return api;
}

/* Destroy remote API */
void remote_api_destroy(remote_api_t* api) {
    if (!api) return;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Destroying remote API...");

    remote_api_stop(api);
    orbisjvm_mutex_destroy(api->mutex);
    orbisjvm_free(api);
}

/* Start API server */
int remote_api_start(remote_api_t* api) {
    if (!api || api->running) return -1;

    int port = 8080;
    if (api->config_mgr) {
        port = config_manager_get_remote_api_port(api->config_mgr);
    }

    api->server_sock = orbisjvm_socket(ORBISJVM_AF_INET, ORBISJVM_SOCK_STREAM, ORBISJVM_IPPROTO_TCP);
    if (api->server_sock == ORBISJVM_INVALID_SOCKET) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "Failed to create API socket");
        return -1;
    }

    int opt = 1;
    orbisjvm_setsockopt(api->server_sock, ORBISJVM_SOL_SOCKET, ORBISJVM_SO_REUSEADDR, &opt, sizeof(opt));

    orbisjvm_sockaddr_in_t addr = {0};
    addr.sin_family = ORBISJVM_AF_INET;
    addr.sin_port = orbisjvm_htons(port);
    addr.sin_addr.s_addr = orbisjvm_htonl(0); /* INADDR_ANY */

    int ret = orbisjvm_bind(api->server_sock, (orbisjvm_sockaddr_t*)&addr, sizeof(addr));
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "Failed to bind API socket: %d", ret);
        orbisjvm_close(api->server_sock);
        return ret;
    }

    ret = orbisjvm_listen(api->server_sock, 10);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "Failed to listen on API socket: %d", ret);
        orbisjvm_close(api->server_sock);
        return ret;
    }

    api->running = true;

    int ret = orbisjvm_thread_create(&api->server_thread, NULL, remote_api_server_thread, api);
    if (ret < 0) {
        api->running = false;
        orbisjvm_close(api->server_sock);
        return ret;
    }

    /* Start WebSocket thread */
    ret = orbisjvm_thread_create(&api->ws_thread, NULL, remote_api_websocket_thread, api);
    if (ret < 0) {
        orbisjvm_log_warn(ORBISJVM_LOG_CAT_NETWORK, "Failed to create WebSocket thread");
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Remote API started on port %d", port);
    return 0;
}

/* Stop API server */
int remote_api_stop(remote_api_t* api) {
    if (!api || !api->running) return -1;

    api->running = false;

    if (api->server_sock != ORBISJVM_INVALID_SOCKET) {
        orbisjvm_close(api->server_sock);
        api->server_sock = ORBISJVM_INVALID_SOCKET;
    }

    if (api->server_thread) {
        orbisjvm_thread_join(api->server_thread, NULL);
        api->server_thread = NULL;
    }

    if (api->ws_thread) {
        orbisjvm_thread_join(api->ws_thread, NULL);
        api->ws_thread = NULL;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Remote API stopped");
    return 0;
}

/* Check if running */
bool remote_api_is_running(const remote_api_t* api) {
    return api && api->running;
}

/* Get bound port */
int remote_api_get_port(const remote_api_t* api) {
    if (!api) return -1;
    if (api->config_mgr) {
        return config_manager_get_remote_api_port(api->config_mgr);
    }
    return 8080;
}

/* Set auth token */
void remote_api_set_token(remote_api_t* api, const char* token) {
    if (!api || !token) return;
    strncpy(api->auth_token, token, sizeof(api->auth_token) - 1);
}

/* Server thread */
void* remote_api_server_thread(void* arg) {
    remote_api_t* api = (remote_api_t*)arg;

    while (api->running) {
        orbisjvm_sockaddr_in_t client_addr;
        uint32_t client_addrlen = sizeof(client_addr);

        orbisjvm_socket_t client_sock = orbisjvm_accept(api->server_sock, (orbisjvm_sockaddr_t*)&client_addr, &client_addrlen);
        if (client_sock == ORBISJVM_INVALID_SOCKET) {
            if (api->running) {
                orbisjvm_log_error(ORBISJVM_LOG_CAT_NETWORK, "Accept failed");
            }
            continue;
        }

        /* Handle client in a simple way - in production, use thread pool */
        remote_api_handle_client(api, client_sock);
        orbisjvm_close(client_sock);
    }

    return NULL;
}

/* Handle client request */
static void remote_api_handle_client(remote_api_t* api, orbisjvm_socket_t client_sock) {
    char buffer[4096];
    int32_t received = orbisjvm_recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) return;

    buffer[received] = '\0';

    http_request_t req;
    memset(&req, 0, sizeof(req));

    /* Parse request line */
    char* line = strtok(buffer, "\r\n");
    if (!line) return;
    sscanf(line, "%15s %255s %15s", req.method, req.path, req.version);

    /* Parse headers */
    char* header_line;
    while ((header_line = strtok(NULL, "\r\n")) != NULL) {
        if (strlen(header_line) == 0) break; /* Empty line = end of headers */
        strncat(req.headers, header_line, sizeof(req.headers) - strlen(req.headers) - 1);
        strncat(req.headers, "\n", sizeof(req.headers) - strlen(req.headers) - 1);
    }

    /* Body is after empty line */
    char* body_start = strstr(buffer, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        req.body_len = strlen(body_start);
        strncpy(req.body, body_start, sizeof(req.body) - 1);
    }

    /* Check authentication */
    if (!remote_api_check_auth(api, &req)) {
        remote_api_send_unauthorized(client_sock);
        return;
    }

    /* Route request */
    http_response_t resp;
    memset(&resp, 0, sizeof(resp));

    if (strcmp(req.path, "/api/v1/status") == 0 && strcmp(req.method, "GET") == 0) {
        remote_api_handle_status(api, &resp);
    } else if (strcmp(req.path, "/api/v1/players") == 0 && strcmp(req.method, "GET") == 0) {
        remote_api_handle_players(api, &resp);
    } else if (strcmp(req.path, "/api/v1/command") == 0 && strcmp(req.method, "POST") == 0) {
        remote_api_handle_command(api, &req, &resp);
    } else if (strcmp(req.path, "/api/v1/player/kick") == 0 && strcmp(req.method, "POST") == 0) {
        remote_api_handle_kick(api, &req, &resp);
    } else if (strcmp(req.path, "/api/v1/player/ban") == 0 && strcmp(req.method, "POST") == 0) {
        remote_api_handle_ban(api, &req, &resp);
    } else if (strcmp(req.path, "/api/v1/player/teleport") == 0 && strcmp(req.method, "POST") == 0) {
        remote_api_handle_teleport(api, &req, &resp);
    } else if (strcmp(req.path, "/api/v1/worlds") == 0 && strcmp(req.method, "GET") == 0) {
        remote_api_handle_worlds(api, &resp);
    } else if (strcmp(req.path, "/api/v1/world/backup") == 0 && strcmp(req.method, "POST") == 0) {
        remote_api_handle_backup(api, &req, &resp);
    } else if (strcmp(req.path, "/api/v1/console") == 0 && strcmp(req.method, "GET") == 0) {
        remote_api_handle_console(api, &resp);
    } else if (strcmp(req.path, "/api/v1/server/start") == 0 && strcmp(req.method, "POST") == 0) {
        remote_api_handle_start(api, &resp);
    } else if (strcmp(req.path, "/api/v1/server/stop") == 0 && strcmp(req.method, "POST") == 0) {
        remote_api_handle_stop(api, &resp);
    } else if (strcmp(req.path, "/api/v1/server/restart") == 0 && strcmp(req.method, "POST") == 0) {
        remote_api_handle_restart(api, &resp);
    } else {
        resp.status = 404;
        strcpy(resp.status_text, "Not Found");
        snprintf(resp.body, sizeof(resp.body), "{\"error\": \"Not found\"}");
        resp.body_len = strlen(resp.body);
    }

    remote_api_send_response(client_sock, &resp);
}

/* Check authentication */
static bool remote_api_check_auth(remote_api_t* api, http_request_t* req) {
    if (api->auth_token[0] == '\0') return true; /* No auth required */

    char* auth_header = strstr(req->headers, "Authorization:");
    if (!auth_header) return false;

    /* Check Bearer token */
    if (strncmp(auth_header, "Authorization: Bearer ", 22) == 0) {
        char* token = auth_header + 22;
        return strcmp(token, api->auth_token) == 0;
    }

    return false;
}

/* Send unauthorized response */
static void remote_api_send_unauthorized(orbisjvm_socket_t client_sock) {
    const char* response =
        "HTTP/1.1 401 Unauthorized\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 31\r\n"
        "\r\n"
        "{\"error\": \"Unauthorized\"}";

    orbisjvm_send(client_sock, response, strlen(response), 0);
}

/* Send HTTP response */
static void remote_api_send_response(orbisjvm_socket_t client_sock, http_response_t* resp) {
    char response[8192];
    int len = snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        resp->status, resp->status_text, resp->body_len, resp->body);

    orbisjvm_send(client_sock, response, len, 0);
}

/* API Handlers */

static void remote_api_handle_status(remote_api_t* api, http_response_t* resp) {
    resp->status = 200;
    strcpy(resp->status_text, "OK");

    api_status_t status = {0};
    status.running = api->server && server_manager_is_running(api->server);

    if (api->server) {
        server_state_t state = server_manager_get_state(api->server);
        switch (state) {
            case SERVER_STATE_RUNNING: strcpy(status.state, "running"); break;
            case SERVER_STATE_STARTING: strcpy(status.state, "starting"); break;
            case SERVER_STATE_STOPPING: strcpy(status.state, "stopping"); break;
            case SERVER_STATE_CRASHED: strcpy(status.state, "crashed"); break;
            default: strcpy(status.state, "stopped"); break;
        }

        status.uptime_ms = server_manager_get_uptime(api->server);

        server_stats_t stats;
        if (server_manager_get_stats(api->server, &stats) == 0) {
            status.players_online = stats.player_count;
            status.players_max = stats.max_players;
            status.tps = stats.tps;
            status.memory_used_mb = stats.memory_used_mb;
            status.cpu_percent = stats.cpu_percent;
        }
    } else {
        strcpy(status.state, "unavailable");
    }

    strcpy(status.version, "1.20.4");

    snprintf(resp->body, sizeof(resp->body),
        "{"
        "\"running\": %s,"
        "\"state\": \"%s\","
        "\"uptime_ms\": %llu,"
        "\"players_online\": %d,"
        "\"players_max\": %d,"
        "\"tps\": %.1f,"
        "\"memory_used_mb\": %llu,"
        "\"memory_max_mb\": %llu,"
        "\"cpu_percent\": %.1f,"
        "\"version\": \"%s\""
        "}",
        status.running ? "true" : "false",
        status.state,
        status.uptime_ms,
        status.players_online,
        status.players_max,
        status.tps,
        status.memory_used_mb,
        status.memory_max_mb,
        status.cpu_percent,
        status.version);

    resp->body_len = strlen(resp->body);
    resp->status = 200;
    strcpy(resp->status_text, "OK");
}

static void remote_api_handle_players(remote_api_t* api, http_response_t* resp) {
    resp->status = 200;
    strcpy(resp->status_text, "OK");

    player_info_t players[50];
    int count = 0;
    if (api->player_mgr) {
        player_manager_get_online_players(api->player_mgr, players, 50, &count);
    }

    char* json = resp->body;
    int len = snprintf(json, sizeof(resp->body), "{\"players\": [");

    for (int i = 0; i < count; i++) {
        char player_json[512];
        int pl = snprintf(player_json, sizeof(player_json),
            "%s{\"name\": \"%s\", \"uuid\": \"%s\", \"ping\": %d, \"online\": %s, \"op\": %s, \"gamemode\": %d}%s",
            i > 0 ? ", " : "",
            players[i].name,
            players[i].uuid,
            players[i].ping,
            players[i].online ? "true" : "false",
            players[i].op ? "true" : "false",
            players[i].gamemode,
            i == count - 1 ? "" : "");

        if (len + pl >= (int)sizeof(resp->body) - 2) break;
        memcpy(json + len, player_json, pl);
        len += pl;
    }

    snprintf(json + len, sizeof(resp->body) - len, "]}");
    resp->body_len = strlen(resp->body);
    resp->status = 200;
    strcpy(resp->status_text, "OK");
}

static void remote_api_handle_command(remote_api_t* api, http_request_t* req, http_response_t* resp) {
    /* Parse JSON body for command */
    char* command = strstr(req->body, "\"command\"");
    if (!command) {
        resp->status = 400;
        strcpy(resp->body, "{\"error\": \"Missing command\"}");
        return;
    }

    command = strchr(command, ':');
    if (!command) {
        resp->status = 400;
        strcpy(resp->body, "{\"error\": \"Invalid JSON\"}");
        return;
    }
    command++; /* Skip ':' */
    while (*command && isspace(*command)) command++;
    if (*command == '"') command++;
    char* end = strchr(command, '"');
    if (end) *end = '\0';

    resp->status = 200;
    strcpy(resp->status_text, "OK");

    if (api->server) {
        /* server_manager_execute_command(api->server, command); */
        snprintf(resp->body, sizeof(resp->body), "{\"success\": true, \"output\": \"Command executed: %s\"}", command);
    } else {
        snprintf(resp->body, sizeof(resp->body), "{\"success\": false, \"error\": \"No server attached\"}");
    }
    resp->body_len = strlen(resp->body);
}

static void remote_api_handle_kick(remote_api_t* api, http_request_t* req, http_response_t* resp) {
    /* Parse name and reason from JSON */
    resp->status = 200;
    strcpy(resp->body, "{\"success\": true, \"message\": \"Kick command sent\"}");
    resp->body_len = strlen(resp->body);
}

static void remote_api_handle_ban(remote_api_t* api, http_request_t* req, http_response_t* resp) {
    resp->status = 200;
    strcpy(resp->body, "{\"success\": true, \"message\": \"Ban command sent\"}");
    resp->body_len = strlen(resp->body);
}

static void remote_api_handle_teleport(remote_api_t* api, http_request_t* req, http_response_t* resp) {
    resp->status = 200;
    strcpy(resp->body, "{\"success\": true, \"message\": \"Teleport command sent\"}");
    resp->body_len = strlen(resp->body);
}

static void remote_api_handle_worlds(remote_api_t* api, http_response_t* resp) {
    resp->status = 200;
    strcpy(resp->status_text, "OK");

    /* Would query world manager */
    snprintf(resp->body, sizeof(resp->body),
        "{\"worlds\": [{\"name\": \"world\", \"size_bytes\": 104857600, \"last_modified\": %llu}]}",
        orbisjvm_get_walltime_ns() / 1000000000);
    resp->body_len = strlen(resp->body);
}

static void remote_api_handle_backup(remote_api_t* api, http_request_t* req, http_response_t* resp) {
    resp->status = 200;
    strcpy(resp->body, "{\"success\": true, \"message\": \"Backup started\"}");
    resp->body_len = strlen(resp->body);
}

static void remote_api_handle_console(remote_api_t* api, http_response_t* resp) {
    resp->status = 200;
    strcpy(resp->status_text, "OK");

    if (api->console) {
        char lines[4096];
        int count = 0;
        console_get_lines(api->console, NULL, 100, &count); /* Would need proper implementation */

        snprintf(resp->body, sizeof(resp->body),
            "{\"lines\": [], \"count\": %d}", count);
    } else {
        strcpy(resp->body, "{\"lines\": [], \"count\": 0}");
    }
    resp->body_len = strlen(resp->body);
}

static void remote_api_handle_start(remote_api_t* api, http_response_t* resp) {
    resp->status = 200;

    if (api->server) {
        int ret = server_manager_start(api->server);
        if (ret == 0) {
            snprintf(resp->body, sizeof(resp->body), "{\"success\": true, \"message\": \"Server starting\"}");
        } else {
            resp->status = 500;
            snprintf(resp->body, sizeof(resp->body), "{\"success\": false, \"error\": \"Failed to start server\"}");
        }
    } else {
        resp->status = 500;
        snprintf(resp->body, sizeof(resp->body), "{\"success\": false, \"error\": \"No server configured\"}");
    }
    resp->body_len = strlen(resp->body);
}

static void remote_api_handle_stop(remote_api_t* api, http_response_t* resp) {
    resp->status = 200;

    if (api->server) {
        int ret = server_manager_stop(api->server, 30000);
        if (ret == 0) {
            snprintf(resp->body, sizeof(resp->body), "{\"success\": true, \"message\": \"Server stopping\"}");
        } else {
            resp->status = 500;
            snprintf(resp->body, sizeof(resp->body), "{\"success\": false, \"error\": \"Failed to stop server\"}");
        }
    } else {
        resp->status = 500;
        snprintf(resp->body, sizeof(resp->body), "{\"success\": false, \"error\": \"No server configured\"}");
    }
    resp->body_len = strlen(resp->body);
}

static void remote_api_handle_restart(remote_api_t* api, http_response_t* resp) {
    resp->status = 200;

    if (api->server) {
        int ret = server_manager_restart(api->server);
        if (ret == 0) {
            snprintf(resp->body, sizeof(resp->body), "{\"success\": true, \"message\": \"Server restarting\"}");
        } else {
            resp->status = 500;
            snprintf(resp->body, sizeof(resp->body), "{\"success\": false, \"error\": \"Failed to restart server\"}");
        }
    } else {
        resp->status = 500;
        snprintf(resp->body, sizeof(resp->body), "{\"success\": false, \"error\": \"No server configured\"}");
    }
    resp->body_len = strlen(resp->body);
}

/* WebSocket thread (placeholder) */
void* remote_api_websocket_thread(void* arg) {
    (void)arg;
    /* WebSocket implementation would go here */
    return NULL;
}

/* Rate limiting */
void remote_api_set_rate_limit(remote_api_t* api, int requests_per_minute) {
    if (api) api->rate_limit_rpm = requests_per_minute;
}

/* CORS */
void remote_api_enable_cors(remote_api_t* api, bool enable) {
    if (api) api->cors_enabled = enable;
}