/**
 * @file config_manager.h
 * @brief OrbisMC Configuration Manager
 *
 * Manages all configuration for the server and application.
 */

#ifndef ORBISMC_CONFIG_MANAGER_H
#define ORBISMC_CONFIG_MANAGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct config_manager config_manager_t;

/* Create config manager */
config_manager_t* config_manager_create(void);

/* Destroy config manager */
void config_manager_destroy(config_manager_t* mgr);

/* Load configuration from file */
int config_manager_load(config_manager_t* mgr);

/* Save configuration to file */
int config_manager_save(config_manager_t* mgr);

/* Create default configuration */
void config_manager_create_defaults(config_manager_t* mgr);

/* Get config file path */
int config_manager_get_config_path(config_manager_t* mgr, char* buffer, size_t size);

/* Server configuration */
typedef struct {
    char java_path[256];
    char server_jar[256];
    char server_dir[512];
    char world_name[64];
    size_t heap_max_mb;
    size_t heap_initial_mb;
    char java_flags[1024];
    int port;
    bool auto_start;
    bool auto_restart;
    int restart_delay_sec;
    int graceful_stop_timeout_sec;
    char server_properties[4096];
    char paper_yml[8192];
} server_config_t;

int config_manager_get_server_config(config_manager_t* mgr, server_config_t* config);
int config_manager_set_server_config(config_manager_t* mgr, const server_config_t* config);

/* Application configuration */
typedef struct {
    char app_data_dir[512];
    char worlds_dir[512];
    char backups_dir[512];
    char logs_dir[512];
    int remote_api_port;
    bool remote_api_enabled;
    char remote_api_bind[64];
    bool remote_api_tls;
    char remote_api_cert[256];
    char remote_api_key[256];
    char admin_username[64];
    char admin_password_hash[128];
    int log_level;
    bool debug_mode;
} app_config_t;

int config_manager_get_app_config(config_manager_t* mgr, app_config_t* config);
int config_manager_set_app_config(config_manager_t* mgr, const app_config_t* config);

/* World directories */
int config_manager_get_worlds_dir(config_manager_t* mgr, char* buffer, size_t size);
int config_manager_set_worlds_dir(config_manager_t* mgr, const char* path);
int config_manager_get_backups_dir(config_manager_t* mgr, char* buffer, size_t size);
int config_manager_set_backups_dir(config_manager_t* mgr, const char* path);
int config_manager_get_logs_dir(config_manager_t* mgr, char* buffer, size_t size);

/* Auto-backup configuration */
typedef struct {
    bool enabled;
    int interval_minutes;
    int max_backups;
    char backup_dir[512];
} auto_backup_config_t;

int config_manager_get_auto_backup_config(config_manager_t* mgr, auto_backup_config_t* config);
int config_manager_set_auto_backup_config(config_manager_t* mgr, const auto_backup_config_t* config);

/* Remote API configuration */
int config_manager_get_remote_api_port(config_manager_t* mgr);
int config_manager_set_remote_api_port(config_manager_t* mgr, int port);
bool config_manager_get_remote_api_enabled(config_manager_t* mgr);
int config_manager_set_remote_api_enabled(config_manager_t* mgr, bool enabled);
int config_manager_get_remote_api_bind(config_manager_t* mgr, char* buffer, size_t size);
int config_manager_set_remote_api_bind(config_manager_t* mgr, const char* bind);
bool config_manager_get_remote_api_tls(config_manager_t* mgr);
int config_manager_set_remote_api_tls(config_manager_t* mgr, bool enabled);
int config_manager_get_remote_api_cert(config_manager_t* mgr, char* buffer, size_t size);
int config_manager_set_remote_api_cert(config_manager_t* mgr, const char* path);
int config_manager_get_remote_api_key(config_manager_t* mgr, char* buffer, size_t size);
int config_manager_set_remote_api_key(config_manager_t* mgr, const char* path);

/* Authentication */
int config_manager_get_admin_username(config_manager_t* mgr, char* buffer, size_t size);
int config_manager_set_admin_username(config_manager_t* mgr, const char* username);
int config_manager_get_admin_password_hash(config_manager_t* mgr, char* buffer, size_t size);
int config_manager_set_admin_password(config_manager_t* mgr, const char* password);
bool config_manager_verify_password(config_manager_t* mgr, const char* password);

/* Logging */
int config_manager_get_log_level(config_manager_t* mgr);
int config_manager_set_log_level(config_manager_t* mgr, int level);
bool config_manager_get_debug_mode(config_manager_t* mgr);
int config_manager_set_debug_mode(config_manager_t* mgr, bool enabled);

/* Minecraft server.properties helpers */
int config_manager_load_server_properties(config_manager_t* mgr, const char* path);
int config_manager_save_server_properties(config_manager_t* mgr, const char* path);
int config_manager_get_server_property(config_manager_t* mgr, const char* key, char* value, size_t size);
int config_manager_set_server_property(config_manager_t* mgr, const char* key, const char* value);

/* Paper/YAML configuration helpers */
int config_manager_load_paper_yml(config_manager_t* mgr, const char* path);
int config_manager_save_paper_yml(config_manager_t* mgr, const char* path);

/* Reset to defaults */
int config_manager_reset_to_defaults(config_manager_t* mgr);

/* Export/Import configuration */
int config_manager_export(config_manager_t* mgr, const char* path);
int config_manager_import(config_manager_t* mgr, const char* path);

#ifdef __cplusplus
}
#endif

#endif /* ORBISMC_CONFIG_MANAGER_H */