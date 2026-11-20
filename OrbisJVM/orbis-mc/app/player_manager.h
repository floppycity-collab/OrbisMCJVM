/**
 * @file player_manager.h
 * @brief OrbisMC Player Manager
 *
 * Manages player connections, whitelist, ops, bans, and player data.
 */

#ifndef ORBISMC_PLAYER_MANAGER_H
#define ORBISMC_PLAYER_MANAGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct server_manager server_manager_t;

/* Player info */
typedef struct {
    char name[64];
    char uuid[64];
    char ip[64];
    int ping;
    uint64_t join_time;
    uint64_t last_seen;
    bool online;
    bool op;
    bool whitelisted;
    bool banned;
    char ban_reason[256];
    int gamemode;
    double x, y, z;
    float yaw, pitch;
    int health;
    int food_level;
} player_info_t;

/* Player manager */
typedef struct player_manager player_manager_t;

/* Create player manager */
player_manager_t* player_manager_create(server_manager_t* server);

/* Destroy player manager */
void player_manager_destroy(player_manager_t* mgr);

/* Get online players */
int player_manager_get_online_players(player_manager_t* mgr, player_info_t* players, int max_players, int* count);

/* Get player by name */
int player_manager_get_player(player_manager_t* mgr, const char* name, player_info_t* info);

/* Kick player */
int player_manager_kick(player_manager_t* mgr, const char* name, const char* reason);

/* Ban player */
int player_manager_ban(player_manager_t* mgr, const char* name, const char* reason);

/* Unban player */
int player_manager_unban(player_manager_t* mgr, const char* name);

/* Whitelist player */
int player_manager_whitelist_add(player_manager_t* mgr, const char* name);

/* Remove from whitelist */
int player_manager_whitelist_remove(player_manager_t* mgr, const char* name);

/* Check if whitelisted */
bool player_manager_is_whitelisted(player_manager_t* mgr, const char* name);

/* Set operator status */
int player_manager_set_op(player_manager_t* mgr, const char* name, bool op);

/* Check if operator */
bool player_manager_is_op(player_manager_t* mgr, const char* name);

/* Get banned players list */
int player_manager_get_banned(player_manager_t* mgr, player_info_t* players, int max_players, int* count);

/* Get whitelisted players list */
int player_manager_get_whitelisted(player_manager_t* mgr, player_info_t* players, int max_players, int* count);

/* Get operators list */
int player_manager_get_ops(player_manager_t* mgr, player_info_t* players, int max_players, int* count);

/* Teleport player */
int player_manager_teleport(player_manager_t* mgr, const char* name, double x, double y, double z);

/* Set player gamemode */
int player_manager_set_gamemode(player_manager_t* mgr, const char* name, int gamemode);

/* Give item to player */
int player_manager_give_item(player_manager_t* mgr, const char* name, const char* item, int count);

/* Send message to player */
int player_manager_send_message(player_manager_t* mgr, const char* name, const char* message);

/* Broadcast message to all players */
int player_manager_broadcast(player_manager_t* mgr, const char* message);

/* Save player data */
int player_manager_save_all(player_manager_t* mgr);

/* Load player data */
int player_manager_load_all(player_manager_t* mgr);

/* Player statistics */
typedef struct {
    int total_players;
    int online_players;
    int max_players;
    int banned_count;
    int whitelisted_count;
    int op_count;
    uint64_t total_playtime;
} player_stats_t;

int player_manager_get_stats(player_manager_t* mgr, player_stats_t* stats);

/* Refresh player list from server */
void player_manager_refresh(player_manager_t* mgr);

/* Callbacks */
typedef void (*player_join_callback_t)(const char* name, void* user_data);
typedef void (*player_leave_callback_t)(const char* name, void* user_data);
typedef void (*player_chat_callback_t)(const char* name, const char* message, void* user_data);

void player_manager_set_join_callback(player_manager_t* mgr, player_join_callback_t cb, void* user_data);
void player_manager_set_leave_callback(player_manager_t* mgr, player_leave_callback_t cb, void* user_data);
void player_manager_set_chat_callback(player_manager_t* mgr, player_chat_callback_t cb, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* ORBISMC_PLAYER_MANAGER_H */