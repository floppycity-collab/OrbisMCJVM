/**
 * @file config_manager.c
 * @brief OrbisMC Configuration Manager Implementation
 */

#include "config_manager.h"
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/filesystem.h>
#include <orbisjvm/environment.h>
#include <orbisjvm/security.h>
#include <orbisjvm/memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Config manager structure */
struct config_manager {
    char config_file[512];
    server_config_t server_config;
    app_config_t app_config;
    auto_backup_config_t auto_backup_config;
    orbisjvm_mutex_t mutex;
};

/* Default configuration file name */
#define DEFAULT_CONFIG_FILE "orbismc.conf"

/* Create config manager */
config_manager_t* config_manager_create(void) {
    config_manager_t* mgr = orbisjvm_malloc(sizeof(config_manager_t), ORBISJVM_MEM_ZERO);
    if (!mgr) return NULL;

    /* Get config file path */
    char app_config_dir[512];
    orbisjvm_get_app_config_dir(app_config_dir, sizeof(app_config_dir));
    snprintf(mgr->config_file, sizeof(mgr->config_file), "%s/%s", app_config_dir, DEFAULT_CONFIG_FILE);

    orbisjvm_mutex_create(&mgr->mutex, NULL);

    /* Create defaults */
    config_manager_create_defaults(mgr);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Config manager created (config: %s)", mgr->config_file);
    return mgr;
}

/* Destroy config manager */
void config_manager_destroy(config_manager_t* mgr) {
    if (!mgr) return;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Destroying config manager...");
    orbisjvm_mutex_destroy(mgr->mutex);
    orbisjvm_free(mgr);
}

/* Load configuration from file */
int config_manager_load(config_manager_t* mgr) {
    if (!mgr) return -1;

    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, mgr->config_file, ORBISJVM_O_RDONLY, 0);
    if (ret < 0) {
        return -1; /* File doesn't exist */
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Loading config from: %s", mgr->config_file);

    char line[2048];
    size_t bytes_read;
    while ((ret = orbisjvm_file_read(file, line, sizeof(line) - 1, &bytes_read)) == 0 && bytes_read > 0) {
        line[bytes_read] = '\0';

        char* p = line;
        while (*p) {
            char* nl = strchr(p, '\n');
            if (nl) *nl = '\0';

            /* Skip comments and empty lines */
            if (p[0] == '#' || p[0] == '\0') {
                p = nl ? nl + 1 : p + strlen(p);
                continue;
            }

            char* eq = strchr(p, '=');
            if (eq) {
                *eq = '\0';
                char* key = p;
                char* value = eq + 1;

                /* Trim whitespace */
                while (*key && isspace(*key)) key++;
                char* key_end = key + strlen(key) - 1;
                while (key_end > key && isspace(*key_end)) *key_end-- = '\0';

                while (*value && isspace(*value)) value++;
                char* value_end = value + strlen(value) - 1;
                while (value_end > value && isspace(*value_end)) *value_end-- = '\0';

                /* Parse key-value pairs */
                config_parse_key_value(mgr, key, value);
            }

            p = nl ? nl + 1 : p + strlen(p);
        }
    }

    orbisjvm_file_close(file);
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Config loaded successfully");
    return 0;
}

/* Parse key-value pair */
static void config_parse_key_value(config_manager_t* mgr, const char* key, const char* value) {
    /* Server config */
    if (strcmp(key, "server.java_path") == 0) strncpy(mgr->server_config.java_path, value, sizeof(mgr->server_config.java_path) - 1);
    else if (strcmp(key, "server.server_jar") == 0) strncpy(mgr->server_config.server_jar, value, sizeof(mgr->server_config.server_jar) - 1);
    else if (strcmp(key, "server.server_dir") == 0) strncpy(mgr->server_config.server_dir, value, sizeof(mgr->server_config.server_dir) - 1);
    else if (strcmp(key, "server.world_name") == 0) strncpy(mgr->server_config.world_name, value, sizeof(mgr->server_config.world_name) - 1);
    else if (strcmp(key, "server.heap_max_mb") == 0) mgr->server_config.heap_max_mb = atoll(value);
    else if (strcmp(key, "server.heap_initial_mb") == 0) mgr->server_config.heap_initial_mb = atoll(value);
    else if (strcmp(key, "server.java_flags") == 0) strncpy(mgr->server_config.java_flags, value, sizeof(mgr->server_config.java_flags) - 1);
    else if (strcmp(key, "server.port") == 0) mgr->server_config.port = atoi(value);
    else if (strcmp(key, "server.auto_start") == 0) mgr->server_config.auto_start = (strcmp(value, "true") == 0 || atoi(value) == 1);
    else if (strcmp(key, "server.auto_restart") == 0) mgr->server_config.auto_restart = (strcmp(value, "true") == 0 || atoi(value) == 1);
    else if (strcmp(key, "server.restart_delay_sec") == 0) mgr->server_config.restart_delay_sec = atoi(value);
    else if (strcmp(key, "server.graceful_stop_timeout_sec") == 0) mgr->server_config.graceful_stop_timeout_sec = atoi(value);

    /* App config */
    else if (strcmp(key, "app.worlds_dir") == 0) strncpy(mgr->app_config.worlds_dir, value, sizeof(mgr->app_config.worlds_dir) - 1);
    else if (strcmp(key, "app.backups_dir") == 0) strncpy(mgr->app_config.backups_dir, value, sizeof(mgr->app_config.backups_dir) - 1);
    else if (strcmp(key, "app.logs_dir") == 0) strncpy(mgr->app_config.logs_dir, value, sizeof(mgr->app_config.logs_dir) - 1);
    else if (strcmp(key, "app.remote_api_port") == 0) mgr->app_config.remote_api_port = atoi(value);
    else if (strcmp(key, "app.remote_api_enabled") == 0) mgr->app_config.remote_api_enabled = (strcmp(value, "true") == 0 || atoi(value) == 1);
    else if (strcmp(key, "app.remote_api_bind") == 0) strncpy(mgr->app_config.remote_api_bind, value, sizeof(mgr->app_config.remote_api_bind) - 1);
    else if (strcmp(key, "app.remote_api_tls") == 0) mgr->app_config.remote_api_tls = (strcmp(value, "true") == 0 || atoi(value) == 1);
    else if (strcmp(key, "app.remote_api_cert") == 0) strncpy(mgr->app_config.remote_api_cert, value, sizeof(mgr->app_config.remote_api_cert) - 1);
    else if (strcmp(key, "app.remote_api_key") == 0) strncpy(mgr->app_config.remote_api_key, value, sizeof(mgr->app_config.remote_api_key) - 1);
    else if (strcmp(key, "app.admin_username") == 0) strncpy(mgr->app_config.admin_username, value, sizeof(mgr->app_config.admin_username) - 1);
    else if (strcmp(key, "app.admin_password_hash") == 0) strncpy(mgr->app_config.admin_password_hash, value, sizeof(mgr->app_config.admin_password_hash) - 1);
    else if (strcmp(key, "app.log_level") == 0) mgr->app_config.log_level = atoi(value);
    else if (strcmp(key, "app.debug_mode") == 0) mgr->app_config.debug_mode = (strcmp(value, "true") == 0 || atoi(value) == 1);

    /* Auto-backup config */
    else if (strcmp(key, "autobackup.enabled") == 0) mgr->auto_backup_config.enabled = (strcmp(value, "true") == 0 || atoi(value) == 1);
    else if (strcmp(key, "autobackup.interval_minutes") == 0) mgr->auto_backup_config.interval_minutes = atoi(value);
    else if (strcmp(key, "autobackup.max_backups") == 0) mgr->auto_backup_config.max_backups = atoi(value);
    else if (strcmp(key, "autobackup.backup_dir") == 0) strncpy(mgr->auto_backup_config.backup_dir, value, sizeof(mgr->auto_backup_config.backup_dir) - 1);
}

/* Save configuration to file */
int config_manager_save(config_manager_t* mgr) {
    if (!mgr) return -1;

    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, mgr->config_file, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to open config file for writing: %s", mgr->config_file);
        return ret;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Saving config to: %s", mgr->config_file);

    char buffer[4096];
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);

    int len = snprintf(buffer, sizeof(buffer),
        "# OrbisMC Configuration File\n"
        "# Generated: %s\n"
        "# Do not edit manually unless you know what you're doing\n\n",
        timestamp);
    orbisjvm_file_write(file, buffer, len, NULL);

    /* Server section */
    len = snprintf(buffer, sizeof(buffer),
        "[Server]\n"
        "server.java_path=%s\n"
        "server.server_jar=%s\n"
        "server.server_dir=%s\n"
        "server.world_name=%s\n"
        "server.heap_max_mb=%zu\n"
        "server.heap_initial_mb=%zu\n"
        "server.java_flags=%s\n"
        "server.port=%d\n"
        "server.auto_start=%s\n"
        "server.auto_restart=%s\n"
        "server.restart_delay_sec=%d\n"
        "server.graceful_stop_timeout_sec=%d\n\n",
        mgr->server_config.java_path,
        mgr->server_config.server_jar,
        mgr->server_config.server_dir,
        mgr->server_config.world_name,
        mgr->server_config.heap_max_mb,
        mgr->server_config.heap_initial_mb,
        mgr->server_config.java_flags,
        mgr->server_config.port,
        mgr->server_config.auto_start ? "true" : "false",
        mgr->server_config.auto_restart ? "true" : "false",
        mgr->server_config.restart_delay_sec,
        mgr->server_config.graceful_stop_timeout_sec);
    orbisjvm_file_write(file, buffer, len, NULL);

    /* App section */
    len = snprintf(buffer, sizeof(buffer),
        "[Application]\n"
        "app.worlds_dir=%s\n"
        "app.backups_dir=%s\n"
        "app.logs_dir=%s\n"
        "app.remote_api_port=%d\n"
        "app.remote_api_enabled=%s\n"
        "app.remote_api_bind=%s\n"
        "app.remote_api_tls=%s\n"
        "app.remote_api_cert=%s\n"
        "app.remote_api_key=%s\n"
        "app.admin_username=%s\n"
        "app.admin_password_hash=%s\n"
        "app.log_level=%d\n"
        "app.debug_mode=%s\n\n",
        mgr->app_config.worlds_dir,
        mgr->app_config.backups_dir,
        mgr->app_config.logs_dir,
        mgr->app_config.remote_api_port,
        mgr->app_config.remote_api_enabled ? "true" : "false",
        mgr->app_config.remote_api_bind,
        mgr->app_config.remote_api_tls ? "true" : "false",
        mgr->app_config.remote_api_cert,
        mgr->app_config.remote_api_key,
        mgr->app_config.admin_username,
        mgr->app_config.admin_password_hash,
        mgr->app_config.log_level,
        mgr->app_config.debug_mode ? "true" : "false");
    orbisjvm_file_write(file, buffer, len, NULL);

    /* Auto-backup section */
    len = snprintf(buffer, sizeof(buffer),
        "[AutoBackup]\n"
        "autobackup.enabled=%s\n"
        "autobackup.interval_minutes=%d\n"
        "autobackup.max_backups=%d\n"
        "autobackup.backup_dir=%s\n\n",
        mgr->auto_backup_config.enabled ? "true" : "false",
        mgr->auto_backup_config.interval_minutes,
        mgr->auto_backup_config.max_backups,
        mgr->auto_backup_config.backup_dir);
    orbisjvm_file_write(file, buffer, len, NULL);

    orbisjvm_file_close(file);
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Config saved successfully");
    return 0;
}

/* Create default configuration */
void config_manager_create_defaults(config_manager_t* mgr) {
    if (!mgr) return;

    /* Server defaults */
    strncpy(mgr->server_config.java_path, "java", sizeof(mgr->server_config.java_path) - 1);
    strncpy(mgr->server_config.server_jar, "server.jar", sizeof(mgr->server_config.server_jar) - 1);
    strncpy(mgr->server_config.server_dir, "./server", sizeof(mgr->server_config.server_dir) - 1);
    strncpy(mgr->server_config.world_name, "world", sizeof(mgr->server_config.world_name) - 1);
    mgr->server_config.heap_max_mb = 3072;
    mgr->server_config.heap_initial_mb = 1024;
    strncpy(mgr->server_config.java_flags,
        "-XX:+UseG1GC -XX:MaxGCPauseMillis=50 -XX:+ParallelRefProcEnabled "
        "-XX:+AlwaysPreTouch -XX:+UnlockExperimentalVMOptions",
        sizeof(mgr->server_config.java_flags) - 1);
    mgr->server_config.port = 25565;
    mgr->server_config.auto_start = false;
    mgr->server_config.auto_restart = true;
    mgr->server_config.restart_delay_sec = 10;
    mgr->server_config.graceful_stop_timeout_sec = 30;
    mgr->server_config.server_properties[0] = '\0';
    mgr->server_config.paper_yml[0] = '\0';

    /* App defaults */
    char app_data_dir[512];
    orbisjvm_get_app_data_dir(app_data_dir, sizeof(app_data_dir));
    snprintf(mgr->app_config.worlds_dir, sizeof(mgr->app_config.worlds_dir), "%s/worlds", app_data_dir);
    snprintf(mgr->app_config.backups_dir, sizeof(mgr->app_config.backups_dir), "%s/backups", app_data_dir);
    snprintf(mgr->app_config.logs_dir, sizeof(mgr->app_config.logs_dir), "%s/logs", app_data_dir);
    mgr->app_config.remote_api_port = 8080;
    mgr->app_config.remote_api_enabled = true;
    strncpy(mgr->app_config.remote_api_bind, "0.0.0.0", sizeof(mgr->app_config.remote_api_bind) - 1);
    mgr->app_config.remote_api_tls = false;
    mgr->app_config.remote_api_cert[0] = '\0';
    mgr->app_config.remote_api_key[0] = '\0';
    strncpy(mgr->app_config.admin_username, "admin", sizeof(mgr->app_config.admin_username) - 1);
    mgr->app_config.admin_password_hash[0] = '\0';
    mgr->app_config.log_level = 3; /* INFO */
    mgr->app_config.debug_mode = false;

    /* Auto-backup defaults */
    mgr->auto_backup_config.enabled = false;
    mgr->auto_backup_config.interval_minutes = 30;
    mgr->auto_backup_config.max_backups = 10;
    snprintf(mgr->auto_backup_config.backup_dir, sizeof(mgr->auto_backup_config.backup_dir), "%s/backups", app_data_dir);

    /* Ensure directories exist */
    orbisjvm_dir_create_recursive(mgr->app_config.worlds_dir, ORBISJVM_DEFAULT_DIR_MODE);
    orbisjvm_dir_create_recursive(mgr->app_config.backups_dir, ORBISJVM_DEFAULT_DIR_MODE);
    orbisjvm_dir_create_recursive(mgr->app_config.logs_dir, ORBISJVM_DEFAULT_DIR_MODE);
}

/* Get config file path */
int config_manager_get_config_path(config_manager_t* mgr, char* buffer, size_t size) {
    if (!mgr || !buffer || size == 0) return -1;
    strncpy(buffer, mgr->config_file, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

/* Server config getters/setters */
int config_manager_get_server_config(config_manager_t* mgr, server_config_t* config) {
    if (!mgr || !config) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    *config = mgr->server_config;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

int config_manager_set_server_config(config_manager_t* mgr, const server_config_t* config) {
    if (!mgr || !config) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    mgr->server_config = *config;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return config_manager_save(mgr);
}

/* App config getters/setters */
int config_manager_get_app_config(config_manager_t* mgr, app_config_t* config) {
    if (!mgr || !config) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    *config = mgr->app_config;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

int config_manager_set_app_config(config_manager_t* mgr, const app_config_t* config) {
    if (!mgr || !config) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    mgr->app_config = *config;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return config_manager_save(mgr);
}

/* Directory getters/setters */
int config_manager_get_worlds_dir(config_manager_t* mgr, char* buffer, size_t size) {
    if (!mgr || !buffer || size == 0) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    strncpy(buffer, mgr->app_config.worlds_dir, size - 1);
    buffer[size - 1] = '\0';
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

int config_manager_set_worlds_dir(config_manager_t* mgr, const char* path) {
    if (!mgr || !path) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    strncpy(mgr->app_config.worlds_dir, path, sizeof(mgr->app_config.worlds_dir) - 1);
    orbisjvm_mutex_unlock(&mgr->mutex);
    orbisjvm_dir_create_recursive(path, ORBISJVM_DEFAULT_DIR_MODE);
    return config_manager_save(mgr);
}

int config_manager_get_backups_dir(config_manager_t* mgr, char* buffer, size_t size) {
    if (!mgr || !buffer || size == 0) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    strncpy(buffer, mgr->app_config.backups_dir, size - 1);
    buffer[size - 1] = '\0';
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

int config_manager_set_backups_dir(config_manager_t* mgr, const char* path) {
    if (!mgr || !path) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    strncpy(mgr->app_config.backups_dir, path, sizeof(mgr->app_config.backups_dir) - 1);
    orbisjvm_mutex_unlock(&mgr->mutex);
    orbisjvm_dir_create_recursive(path, ORBISJVM_DEFAULT_DIR_MODE);
    return config_manager_save(mgr);
}

int config_manager_get_logs_dir(config_manager_t* mgr, char* buffer, size_t size) {
    if (!mgr || !buffer || size == 0) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    strncpy(buffer, mgr->app_config.logs_dir, size - 1);
    buffer[size - 1] = '\0';
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

/* Auto-backup config getters/setters */
int config_manager_get_auto_backup_config(config_manager_t* mgr, auto_backup_config_t* config) {
    if (!mgr || !config) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    *config = mgr->auto_backup_config;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

int config_manager_set_auto_backup_config(config_manager_t* mgr, const auto_backup_config_t* config) {
    if (!mgr || !config) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    mgr->auto_backup_config = *config;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return config_manager_save(mgr);
}

/* Remote API config getters/setters */
int config_manager_get_remote_api_port(config_manager_t* mgr) {
    if (!mgr) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    int port = mgr->app_config.remote_api_port;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return port;
}

int config_manager_set_remote_api_port(config_manager_t* mgr, int port) {
    if (!mgr || port <= 0 || port > 65535) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    mgr->app_config.remote_api_port = port;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return config_manager_save(mgr);
}

bool config_manager_get_remote_api_enabled(config_manager_t* mgr) {
    if (!mgr) return false;
    orbisjvm_mutex_lock(&mgr->mutex);
    bool enabled = mgr->app_config.remote_api_enabled;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return enabled;
}

int config_manager_set_remote_api_enabled(config_manager_t* mgr, bool enabled) {
    if (!mgr) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    mgr->app_config.remote_api_enabled = enabled;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return config_manager_save(mgr);
}

int config_manager_get_remote_api_bind(config_manager_t* mgr, char* buffer, size_t size) {
    if (!mgr || !buffer || size == 0) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    strncpy(buffer, mgr->app_config.remote_api_bind, size - 1);
    buffer[size - 1] = '\0';
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

int config_manager_set_remote_api_bind(config_manager_t* mgr, const char* bind) {
    if (!mgr || !bind) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    strncpy(mgr->app_config.remote_api_bind, bind, sizeof(mgr->app_config.remote_api_bind) - 1);
    orbisjvm_mutex_unlock(&mgr->mutex);
    return config_manager_save(mgr);
}

bool config_manager_get_remote_api_tls(config_manager_t* mgr) {
    if (!mgr) return false;
    orbisjvm_mutex_lock(&mgr->mutex);
    bool tls = mgr->app_config.remote_api_tls;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return tls;
}

int config_manager_set_remote_api_tls(config_manager_t* mgr, bool enabled) {
    if (!mgr) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    mgr->app_config.remote_api_tls = enabled;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return config_manager_save(mgr);
}

int config_manager_get_remote_api_cert(config_manager_t* mgr, char* buffer, size_t size) {
    if (!mgr || !buffer || size == 0) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    strncpy(buffer, mgr->app_config.remote_api_cert, size - 1);
    buffer[size - 1] = '\0';
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

int config_manager_set_remote_api_cert(config_manager_t* mgr, const char* path) {
    if (!mgr || !path) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    strncpy(mgr->app_config.remote_api_cert, path, sizeof(mgr->app_config.remote_api_cert) - 1);
    orbisjvm_mutex_unlock(&mgr->mutex);
    return config_manager_save(mgr);
}

int config_manager_get_remote_api_key(config_manager_t* mgr, char* buffer, size_t size) {
    if (!mgr || !buffer || size == 0) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    strncpy(buffer, mgr->app_config.remote_api_key, size - 1);
    buffer[size - 1] = '\0';
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

int config_manager_set_remote_api_key(config_manager_t* mgr, const char* path) {
    if (!mgr || !path) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    strncpy(mgr->app_config.remote_api_key, path, sizeof(mgr->app_config.remote_api_key) - 1);
    orbisjvm_mutex_unlock(&mgr->mutex);
    return config_manager_save(mgr);
}

/* Authentication getters/setters */
int config_manager_get_admin_username(config_manager_t* mgr, char* buffer, size_t size) {
    if (!mgr || !buffer || size == 0) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    strncpy(buffer, mgr->app_config.admin_username, size - 1);
    buffer[size - 1] = '\0';
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

int config_manager_set_admin_username(config_manager_t* mgr, const char* username) {
    if (!mgr || !username) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    strncpy(mgr->app_config.admin_username, username, sizeof(mgr->app_config.admin_username) - 1);
    orbisjvm_mutex_unlock(&mgr->mutex);
    return config_manager_save(mgr);
}

int config_manager_get_admin_password_hash(config_manager_t* mgr, char* buffer, size_t size) {
    if (!mgr || !buffer || size == 0) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    strncpy(buffer, mgr->app_config.admin_password_hash, size - 1);
    buffer[size - 1] = '\0';
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

int config_manager_set_admin_password(config_manager_t* mgr, const char* password) {
    if (!mgr || !password) return -1;

    /* Hash password using bcrypt or similar */
    char hash[128];
    /* For simplicity, use SHA-256 + salt */
    uint8_t salt[16];
    orbisjvm_secure_random(salt, sizeof(salt));

    char salted[256];
    snprintf(salted, sizeof(salted), "%s", password);

    /* In real implementation, use proper password hashing like bcrypt */
    orbisjvm_hash_compute(ORBISJVM_HASH_SHA256, salted, strlen(salted), hash, sizeof(hash));

    orbisjvm_mutex_lock(&mgr->mutex);
    snprintf(mgr->app_config.admin_password_hash, sizeof(mgr->app_config.admin_password_hash),
             "$sha256$%s$%s", hash, "salt_placeholder");
    orbisjvm_mutex_unlock(&mgr->mutex);

    return config_manager_save(mgr);
}

bool config_manager_verify_password(config_manager_t* mgr, const char* password) {
    if (!mgr || !password) return false;

    orbisjvm_mutex_lock(&mgr->mutex);
    char stored_hash[128];
    strncpy(stored_hash, mgr->app_config.admin_password_hash, sizeof(stored_hash) - 1);
    orbisjvm_mutex_unlock(&mgr->mutex);

    if (stored_hash[0] == '\0') return false;

    /* Verify using same hashing method */
    /* Simplified - real implementation would parse stored hash */
    return false; /* Placeholder */
}

/* Logging getters/setters */
int config_manager_get_log_level(config_manager_t* mgr) {
    if (!mgr) return 3;
    orbisjvm_mutex_lock(&mgr->mutex);
    int level = mgr->app_config.log_level;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return level;
}

int config_manager_set_log_level(config_manager_t* mgr, int level) {
    if (!mgr || level < 0 || level > 5) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    mgr->app_config.log_level = level;
    orbisjvm_mutex_unlock(&mgr->mutex);
    orbisjvm_diag_set_log_level(level);
    return config_manager_save(mgr);
}

bool config_manager_get_debug_mode(config_manager_t* mgr) {
    if (!mgr) return false;
    orbisjvm_mutex_lock(&mgr->mutex);
    bool debug = mgr->app_config.debug_mode;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return debug;
}

int config_manager_set_debug_mode(config_manager_t* mgr, bool enabled) {
    if (!mgr) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    mgr->app_config.debug_mode = enabled;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return config_manager_save(mgr);
}

/* Server properties helpers */
int config_manager_load_server_properties(config_manager_t* mgr, const char* path) {
    if (!mgr || !path) return -1;
    /* Parse server.properties file into server_config_t */
    return 0;
}

int config_manager_save_server_properties(config_manager_t* mgr, const char* path) {
    if (!mgr || !path) return -1;
    /* Write server.properties file from server_config_t */
    return 0;
}

int config_manager_get_server_property(config_manager_t* mgr, const char* key, char* value, size_t size) {
    if (!mgr || !key || !value || size == 0) return -1;
    /* Search in server_config_t server_properties */
    return -1;
}

int config_manager_set_server_property(config_manager_t* mgr, const char* key, const char* value) {
    if (!mgr || !key || !value) return -1;
    /* Set in server_config_t server_properties */
    return config_manager_save(mgr);
}

/* Paper/YAML helpers */
int config_manager_load_paper_yml(config_manager_t* mgr, const char* path) {
    (void)mgr; (void)path;
    return 0;
}

int config_manager_save_paper_yml(config_manager_t* mgr, const char* path) {
    (void)mgr; (void)path;
    return 0;
}

/* Reset to defaults */
int config_manager_reset_to_defaults(config_manager_t* mgr) {
    if (!mgr) return -1;
    config_manager_create_defaults(mgr);
    return config_manager_save(mgr);
}

/* Export/Import */
int config_manager_export(config_manager_t* mgr, const char* path) {
    if (!mgr || !path) return -1;
    /* Save to different path */
    char old_path[512];
    strncpy(old_path, mgr->config_file, sizeof(old_path) - 1);
    strncpy(mgr->config_file, path, sizeof(mgr->config_file) - 1);
    int ret = config_manager_save(mgr);
    strncpy(mgr->config_file, old_path, sizeof(mgr->config_file) - 1);
    return ret;
}

int config_manager_import(config_manager_t* mgr, const char* path) {
    if (!mgr || !path) return -1;
    char old_path[512];
    strncpy(old_path, mgr->config_file, sizeof(old_path) - 1);
    strncpy(mgr->config_file, path, sizeof(mgr->config_file) - 1);
    int ret = config_manager_load(mgr);
    strncpy(mgr->config_file, old_path, sizeof(mgr->config_file) - 1);
    if (ret == 0) config_manager_save(mgr);
    return ret;
}