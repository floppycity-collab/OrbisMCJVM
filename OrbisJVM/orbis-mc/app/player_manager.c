/**
 * @file player_manager.c
 * @brief OrbisMC Player Manager Implementation
 */

#include "player_manager.h"
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/threading.h>
#include <orbisjvm/filesystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Player manager structure */
struct player_manager {
    server_manager_t* server;
    orbisjvm_mutex_t mutex;

    /* Player cache */
    player_info_t* players;
    int player_count;
    int player_capacity;

    /* Whitelist */
    char** whitelist;
    int whitelist_count;
    int whitelist_capacity;

    /* Ops */
    char** ops;
    int ops_count;
    int ops_capacity;

    /* Bans */
    typedef struct {
        char name[64];
        char uuid[64];
        char reason[256];
        uint64_t banned_at;
        char banned_by[64];
    } ban_entry_t;

    ban_entry_t* bans;
    int bans_count;
    int bans_capacity;

    /* Callbacks */
    player_join_callback_t join_callback;
    void* join_callback_data;
    player_leave_callback_t leave_callback;
    void* leave_callback_data;
    player_chat_callback_t chat_callback;
    void* chat_callback_data;
};

/* Create player manager */
player_manager_t* player_manager_create(server_manager_t* server) {
    player_manager_t* mgr = orbisjvm_malloc(sizeof(player_manager_t), ORBISJVM_MEM_ZERO);
    if (!mgr) return NULL;

    mgr->server = server;
    orbisjvm_mutex_create(&mgr->mutex, NULL);

    /* Initialize player cache */
    mgr->player_capacity = 100;
    mgr->players = orbisjvm_malloc(mgr->player_capacity * sizeof(player_info_t), ORBISJVM_MEM_ZERO);
    mgr->player_count = 0;

    /* Initialize whitelist */
    mgr->whitelist_capacity = 100;
    mgr->whitelist = orbisjvm_malloc(mgr->whitelist_capacity * sizeof(char*), ORBISJVM_MEM_ZERO);
    mgr->whitelist_count = 0;

    /* Initialize ops */
    mgr->ops_capacity = 50;
    mgr->ops = orbisjvm_malloc(mgr->ops_capacity * sizeof(char*), ORBISJVM_MEM_ZERO);
    mgr->ops_count = 0;

    /* Initialize bans */
    mgr->bans_capacity = 100;
    mgr->bans = orbisjvm_malloc(mgr->bans_capacity * sizeof(ban_entry_t), ORBISJVM_MEM_ZERO);
    mgr->bans_count = 0;

    /* Load persisted data */
    player_manager_load_all(mgr);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Player manager created");
    return mgr;
}

/* Destroy player manager */
void player_manager_destroy(player_manager_t* mgr) {
    if (!mgr) return;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Destroying player manager...");

    /* Save data */
    player_manager_save_all(mgr);

    /* Free player cache */
    orbisjvm_free(mgr->players);

    /* Free whitelist */
    for (int i = 0; i < mgr->whitelist_count; i++) {
        orbisjvm_free(mgr->whitelist[i]);
    }
    orbisjvm_free(mgr->whitelist);

    /* Free ops */
    for (int i = 0; i < mgr->ops_count; i++) {
        orbisjvm_free(mgr->ops[i]);
    }
    orbisjvm_free(mgr->ops);

    /* Free bans */
    orbisjvm_free(mgr->bans);

    orbisjvm_mutex_destroy(mgr->mutex);
    orbisjvm_free(mgr);
}

/* Get online players */
int player_manager_get_online_players(player_manager_t* mgr, player_info_t* players, int max_players, int* count) {
    if (!mgr || !players || !count || max_players <= 0) return -1;

    orbisjvm_mutex_lock(&mgr->mutex);

    int online = 0;
    for (int i = 0; i < mgr->player_count && online < max_players; i++) {
        if (mgr->players[i].online) {
            players[online++] = mgr->players[i];
        }
    }

    *count = online;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

/* Get player by name */
int player_manager_get_player(player_manager_t* mgr, const char* name, player_info_t* info) {
    if (!mgr || !name || !info) return -1;

    orbisjvm_mutex_lock(&mgr->mutex);

    for (int i = 0; i < mgr->player_count; i++) {
        if (strcmp(mgr->players[i].name, name) == 0) {
            *info = mgr->players[i];
            orbisjvm_mutex_unlock(&mgr->mutex);
            return 0;
        }
    }

    orbisjvm_mutex_unlock(&mgr->mutex);
    return -1;
}

/* Kick player */
int player_manager_kick(player_manager_t* mgr, const char* name, const char* reason) {
    if (!mgr || !name) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Kicking player: %s (%s)", name, reason ? reason : "No reason given");

    if (mgr->server) {
        char command[512];
        snprintf(command, sizeof(command), "kick %s %s", name, reason ? reason : "");
        /* server_manager_execute_command(mgr->server, command); */
    }

    /* Update local cache */
    orbisjvm_mutex_lock(&mgr->mutex);
    for (int i = 0; i < mgr->player_count; i++) {
        if (strcmp(mgr->players[i].name, name) == 0) {
            mgr->players[i].online = false;
            break;
        }
    }
    orbisjvm_mutex_unlock(&mgr->mutex);

    return 0;
}

/* Ban player */
int player_manager_ban(player_manager_t* mgr, const char* name, const char* reason) {
    if (!mgr || !name) return -1;

    orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "Banning player: %s (%s)", name, reason ? reason : "No reason given");

    /* Kick first */
    player_manager_kick(mgr, name, reason);

    /* Add to ban list */
    orbisjvm_mutex_lock(&mgr->mutex);

    /* Resize if needed */
    if (mgr->bans_count >= mgr->bans_capacity) {
        mgr->bans_capacity *= 2;
        mgr->bans = orbisjvm_realloc(mgr->bans, mgr->bans_capacity * sizeof(ban_entry_t), ORBISJVM_MEM_ZERO);
    }

    ban_entry_t* ban = &mgr->bans[mgr->bans_count++];
    strncpy(ban->name, name, sizeof(ban->name) - 1);
    ban->uuid[0] = '\0'; /* Would need to look up UUID */
    strncpy(ban->reason, reason ? reason : "No reason given", sizeof(ban->reason) - 1);
    ban->banned_at = time(NULL);
    strncpy(ban->banned_by, "console", sizeof(ban->banned_by) - 1);

    orbisjvm_mutex_unlock(&mgr->mutex);

    /* Execute ban command on server */
    if (mgr->server) {
        char command[512];
        snprintf(command, sizeof(command), "ban %s %s", name, reason ? reason : "");
        /* server_manager_execute_command(mgr->server, command); */
    }

    return 0;
}

/* Unban player */
int player_manager_unban(player_manager_t* mgr, const char* name) {
    if (!mgr || !name) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Unbanning player: %s", name);

    orbisjvm_mutex_lock(&mgr->mutex);

    for (int i = 0; i < mgr->bans_count; i++) {
        if (strcmp(mgr->bans[i].name, name) == 0) {
            /* Remove by shifting */
            for (int j = i; j < mgr->bans_count - 1; j++) {
                mgr->bans[j] = mgr->bans[j + 1];
            }
            mgr->bans_count--;
            break;
        }
    }

    orbisjvm_mutex_unlock(&mgr->mutex);

    /* Execute unban command on server */
    if (mgr->server) {
        char command[512];
        snprintf(command, sizeof(command), "pardon %s", name);
        /* server_manager_execute_command(mgr->server, command); */
    }

    return 0;
}

/* Add to whitelist */
int player_manager_whitelist_add(player_manager_t* mgr, const char* name) {
    if (!mgr || !name) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Adding to whitelist: %s", name);

    orbisjvm_mutex_lock(&mgr->mutex);

    /* Check if already whitelisted */
    for (int i = 0; i < mgr->whitelist_count; i++) {
        if (strcmp(mgr->whitelist[i], name) == 0) {
            orbisjvm_mutex_unlock(&mgr->mutex);
            return 0; /* Already whitelisted */
        }
    }

    /* Resize if needed */
    if (mgr->whitelist_count >= mgr->whitelist_capacity) {
        mgr->whitelist_capacity *= 2;
        mgr->whitelist = orbisjvm_realloc(mgr->whitelist, mgr->whitelist_capacity * sizeof(char*), ORBISJVM_MEM_ZERO);
    }

    mgr->whitelist[mgr->whitelist_count++] = strdup(name);

    orbisjvm_mutex_unlock(&mgr->mutex);

    /* Execute whitelist command on server */
    if (mgr->server) {
        char command[512];
        snprintf(command, sizeof(command), "whitelist add %s", name);
        /* server_manager_execute_command(mgr->server, command); */
    }

    return 0;
}

/* Remove from whitelist */
int player_manager_whitelist_remove(player_manager_t* mgr, const char* name) {
    if (!mgr || !name) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Removing from whitelist: %s", name);

    orbisjvm_mutex_lock(&mgr->mutex);

    for (int i = 0; i < mgr->whitelist_count; i++) {
        if (strcmp(mgr->whitelist[i], name) == 0) {
            orbisjvm_free(mgr->whitelist[i]);
            for (int j = i; j < mgr->whitelist_count - 1; j++) {
                mgr->whitelist[j] = mgr->whitelist[j + 1];
            }
            mgr->whitelist_count--;
            break;
        }
    }

    orbisjvm_mutex_unlock(&mgr->mutex);

    /* Execute whitelist command on server */
    if (mgr->server) {
        char command[512];
        snprintf(command, sizeof(command), "whitelist remove %s", name);
        /* server_manager_execute_command(mgr->server, command); */
    }

    return 0;
}

/* Check if whitelisted */
bool player_manager_is_whitelisted(player_manager_t* mgr, const char* name) {
    if (!mgr || !name) return false;

    orbisjvm_mutex_lock(&mgr->mutex);

    for (int i = 0; i < mgr->whitelist_count; i++) {
        if (strcmp(mgr->whitelist[i], name) == 0) {
            orbisjvm_mutex_unlock(&mgr->mutex);
            return true;
        }
    }

    orbisjvm_mutex_unlock(&mgr->mutex);
    return false;
}

/* Set operator status */
int player_manager_set_op(player_manager_t* mgr, const char* name, bool op) {
    if (!mgr || !name) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Setting OP status for %s: %s", name, op ? "true" : "false");

    orbisjvm_mutex_lock(&mgr->mutex);

    if (op) {
        /* Add to ops */
        bool already_op = false;
        for (int i = 0; i < mgr->ops_count; i++) {
            if (strcmp(mgr->ops[i], name) == 0) {
                already_op = true;
                break;
            }
        }

        if (!already_op) {
            if (mgr->ops_count >= mgr->ops_capacity) {
                mgr->ops_capacity *= 2;
                mgr->ops = orbisjvm_realloc(mgr->ops, mgr->ops_capacity * sizeof(char*), ORBISJVM_MEM_ZERO);
            }
            mgr->ops[mgr->ops_count++] = strdup(name);
        }
    } else {
        /* Remove from ops */
        for (int i = 0; i < mgr->ops_count; i++) {
            if (strcmp(mgr->ops[i], name) == 0) {
                orbisjvm_free(mgr->ops[i]);
                for (int j = i; j < mgr->ops_count - 1; j++) {
                    mgr->ops[j] = mgr->ops[j + 1];
                }
                mgr->ops_count--;
                break;
            }
        }
    }

    orbisjvm_mutex_unlock(&mgr->mutex);

    /* Execute op/deop command on server */
    if (mgr->server) {
        char command[512];
        snprintf(command, sizeof(command), "%s %s", op ? "op" : "deop", name);
        /* server_manager_execute_command(mgr->server, command); */
    }

    return 0;
}

/* Check if operator */
bool player_manager_is_op(player_manager_t* mgr, const char* name) {
    if (!mgr || !name) return false;

    orbisjvm_mutex_lock(&mgr->mutex);

    for (int i = 0; i < mgr->ops_count; i++) {
        if (strcmp(mgr->ops[i], name) == 0) {
            orbisjvm_mutex_unlock(&mgr->mutex);
            return true;
        }
    }

    orbisjvm_mutex_unlock(&mgr->mutex);
    return false;
}

/* Get banned players */
int player_manager_get_banned(player_manager_t* mgr, player_info_t* players, int max_players, int* count) {
    if (!mgr || !players || !count || max_players <= 0) return -1;

    orbisjvm_mutex_lock(&mgr->mutex);

    int found = 0;
    for (int i = 0; i < mgr->bans_count && found < max_players; i++) {
        player_info_t* p = &players[found++];
        memset(p, 0, sizeof(player_info_t));
        strncpy(p->name, mgr->bans[i].name, sizeof(p->name) - 1);
        strncpy(p->uuid, mgr->bans[i].uuid, sizeof(p->uuid) - 1);
        p->banned = true;
    }

    *count = found;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

/* Get whitelisted players */
int player_manager_get_whitelisted(player_manager_t* mgr, player_info_t* players, int max_players, int* count) {
    if (!mgr || !players || !count || max_players <= 0) return -1;

    orbisjvm_mutex_lock(&mgr->mutex);

    int found = 0;
    for (int i = 0; i < mgr->whitelist_count && found < max_players; i++) {
        player_info_t* p = &players[found++];
        memset(p, 0, sizeof(player_info_t));
        strncpy(p->name, mgr->whitelist[i], sizeof(p->name) - 1);
        p->whitelisted = true;
    }

    *count = found;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

/* Get operators */
int player_manager_get_ops(player_manager_t* mgr, player_info_t* players, int max_players, int* count) {
    if (!mgr || !players || !count || max_players <= 0) return -1;

    orbisjvm_mutex_lock(&mgr->mutex);

    int found = 0;
    for (int i = 0; i < mgr->ops_count && found < max_players; i++) {
        player_info_t* p = &players[found++];
        memset(p, 0, sizeof(player_info_t));
        strncpy(p->name, mgr->ops[i], sizeof(p->name) - 1);
        p->op = true;
    }

    *count = found;
    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

/* Teleport player */
int player_manager_teleport(player_manager_t* mgr, const char* name, double x, double y, double z) {
    if (!mgr || !name) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Teleporting %s to %.1f, %.1f, %.1f", name, x, y, z);

    if (mgr->server) {
        char command[512];
        snprintf(command, sizeof(command), "tp %s %.1f %.1f %.1f", name, x, y, z);
        /* server_manager_execute_command(mgr->server, command); */
    }

    return 0;
}

/* Set player gamemode */
int player_manager_set_gamemode(player_manager_t* mgr, const char* name, int gamemode) {
    if (!mgr || !name) return -1;

    const char* modes[] = {"survival", "creative", "adventure", "spectator"};
    if (gamemode < 0 || gamemode > 3) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Setting gamemode for %s to %s", name, modes[gamemode]);

    if (mgr->server) {
        char command[512];
        snprintf(command, sizeof(command), "gamemode %s %s", modes[gamemode], name);
        /* server_manager_execute_command(mgr->server, command); */
    }

    return 0;
}

/* Give item to player */
int player_manager_give_item(player_manager_t* mgr, const char* name, const char* item, int count) {
    if (!mgr || !name || !item || count <= 0) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Giving %d x %s to %s", count, item, name);

    if (mgr->server) {
        char command[512];
        snprintf(command, sizeof(command), "give %s %s %d", name, item, count);
        /* server_manager_execute_command(mgr->server, command); */
    }

    return 0;
}

/* Send message to player */
int player_manager_send_message(player_manager_t* mgr, const char* name, const char* message) {
    if (!mgr || !name || !message) return -1;

    if (mgr->server) {
        char command[512];
        snprintf(command, sizeof(command), "tell %s %s", name, message);
        /* server_manager_execute_command(mgr->server, command); */
    }

    return 0;
}

/* Broadcast message */
int player_manager_broadcast(player_manager_t* mgr, const char* message) {
    if (!mgr || !message) return -1;

    if (mgr->server) {
        char command[512];
        snprintf(command, sizeof(command), "say %s", message);
        /* server_manager_execute_command(mgr->server, command); */
    }

    return 0;
}

/* Save all player data */
int player_manager_save_all(player_manager_t* mgr) {
    if (!mgr) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Saving player data...");

    /* Save whitelist */
    char whitelist_path[512];
    orbisjvm_get_app_data_dir(whitelist_path, sizeof(whitelist_path));
    strncat(whitelist_path, "/whitelist.txt", sizeof(whitelist_path) - strlen(whitelist_path) - 1);

    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, whitelist_path, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret == 0) {
        orbisjvm_mutex_lock(&mgr->mutex);
        for (int i = 0; i < mgr->whitelist_count; i++) {
            char line[128];
            snprintf(line, sizeof(line), "%s\n", mgr->whitelist[i]);
            orbisjvm_file_write(file, line, strlen(line), NULL);
        }
        orbisjvm_mutex_unlock(&mgr->mutex);
        orbisjvm_file_close(file);
    }

    /* Save ops */
    char ops_path[512];
    orbisjvm_get_app_data_dir(ops_path, sizeof(ops_path));
    strncat(ops_path, "/ops.txt", sizeof(ops_path) - strlen(ops_path) - 1);

    ret = orbisjvm_file_open(&file, ops_path, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret == 0) {
        orbisjvm_mutex_lock(&mgr->mutex);
        for (int i = 0; i < mgr->ops_count; i++) {
            char line[128];
            snprintf(line, sizeof(line), "%s\n", mgr->ops[i]);
            orbisjvm_file_write(file, line, strlen(line), NULL);
        }
        orbisjvm_mutex_unlock(&mgr->mutex);
        orbisjvm_file_close(file);
    }

    /* Save bans */
    char bans_path[512];
    orbisjvm_get_app_data_dir(bans_path, sizeof(bans_path));
    strncat(bans_path, "/bans.txt", sizeof(bans_path) - strlen(bans_path) - 1);

    ret = orbisjvm_file_open(&file, bans_path, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret == 0) {
        orbisjvm_mutex_lock(&mgr->mutex);
        for (int i = 0; i < mgr->bans_count; i++) {
            char line[512];
            snprintf(line, sizeof(line), "%s|%s|%llu|%s|%s\n",
                     mgr->bans[i].name,
                     mgr->bans[i].uuid,
                     mgr->bans[i].banned_at,
                     mgr->bans[i].banned_by,
                     mgr->bans[i].reason);
            orbisjvm_file_write(file, line, strlen(line), NULL);
        }
        orbisjvm_mutex_unlock(&mgr->mutex);
        orbisjvm_file_close(file);
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Player data saved");
    return 0;
}

/* Load all player data */
int player_manager_load_all(player_manager_t* mgr) {
    if (!mgr) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Loading player data...");

    /* Load whitelist */
    char whitelist_path[512];
    orbisjvm_get_app_data_dir(whitelist_path, sizeof(whitelist_path));
    strncat(whitelist_path, "/whitelist.txt", sizeof(whitelist_path) - strlen(whitelist_path) - 1);

    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, whitelist_path, ORBISJVM_O_RDONLY, 0);
    if (ret == 0) {
        char line[256];
        size_t read;
        while ((ret = orbisjvm_file_read(file, line, sizeof(line) - 1, &read)) == 0 && read > 0) {
            line[read] = '\0';
            char* nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            if (line[0] && line[0] != '#') {
                player_manager_whitelist_add(mgr, line);
            }
        }
        orbisjvm_file_close(file);
    }

    /* Load ops */
    char ops_path[512];
    orbisjvm_get_app_data_dir(ops_path, sizeof(ops_path));
    strncat(ops_path, "/ops.txt", sizeof(ops_path) - strlen(ops_path) - 1);

    ret = orbisjvm_file_open(&file, ops_path, ORBISJVM_O_RDONLY, 0);
    if (ret == 0) {
        char line[256];
        size_t read;
        while ((ret = orbisjvm_file_read(file, line, sizeof(line) - 1, &read)) == 0 && read > 0) {
            line[read] = '\0';
            char* nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            if (line[0] && line[0] != '#') {
                /* Add to ops without executing server command */
                orbisjvm_mutex_lock(&mgr->mutex);
                if (mgr->ops_count < mgr->ops_capacity) {
                    mgr->ops[mgr->ops_count++] = strdup(line);
                }
                orbisjvm_mutex_unlock(&mgr->mutex);
            }
        }
        orbisjvm_file_close(file);
    }

    /* Load bans */
    char bans_path[512];
    orbisjvm_get_app_data_dir(bans_path, sizeof(bans_path));
    strncat(bans_path, "/bans.txt", sizeof(bans_path) - strlen(bans_path) - 1);

    ret = orbisjvm_file_open(&file, bans_path, ORBISJVM_O_RDONLY, 0);
    if (ret == 0) {
        char line[512];
        size_t read;
        while ((ret = orbisjvm_file_read(file, line, sizeof(line) - 1, &read)) == 0 && read > 0) {
            line[read] = '\0';
            char* nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            if (line[0] && line[0] != '#') {
                char* name = strtok(line, "|");
                char* uuid = strtok(NULL, "|");
                char* banned_at_str = strtok(NULL, "|");
                char* banned_by = strtok(NULL, "|");
                char* reason = strtok(NULL, "|");

                if (name) {
                    orbisjvm_mutex_lock(&mgr->mutex);
                    if (mgr->bans_count < mgr->bans_capacity) {
                        ban_entry_t* ban = &mgr->bans[mgr->bans_count++];
                        strncpy(ban->name, name, sizeof(ban->name) - 1);
                        strncpy(ban->uuid, uuid ? uuid : "", sizeof(ban->uuid) - 1);
                        ban->banned_at = banned_at_str ? atoll(banned_at_str) : 0;
                        strncpy(ban->banned_by, banned_by ? banned_by : "unknown", sizeof(ban->banned_by) - 1);
                        strncpy(ban->reason, reason ? reason : "No reason given", sizeof(ban->reason) - 1);
                    }
                    orbisjvm_mutex_unlock(&mgr->mutex);
                }
            }
        }
        orbisjvm_file_close(file);
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Player data loaded (whitelist: %d, ops: %d, bans: %d)",
                      mgr->whitelist_count, mgr->ops_count, mgr->bans_count);
    return 0;
}

/* Get player stats */
int player_manager_get_stats(player_manager_t* mgr, player_stats_t* stats) {
    if (!mgr || !stats) return -1;

    orbisjvm_mutex_lock(&mgr->mutex);

    stats->total_players = mgr->player_count;
    stats->online_players = 0;
    for (int i = 0; i < mgr->player_count; i++) {
        if (mgr->players[i].online) stats->online_players++;
    }
    stats->max_players = 20; /* From server config */
    stats->banned_count = mgr->bans_count;
    stats->whitelisted_count = mgr->whitelist_count;
    stats->op_count = mgr->ops_count;
    stats->total_playtime = 0; /* Would need tracking */

    orbisjvm_mutex_unlock(&mgr->mutex);
    return 0;
}

/* Refresh player list */
void player_manager_refresh(player_manager_t* mgr) {
    if (!mgr || !mgr->server) return;

    /* Query server for player list */
    /* server_manager_execute_command(mgr->server, "list"); */

    /* In real implementation, parse server output for player list */
}

/* Set join callback */
void player_manager_set_join_callback(player_manager_t* mgr, player_join_callback_t cb, void* user_data) {
    if (!mgr) return;
    mgr->join_callback = cb;
    mgr->join_callback_data = user_data;
}

/* Set leave callback */
void player_manager_set_leave_callback(player_manager_t* mgr, player_leave_callback_t cb, void* user_data) {
    if (!mgr) return;
    mgr->leave_callback = cb;
    mgr->leave_callback_data = user_data;
}

/* Set chat callback */
void player_manager_set_chat_callback(player_manager_t* mgr, player_chat_callback_t cb, void* user_data) {
    if (!mgr) return;
    mgr->chat_callback = cb;
    mgr->chat_callback_data = user_data;
}