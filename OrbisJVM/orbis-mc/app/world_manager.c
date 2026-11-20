/**
 * @file world_manager.c
 * @brief OrbisMC World Manager Implementation
 */

#include "world_manager.h"
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/filesystem.h>
#include <orbisjvm/threading.h>
#include <orbisjvm/time.h>
#include <orbisjvm/memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

/* World manager structure */
struct world_manager {
    config_manager_t* config_mgr;
    char worlds_dir[512];
    char backups_dir[512];
    orbisjvm_mutex_t mutex;

    /* Auto-backup */
    world_auto_backup_config_t auto_backup_config;
    orbisjvm_thread_t auto_backup_thread;
    bool auto_backup_running;
};

/* Create world manager */
world_manager_t* world_manager_create(config_manager_t* config) {
    world_manager_t* mgr = orbisjvm_malloc(sizeof(world_manager_t), ORBISJVM_MEM_ZERO);
    if (!mgr) return NULL;

    mgr->config_mgr = config;

    /* Get worlds directory from config or use default */
    if (config) {
        config_manager_get_worlds_dir(config, mgr->worlds_dir, sizeof(mgr->worlds_dir));
        config_manager_get_backups_dir(config, mgr->backups_dir, sizeof(mgr->backups_dir));
    } else {
        snprintf(mgr->worlds_dir, sizeof(mgr->worlds_dir), "./worlds");
        snprintf(mgr->backups_dir, sizeof(mgr->backups_dir), "./backups");
    }

    orbisjvm_mutex_create(&mgr->mutex, NULL);

    /* Default auto-backup config */
    mgr->auto_backup_config.enabled = false;
    mgr->auto_backup_config.interval_minutes = 30;
    mgr->auto_backup_config.max_backups = 10;
    snprintf(mgr->auto_backup_config.backup_dir, sizeof(mgr->auto_backup_config.backup_dir), "%s", mgr->backups_dir);

    mgr->auto_backup_thread = NULL;
    mgr->auto_backup_running = false;

    /* Load auto-backup config from config manager */
    if (config) {
        config_manager_get_auto_backup_config(config, &mgr->auto_backup_config);
    }

    /* Ensure directories exist */
    orbisjvm_dir_create_recursive(mgr->worlds_dir, ORBISJVM_DEFAULT_DIR_MODE);
    orbisjvm_dir_create_recursive(mgr->backups_dir, ORBISJVM_DEFAULT_DIR_MODE);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "World manager created (worlds: %s, backups: %s)", mgr->worlds_dir, mgr->backups_dir);
    return mgr;
}

/* Destroy world manager */
void world_manager_destroy(world_manager_t* mgr) {
    if (!mgr) return;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Destroying world manager...");

    /* Stop auto-backup thread */
    if (mgr->auto_backup_running) {
        mgr->auto_backup_running = false;
        if (mgr->auto_backup_thread) {
            orbisjvm_thread_join(mgr->auto_backup_thread, NULL);
        }
    }

    orbisjvm_mutex_destroy(mgr->mutex);
    orbisjvm_free(mgr);
}

/* List all worlds */
int world_manager_list(world_manager_t* mgr, world_info_t* worlds, int max_worlds, int* count) {
    if (!mgr || !worlds || !count || max_worlds <= 0) return -1;

    orbisjvm_mutex_lock(&mgr->mutex);

    orbisjvm_dir_t dir;
    int ret = orbisjvm_dir_open(&dir, mgr->worlds_dir);
    if (ret < 0) {
        orbisjvm_mutex_unlock(&mgr->mutex);
        *count = 0;
        return 0; /* Not an error, just no worlds dir */
    }

    int found = 0;
    orbisjvm_dirent_t entry;

    while (found < max_worlds) {
        ret = orbisjvm_dir_read(dir, &entry);
        if (ret == 1) break; /* End of directory */
        if (ret < 0) continue;

        if (entry.type == ORBISJVM_FILE_TYPE_DIR) {
            /* Skip . and .. */
            if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) continue;

            world_info_t* info = &worlds[found];
            strncpy(info->name, entry.name, sizeof(info->name) - 1);
            snprintf(info->path, sizeof(info->path), "%s/%s", mgr->worlds_dir, entry.name);
            info->exists = true;

            /* Get world info */
            world_manager_get_info(mgr, entry.name, info);
            found++;
        }
    }

    orbisjvm_dir_close(dir);
    orbisjvm_mutex_unlock(&mgr->mutex);

    *count = found;
    return 0;
}

/* Get world info */
int world_manager_get_info(world_manager_t* mgr, const char* name, world_info_t* info) {
    if (!mgr || !name || !info) return -1;

    memset(info, 0, sizeof(world_info_t));
    strncpy(info->name, name, sizeof(info->name) - 1);

    char world_path[512];
    snprintf(world_path, sizeof(world_path), "%s/%s", mgr->worlds_dir, name);

    info->exists = orbisjvm_path_is_dir(world_path) == 1;
    if (!info->exists) return -1;

    strncpy(info->path, world_path, sizeof(info->path) - 1);

    /* Get directory stats */
    orbisjvm_file_info_t dir_info;
    orbisjvm_file_t dir_handle;
    int ret = orbisjvm_file_open(&dir_handle, world_path, ORBISJVM_O_RDONLY | ORBISJVM_O_DIRECTORY, 0);
    if (ret == 0) {
        orbisjvm_file_stat(dir_handle, &dir_info);
        info->size_bytes = dir_info.size;
        info->last_modified = dir_info.mtime;
        orbisjvm_file_close(dir_handle);
    }

    /* Try to read level.dat for world metadata */
    char level_dat[512];
    snprintf(level_dat, sizeof(level_dat), "%s/level.dat", world_path);
    if (orbisjvm_file_exists(level_dat) == 1) {
        /* In a real implementation, we'd parse the NBT format */
        /* For now, set defaults */
        strncpy(info->version, "1.20.4", sizeof(info->version) - 1);
        strncpy(info->game_mode, "survival", sizeof(info->game_mode) - 1);
        strncpy(info->difficulty, "normal", sizeof(info->difficulty) - 1);
        info->seed = 0;
        info->hardcore = false;
    }

    return 0;
}

/* Create new world */
int world_manager_create_world(world_manager_t* mgr, const char* name, int seed, const char* game_mode, const char* difficulty, bool hardcore) {
    if (!mgr || !name) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Creating world: %s", name);

    /* Validate name */
    if (!orbisjvm_filename_valid(name)) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Invalid world name: %s", name);
        return -1;
    }

    orbisjvm_mutex_lock(&mgr->mutex);

    char world_path[512];
    snprintf(world_path, sizeof(world_path), "%s/%s", mgr->worlds_dir, name);

    /* Check if world already exists */
    if (orbisjvm_path_is_dir(world_path) == 1) {
        orbisjvm_mutex_unlock(&mgr->mutex);
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "World already exists: %s", name);
        return -1;
    }

    /* Create world directory */
    int ret = orbisjvm_dir_create_recursive(world_path, ORBISJVM_DEFAULT_DIR_MODE);
    if (ret < 0) {
        orbisjvm_mutex_unlock(&mgr->mutex);
        return ret;
    }

    /* Create basic world structure */
    char dirs[][64] = {
        "region",
        "poi",
        "entities",
        "data",
        "advancements",
        "playerdata",
        "stats"
    };

    for (int i = 0; i < 7; i++) {
        char subdir[512];
        snprintf(subdir, sizeof(subdir), "%s/%s", world_path, dirs[i]);
        orbisjvm_dir_create_recursive(subdir, ORBISJVM_DEFAULT_DIR_MODE);
    }

    /* Create level.dat (minimal NBT) */
    char level_dat[512];
    snprintf(level_dat, sizeof(level_dat), "%s/level.dat", world_path);
    /* In real implementation, write proper NBT format */
    orbisjvm_file_t file;
    ret = orbisjvm_file_open(&file, level_dat, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret == 0) {
        /* Write minimal level.dat placeholder */
        const char* placeholder = "NBT placeholder for level.dat";
        orbisjvm_file_write(file, placeholder, strlen(placeholder), NULL);
        orbisjvm_file_close(file);
    }

    /* Create level.dat_old */
    char level_dat_old[512];
    snprintf(level_dat_old, sizeof(level_dat_old), "%s/level.dat_old", world_path);
    ret = orbisjvm_file_open(&file, level_dat_old, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret == 0) {
        orbisjvm_file_close(file);
    }

    /* Create session.lock */
    char session_lock[512];
    snprintf(session_lock, sizeof(session_lock), "%s/session.lock", world_path);
    ret = orbisjvm_file_open(&file, session_lock, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret == 0) {
        orbisjvm_file_close(file);
    }

    orbisjvm_mutex_unlock(&mgr->mutex);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "World created: %s", name);
    return 0;
}

/* Delete world */
int world_manager_delete_world(world_manager_t* mgr, const char* name, bool confirm) {
    if (!mgr || !name) return -1;

    if (!confirm) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "World deletion requires confirmation");
        return -1;
    }

    orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "Deleting world: %s", name);

    orbisjvm_mutex_lock(&mgr->mutex);

    char world_path[512];
    snprintf(world_path, sizeof(world_path), "%s/%s", mgr->worlds_dir, name);

    /* Recursively remove directory */
    int ret = orbisjvm_remove_recursive(world_path);

    orbisjvm_mutex_unlock(&mgr->mutex);

    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to delete world: %s", name);
        return ret;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "World deleted: %s", name);
    return 0;
}

/* Recursive directory removal helper */
static int orbisjvm_remove_recursive(const char* path) {
    orbisjvm_dir_t dir;
    int ret = orbisjvm_dir_open(&dir, path);
    if (ret < 0) return ret;

    orbisjvm_dirent_t entry;
    while (true) {
        ret = orbisjvm_dir_read(dir, &entry);
        if (ret == 1) break;
        if (ret < 0) continue;

        if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry.name);

        if (entry.type == ORBISJVM_FILE_TYPE_DIR) {
            orbisjvm_remove_recursive(full_path);
        } else {
            orbisjvm_file_remove(full_path);
        }
    }

    orbisjvm_dir_close(dir);
    return orbisjvm_dir_remove(path);
}

/* Backup world */
int world_manager_backup_world(world_manager_t* mgr, const char* name, const char* backup_path) {
    if (!mgr || !name) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Backing up world: %s", name);

    orbisjvm_mutex_lock(&mgr->mutex);

    char world_path[512];
    snprintf(world_path, sizeof(world_path), "%s/%s", mgr->worlds_dir, name);

    char dest_path[512];
    if (backup_path) {
        strncpy(dest_path, backup_path, sizeof(dest_path) - 1);
    } else {
        /* Generate timestamped backup name */
        time_t now = time(NULL);
        struct tm* tm = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm);
        snprintf(dest_path, sizeof(dest_path), "%s/%s_%s", mgr->backups_dir, name, timestamp);
    }

    /* Create backup directory */
    int ret = orbisjvm_dir_create_recursive(dest_path, ORBISJVM_DEFAULT_DIR_MODE);
    if (ret < 0) {
        orbisjvm_mutex_unlock(&mgr->mutex);
        return ret;
    }

    /* Copy world directory recursively */
    ret = orbisjvm_copy_recursive(world_path, dest_path);

    orbisjvm_mutex_unlock(&mgr->mutex);

    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to backup world: %s", name);
        return ret;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "World backed up to: %s", dest_path);
    return 0;
}

/* Recursive copy helper */
static int orbisjvm_copy_recursive(const char* src, const char* dst) {
    orbisjvm_dir_t dir;
    int ret = orbisjvm_dir_open(&dir, src);
    if (ret < 0) return ret;

    /* Create destination directory */
    orbisjvm_dir_create_recursive(dst, ORBISJVM_DEFAULT_DIR_MODE);

    orbisjvm_dirent_t entry;
    while (true) {
        ret = orbisjvm_dir_read(dir, &entry);
        if (ret == 1) break;
        if (ret < 0) continue;

        if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) continue;

        char src_path[1024];
        char dst_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry.name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry.name);

        if (entry.type == ORBISJVM_FILE_TYPE_DIR) {
            orbisjvm_copy_recursive(src_path, dst_path);
        } else {
            orbisjvm_file_t src_file, dst_file;
            int ret = orbisjvm_file_open(&src_file, src_path, ORBISJVM_O_RDONLY, 0);
            if (ret < 0) continue;

            ret = orbisjvm_file_open(&dst_file, dst_path, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
            if (ret < 0) {
                orbisjvm_file_close(src_file);
                continue;
            }

            char buffer[8192];
            size_t read, written;
            while ((ret = orbisjvm_file_read(src_file, buffer, sizeof(buffer), &read)) == 0 && read > 0) {
                orbisjvm_file_write(dst_file, buffer, read, &written);
            }

            orbisjvm_file_close(src_file);
            orbisjvm_file_close(dst_file);
        }
    }

    orbisjvm_dir_close(dir);
    return 0;
}

/* Restore world from backup */
int world_manager_restore_world(world_manager_t* mgr, const char* name, const char* backup_path) {
    if (!mgr || !name || !backup_path) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Restoring world: %s from %s", name, backup_path);

    /* Delete existing world if present */
    world_manager_delete_world(mgr, name, true);

    /* Copy backup to world directory */
    char world_path[512];
    snprintf(world_path, sizeof(world_path), "%s/%s", mgr->worlds_dir, name);

    return orbisjvm_copy_recursive(backup_path, world_path);
}

/* Export world (placeholder) */
int world_manager_export_world(world_manager_t* mgr, const char* name, const char* export_path) {
    (void)mgr; (void)name; (void)export_path;
    orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "World export not yet implemented");
    return -1;
}

/* Import world (placeholder) */
int world_manager_import_world(world_manager_t* mgr, const char* import_path, const char* new_name) {
    (void)mgr; (void)import_path; (void)new_name;
    orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "World import not yet implemented");
    return -1;
}

/* Rename world */
int world_manager_rename_world(world_manager_t* mgr, const char* old_name, const char* new_name) {
    if (!mgr || !old_name || !new_name) return -1;

    if (!orbisjvm_filename_valid(new_name)) {
        return -1;
    }

    orbisjvm_mutex_lock(&mgr->mutex);

    char old_path[512];
    char new_path[512];
    snprintf(old_path, sizeof(old_path), "%s/%s", mgr->worlds_dir, old_name);
    snprintf(new_path, sizeof(new_path), "%s/%s", mgr->worlds_dir, new_name);

    int ret = orbisjvm_file_rename(old_path, new_path);

    orbisjvm_mutex_unlock(&mgr->mutex);

    return ret;
}

/* Get world path */
int world_manager_get_world_path(world_manager_t* mgr, const char* name, char* buffer, size_t size) {
    if (!mgr || !name || !buffer || size == 0) return -1;
    snprintf(buffer, size, "%s/%s", mgr->worlds_dir, name);
    return 0;
}

/* Get worlds directory */
int world_manager_get_worlds_dir(world_manager_t* mgr, char* buffer, size_t size) {
    if (!mgr || !buffer || size == 0) return -1;
    strncpy(buffer, mgr->worlds_dir, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

/* Calculate world size */
uint64_t world_manager_calculate_size(world_manager_t* mgr, const char* name) {
    if (!mgr || !name) return 0;

    char world_path[512];
    snprintf(world_path, sizeof(world_path), "%s/%s", mgr->worlds_dir, name);

    uint64_t total_size = 0;
    orbisjvm_dir_t dir;
    int ret = orbisjvm_dir_open(&dir, world_path);
    if (ret < 0) return 0;

    orbisjvm_dirent_t entry;
    while (true) {
        ret = orbisjvm_dir_read(dir, &entry);
        if (ret == 1) break;
        if (ret < 0) continue;

        if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", world_path, entry.name);

        orbisjvm_file_info_t info;
        ret = orbisjvm_file_lstat(full_path, &info, 1);
        if (ret == 0) {
            total_size += info.size;
        }

        if (entry.type == ORBISJVM_FILE_TYPE_DIR) {
            total_size += world_manager_calculate_size(mgr, entry.name);
        }
    }

    orbisjvm_dir_close(dir);
    return total_size;
}

/* Validate world */
int world_manager_validate_world(world_manager_t* mgr, const char* name, char* error_buffer, size_t error_size) {
    if (!mgr || !name) return -1;

    char world_path[512];
    snprintf(world_path, sizeof(world_path), "%s/%s", mgr->worlds_dir, name);

    /* Check required files/directories */
    const char* required[] = {
        "level.dat",
        "region",
        "data"
    };

    for (int i = 0; i < 3; i++) {
        char check_path[512];
        snprintf(check_path, sizeof(check_path), "%s/%s", world_path, required[i]);

        int exists = (strstr(required[i], ".") != NULL) ? orbisjvm_file_exists(check_path) : orbisjvm_path_is_dir(check_path);
        if (exists != 1) {
            if (error_buffer && error_size > 0) {
                snprintf(error_buffer, error_size, "Missing required file/directory: %s", required[i]);
            }
            return -1;
        }
    }

    return 0;
}

/* List backups */
int world_manager_list_backups(world_manager_t* mgr, const char* world_name, world_backup_info_t* backups, int max_backups, int* count) {
    if (!mgr || !world_name || !backups || !count || max_backups <= 0) return -1;

    orbisjvm_mutex_lock(&mgr->mutex);

    orbisjvm_dir_t dir;
    int ret = orbisjvm_dir_open(&dir, mgr->backups_dir);
    if (ret < 0) {
        orbisjvm_mutex_unlock(&mgr->mutex);
        *count = 0;
        return 0;
    }

    int found = 0;
    orbisjvm_dirent_t entry;

    while (found < max_backups) {
        ret = orbisjvm_dir_read(dir, &entry);
        if (ret == 1) break;
        if (ret < 0) continue;

        if (entry.type != ORBISJVM_FILE_TYPE_DIR) continue;

        /* Check if backup belongs to this world */
        if (strncmp(entry.name, world_name, strlen(world_name)) != 0) continue;
        if (entry.name[strlen(world_name)] != '_') continue;

        world_backup_info_t* info = &backups[found];
        strncpy(info->name, entry.name, sizeof(info->name) - 1);
        snprintf(info->path, sizeof(info->path), "%s/%s", mgr->backups_dir, entry.name);
        strncpy(info->world_name, world_name, sizeof(info->world_name) - 1);

        char backup_path[512];
        snprintf(backup_path, sizeof(backup_path), "%s/%s", mgr->backups_dir, entry.name);

        orbisjvm_file_info_t bk_info;
        orbisjvm_file_t file;
        if (orbisjvm_file_open(&file, backup_path, ORBISJVM_O_RDONLY | ORBISJVM_O_DIRECTORY, 0) == 0) {
            orbisjvm_file_stat(file, &bk_info);
            info->size_bytes = bk_info.size;
            info->timestamp = bk_info.mtime;
            orbisjvm_file_close(file);
        }

        found++;
    }

    orbisjvm_dir_close(dir);
    orbisjvm_mutex_unlock(&mgr->mutex);

    *count = found;
    return 0;
}

/* Prune old backups */
int world_manager_prune_backups(world_manager_t* mgr, const char* world_name, int keep_count) {
    if (!mgr || !world_name || keep_count <= 0) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Pruning backups for %s (keeping %d)", world_name, keep_count);

    world_backup_info_t backups[100];
    int count = 0;
    int ret = world_manager_list_backups(mgr, world_name, backups, 100, &count);
    if (ret < 0 || count <= keep_count) return 0;

    /* Sort by timestamp (oldest first) - simple bubble sort for small arrays */
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (backups[j].timestamp > backups[j + 1].timestamp) {
                world_backup_info_t tmp = backups[j];
                backups[j] = backups[j + 1];
                backups[j + 1] = tmp;
            }
        }
    }

    /* Delete oldest backups */
    int to_delete = count - keep_count;
    for (int i = 0; i < to_delete; i++) {
        orbisjvm_remove_recursive(backups[i].path);
        orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Pruned old backup: %s", backups[i].name);
    }

    return 0;
}

/* Get auto-backup config */
int world_manager_get_auto_backup_config(world_manager_t* mgr, world_auto_backup_config_t* config) {
    if (!mgr || !config) return -1;
    orbisjvm_mutex_lock(&mgr->mutex);
    *config = mgr->auto_backup_config;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

/* Set auto-backup config */
int world_manager_set_auto_backup_config(world_manager_t* mgr, const world_auto_backup_config_t* config) {
    if (!mgr || !config) return -1;

    orbisjvm_mutex_lock(&mgr->mutex);
    mgr->auto_backup_config = *config;
    orbisjvm_mutex_unlock(&mgr->mutex);

    /* Save to config manager */
    if (mgr->config_mgr) {
        config_manager_set_auto_backup_config(mgr->config_mgr, config);
    }

    return 0;
}

/* Auto-backup thread */
void* world_auto_backup_thread(void* arg) {
    world_manager_t* mgr = (world_manager_t*)arg;

    while (mgr->auto_backup_running) {
        orbisjvm_thread_sleep_ms(mgr->auto_backup_config.interval_minutes * 60 * 1000);

        if (!mgr->auto_backup_running) break;

        orbisjvm_mutex_lock(&mgr->mutex);
        bool enabled = mgr->auto_backup_config.enabled;
        orbisjvm_mutex_unlock(&mgr->mutex);

        if (!enabled) continue;

        /* List worlds and backup each */
        world_info_t worlds[50];
        int count = 0;
        world_manager_list(mgr, worlds, 50, &count);

        for (int i = 0; i < count; i++) {
            world_manager_backup_world(mgr, worlds[i].name, NULL);
            world_manager_prune_backups(mgr, worlds[i].name, mgr->auto_backup_config.max_backups);
        }
    }

    return NULL;
}

/* Start auto-backup */
int world_manager_start_auto_backup(world_manager_t* mgr) {
    if (!mgr || mgr->auto_backup_running) return -1;

    mgr->auto_backup_running = true;
    int ret = orbisjvm_thread_create(&mgr->auto_backup_thread, NULL, world_auto_backup_thread, mgr);
    if (ret < 0) {
        mgr->auto_backup_running = false;
        return ret;
    }

    return 0;
}

/* Stop auto-backup */
int world_manager_stop_auto_backup(world_manager_t* mgr) {
    if (!mgr || !mgr->auto_backup_running) return -1;

    mgr->auto_backup_running = false;
    if (mgr->auto_backup_thread) {
        orbisjvm_thread_join(mgr->auto_backup_thread, NULL);
        mgr->auto_backup_thread = NULL;
    }

    return 0;
}