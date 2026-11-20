/**
 * @file world_manager.h
 * @brief OrbisMC World Manager
 *
 * Manages Minecraft worlds: create, import, export, backup, delete, list.
 */

#ifndef ORBISMC_WORLD_MANAGER_H
#define ORBISMC_WORLD_MANAGER_H

#include <orbisjvm/filesystem.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
typedef struct config_manager config_manager_t;

/* World info */
typedef struct {
    char name[128];
    char path[512];
    uint64_t size_bytes;
    uint64_t last_modified;
    bool exists;
    char version[32];
    char game_mode[16];
    char difficulty[16];
    int seed;
    bool hardcore;
} world_info_t;

/* World manager */
typedef struct world_manager world_manager_t;

/* Create world manager */
world_manager_t* world_manager_create(config_manager_t* config);

/* Destroy world manager */
void world_manager_destroy(world_manager_t* mgr);

/* List all worlds */
int world_manager_list(world_manager_t* mgr, world_info_t* worlds, int max_worlds, int* count);

/* Get world info */
int world_manager_get_info(world_manager_t* mgr, const char* name, world_info_t* info);

/* Create new world */
int world_manager_create_world(world_manager_t* mgr, const char* name, int seed, const char* game_mode, const char* difficulty, bool hardcore);

/* Delete world */
int world_manager_delete_world(world_manager_t* mgr, const char* name, bool confirm);

/* Backup world */
int world_manager_backup_world(world_manager_t* mgr, const char* name, const char* backup_path);

/* Restore world from backup */
int world_manager_restore_world(world_manager_t* mgr, const char* name, const char* backup_path);

/* Export world (to zip/archive) */
int world_manager_export_world(world_manager_t* mgr, const char* name, const char* export_path);

/* Import world (from zip/archive) */
int world_manager_import_world(world_manager_t* mgr, const char* import_path, const char* new_name);

/* Rename world */
int world_manager_rename_world(world_manager_t* mgr, const char* old_name, const char* new_name);

/* Get world directory path */
int world_manager_get_world_path(world_manager_t* mgr, const char* name, char* buffer, size_t size);

/* Get worlds directory */
int world_manager_get_worlds_dir(world_manager_t* mgr, char* buffer, size_t size);

/* Calculate world size */
uint64_t world_manager_calculate_size(world_manager_t* mgr, const char* name);

/* Validate world (check for corruption) */
int world_manager_validate_world(world_manager_t* mgr, const char* name, char* error_buffer, size_t error_size);

/* World backup info */
typedef struct {
    char name[256];
    char path[512];
    uint64_t size_bytes;
    uint64_t timestamp;
    char world_name[128];
} world_backup_info_t;

/* List backups for a world */
int world_manager_list_backups(world_manager_t* mgr, const char* world_name, world_backup_info_t* backups, int max_backups, int* count);

/* Delete old backups (keep N most recent) */
int world_manager_prune_backups(world_manager_t* mgr, const char* world_name, int keep_count);

/* Auto-backup configuration */
typedef struct {
    bool enabled;
    int interval_minutes;
    int max_backups;
    char backup_dir[512];
} world_auto_backup_config_t;

int world_manager_get_auto_backup_config(world_manager_t* mgr, world_auto_backup_config_t* config);
int world_manager_set_auto_backup_config(world_manager_t* mgr, const world_auto_backup_config_t* config);

/* Auto-backup thread */
void* world_auto_backup_thread(void* arg);

#ifdef __cplusplus
}
#endif

#endif /* ORBISMC_WORLD_MANAGER_H */