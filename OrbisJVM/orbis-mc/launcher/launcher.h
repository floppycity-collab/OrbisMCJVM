/**
 * @file launcher.h
 * @brief OrbisMC Launcher
 *
 * Handles first-time setup, updates, and application launching.
 */

#ifndef ORBISMC_LAUNCHER_H
#define ORBISMC_LAUNCHER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Check if first-time setup is needed */
bool launcher_needs_setup(void);

/* Run first-time setup wizard */
int launcher_run_setup(void);

/* Check for updates */
int launcher_check_updates(bool* update_available, char* latest_version, size_t size);

/* Download and apply update */
int launcher_apply_update(const char* version);

/* Verify installation integrity */
int launcher_verify_installation(char* error_buffer, size_t size);

/* Repair installation */
int launcher_repair_installation(void);

/* Uninstall OrbisMC */
int launcher_uninstall(bool keep_data);

/* Get installed version */
int launcher_get_version(char* buffer, size_t size);

/* Get installation path */
int launcher_get_install_path(char* buffer, size_t size);

/* Create desktop shortcut/entry */
int launcher_create_shortcut(void);

/* Remove desktop shortcut/entry */
int launcher_remove_shortcut(void);

/* Backup user data */
int launcher_backup_user_data(const char* backup_path);

/* Restore user data */
int launcher_restore_user_data(const char* backup_path);

/* Migration from older versions */
int launcher_migrate_from_version(const char* from_version);

/* Launcher configuration */
typedef struct {
    char install_path[512];
    char data_path[512];
    char config_path[512];
    char logs_path[512];
    bool auto_update;
    bool check_updates_on_start;
    char update_channel[32]; /* stable, beta, nightly */
    char last_update_check[64];
} launcher_config_t;

int launcher_get_config(launcher_config_t* config);
int launcher_set_config(const launcher_config_t* config);

/* Java detection and management */
int launcher_detect_java(char* java_path, size_t size);
int launcher_install_java(const char* version);
int launcher_set_java_path(const char* path);

/* Server JAR management */
int launcher_download_server_jar(const char* version, const char* type); /* vanilla, paper, fabric */
int launcher_verify_server_jar(const char* path);
int launcher_list_available_versions(char** versions, int max_versions, int* count);

/* Profile management */
typedef struct {
    char name[64];
    char java_path[256];
    char server_jar[256];
    char server_dir[512];
    char jvm_args[1024];
    char server_args[1024];
} launcher_profile_t;

int launcher_create_profile(const launcher_profile_t* profile);
int launcher_delete_profile(const char* name);
int launcher_get_profile(const char* name, launcher_profile_t* profile);
int launcher_list_profiles(launcher_profile_t* profiles, int max_profiles, int* count);
int launcher_set_active_profile(const char* name);

/* Logs */
int launcher_get_logs(char* buffer, size_t size, int max_lines);
int launcher_clear_logs(void);

#ifdef __cplusplus
}
#endif

#endif /* ORBISMC_LAUNCHER_H */