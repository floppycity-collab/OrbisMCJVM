/**
 * @file ui.c
 * @brief OrbisMC UI Implementation
 *
 * PS4-native UI for server management using simple framebuffer rendering.
 */

#include "ui.h"
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/time.h>
#include <orbisjvm/filesystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* UI context structure */
struct ui_context {
    server_manager_t* server;
    console_t* console;
    player_manager_t* player_mgr;
    config_manager_t* config_mgr;

    ui_screen_t current_screen;
    ui_screen_t previous_screen;

    /* Framebuffer */
    void* framebuffer;
    int fb_width;
    int fb_height;
    int fb_pitch;
    int fb_bpp;

    /* Input state */
    bool buttons[17];
    bool prev_buttons[17];
    int analog_lx, analog_ly;
    int analog_rx, analog_ry;

    /* Selected item indices for menus */
    int selected_index[UI_SCREEN_COUNT];

    /* Scroll offsets */
    int scroll_offset[UI_SCREEN_COUNT];

    /* Animation */
    float screen_transition;
    bool transitioning;

    /* Notifications */
    char notification_text[256];
    uint32_t notification_color;
    uint64_t notification_start;
    int notification_duration;

    /* Font rendering (placeholder - would use GNM or FreeType) */
    void* font_data;
};

/* Create UI */
ui_context_t* ui_create(server_manager_t* server, console_t* console, player_manager_t* player_mgr, config_manager_t* config_mgr) {
    ui_context_t* ui = orbisjvm_malloc(sizeof(ui_context_t), ORBISJVM_MEM_ZERO);
    if (!ui) return NULL;

    ui->server = server;
    ui->console = console;
    ui->player_mgr = player_mgr;
    ui->config_mgr = config_mgr;

    ui->current_screen = UI_SCREEN_DASHBOARD;
    ui->previous_screen = UI_SCREEN_DASHBOARD;

    /* Initialize framebuffer (would be set up via GNM in real implementation) */
    ui->fb_width = 1920;
    ui->fb_height = 1080;
    ui->fb_pitch = ui->fb_width * 4;
    ui->fb_bpp = 32;
    ui->framebuffer = orbisjvm_malloc(ui->fb_width * ui->fb_height * 4, ORBISJVM_MEM_ALIGNED);

    /* Initialize input */
    memset(ui->buttons, 0, sizeof(ui->buttons));
    memset(ui->prev_buttons, 0, sizeof(ui->prev_buttons));
    ui->analog_lx = 128;
    ui->analog_ly = 128;
    ui->analog_rx = 128;
    ui->analog_ry = 128;

    /* Initialize selection indices */
    memset(ui->selected_index, 0, sizeof(ui->selected_index));
    memset(ui->scroll_offset, 0, sizeof(ui->scroll_offset));

    /* Animation */
    ui->screen_transition = 1.0f;
    ui->transitioning = false;

    /* Notification */
    ui->notification_text[0] = '\0';
    ui->notification_color = UI_COLOR_GREEN;
    ui->notification_start = 0;
    ui->notification_duration = 0;

    /* Font (placeholder) */
    ui->font_data = NULL;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "UI created (%dx%d)", ui->fb_width, ui->fb_height);
    return ui;
}

/* Destroy UI */
void ui_destroy(ui_context_t* ui) {
    if (!ui) return;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Destroying UI...");

    if (ui->framebuffer) {
        orbisjvm_free(ui->framebuffer);
    }
    if (ui->font_data) {
        orbisjvm_free(ui->font_data);
    }

    orbisjvm_free(ui);
}

/* Run UI frame */
void ui_frame(ui_context_t* ui, uint64_t delta_ms) {
    if (!ui) return;

    /* Update animation */
    if (ui->transitioning) {
        ui->screen_transition += (float)delta_ms / 300.0f; /* 300ms transition */
        if (ui->screen_transition >= 1.0f) {
            ui->screen_transition = 1.0f;
            ui->transitioning = false;
        }
    }

    /* Check notification timeout */
    if (ui->notification_text[0] != '\0') {
        uint64_t elapsed = orbisjvm_get_monotonic_ms() - ui->notification_start;
        if (elapsed >= (uint64_t)ui->notification_duration) {
            ui->notification_text[0] = '\0';
        }
    }

    /* Render current screen */
    /* Clear framebuffer */
    if (ui->framebuffer) {
        uint32_t* fb = (uint32_t*)ui->framebuffer;
        for (int i = 0; i < ui->fb_width * ui->fb_height; i++) {
            fb[i] = UI_COLOR_BG;
        }
    }

    switch (ui->current_screen) {
        case UI_SCREEN_DASHBOARD:
            ui_render_dashboard(ui);
            break;
        case UI_SCREEN_SERVER_CONTROL:
            ui_render_server_control(ui);
            break;
        case UI_SCREEN_WORLD_MANAGEMENT:
            ui_render_world_management(ui);
            break;
        case UI_SCREEN_CONFIGURATION:
            ui_render_configuration(ui);
            break;
        case UI_SCREEN_CONSOLE:
            ui_render_console(ui);
            break;
        case UI_SCREEN_PLAYER_MANAGEMENT:
            ui_render_player_management(ui);
            break;
        case UI_SCREEN_SETTINGS:
            ui_render_settings(ui);
            break;
        case UI_SCREEN_ABOUT:
            ui_render_about(ui);
            break;
    }

    /* Draw notification if active */
    if (ui->notification_text[0] != '\0') {
        uint64_t elapsed = orbisjvm_get_monotonic_ms() - ui->notification_start;
        float alpha = 1.0f - (float)elapsed / (float)ui->notification_duration;
        if (alpha < 0.0f) alpha = 0.0f;
        uint32_t color = (ui->notification_color & 0x00FFFFFF) | ((uint32_t)(alpha * 255) << 24);
        ui_draw_panel(ui->fb_width / 2 - 200, 50, 400, 60, "");
        ui_draw_text_centered(ui->fb_width / 2, 70, 400, ui->notification_text, color, UI_FONT_NORMAL);
    }

    /* Present framebuffer (would swap buffers in real implementation) */
    /* For now, just log that frame completed */
}

/* Handle input */
void ui_handle_input(ui_context_t* ui, int button, bool pressed) {
    if (!ui || button < 0 || button >= 17) return;

    ui->buttons[button] = pressed;

    /* Handle button press events (not held) */
    if (pressed && !ui->prev_buttons[button]) {
        switch (button) {
            case UI_BUTTON_CIRCLE:
                /* Back/Close */
                if (ui->current_screen != UI_SCREEN_DASHBOARD) {
                    ui_set_screen(ui, UI_SCREEN_DASHBOARD);
                }
                break;
            case UI_BUTTON_OPTIONS:
                /* Open settings */
                ui_set_screen(ui, UI_SCREEN_SETTINGS);
                break;
            case UI_BUTTON_TRIANGLE:
                /* Context menu / Refresh */
                break;
            case UI_BUTTON_SQUARE:
                /* Quick action */
                break;
            case UI_BUTTON_DPAD_UP:
                if (ui->selected_index[ui->current_screen] > 0) {
                    ui->selected_index[ui->current_screen]--;
                }
                break;
            case UI_BUTTON_DPAD_DOWN:
                ui->selected_index[ui->current_screen]++;
                break;
            case UI_BUTTON_DPAD_LEFT:
                break;
            case UI_BUTTON_DPAD_RIGHT:
                break;
            case UI_BUTTON_CROSS:
                /* Select/Activate */
                ui_handle_selection(ui);
                break;
        }
    }

    ui->prev_buttons[button] = pressed;
}

/* Handle selection based on current screen */
static void ui_handle_selection(ui_context_t* ui) {
    int idx = ui->selected_index[ui->current_screen];

    switch (ui->current_screen) {
        case UI_SCREEN_DASHBOARD:
            switch (idx) {
                case 0: ui_set_screen(ui, UI_SCREEN_SERVER_CONTROL); break;
                case 1: ui_set_screen(ui, UI_SCREEN_WORLD_MANAGEMENT); break;
                case 2: ui_set_screen(ui, UI_SCREEN_PLAYER_MANAGEMENT); break;
                case 3: ui_set_screen(ui, UI_SCREEN_CONFIGURATION); break;
                case 4: ui_set_screen(ui, UI_SCREEN_CONSOLE); break;
                case 5: ui_set_screen(ui, UI_SCREEN_SETTINGS); break;
            }
            break;
        case UI_SCREEN_SERVER_CONTROL:
            if (ui->server) {
                server_state_t state = server_manager_get_state(ui->server);
                switch (idx) {
                    case 0:
                        if (state == SERVER_STATE_STOPPED || state == SERVER_STATE_CRASHED) {
                            server_manager_start(ui->server);
                            ui_show_notification("Server starting...", UI_COLOR_GREEN, 3000);
                        }
                        break;
                    case 1:
                        if (state == SERVER_STATE_RUNNING) {
                            server_manager_stop(ui->server, 30000);
                            ui_show_notification("Server stopping...", UI_COLOR_YELLOW, 3000);
                        }
                        break;
                    case 2:
                        if (state == SERVER_STATE_RUNNING) {
                            server_manager_restart(ui->server);
                            ui_show_notification("Server restarting...", UI_COLOR_BLUE, 3000);
                        }
                        break;
                    case 3:
                        if (state == SERVER_STATE_RUNNING) {
                            server_manager_kill(ui->server);
                            ui_show_notification("Server force killed!", UI_COLOR_RED, 3000);
                        }
                        break;
                }
            }
            break;
        case UI_SCREEN_WORLD_MANAGEMENT:
            break;
        case UI_SCREEN_PLAYER_MANAGEMENT:
            break;
    }
}

/* Set current screen */
void ui_set_screen(ui_context_t* ui, ui_screen_t screen) {
    if (!ui) return;

    if (screen != ui->current_screen) {
        ui->previous_screen = ui->current_screen;
        ui->current_screen = screen;
        ui->screen_transition = 0.0f;
        ui->transitioning = true;
        ui->selected_index[screen] = 0;
        ui->scroll_offset[screen] = 0;
    }
}

/* Get current screen */
ui_screen_t ui_get_screen(const ui_context_t* ui) {
    return ui ? ui->current_screen : UI_SCREEN_DASHBOARD;
}

/* Show notification */
void ui_show_notification(const char* message, uint32_t color, int duration_ms) {
    /* This would be called from various places - for now just log */
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Notification: %s", message);
}

/* ============ Drawing Helpers ============ */

void ui_draw_rect(int x, int y, int w, int h, uint32_t color) {
    /* Placeholder - would draw to framebuffer */
    (void)x; (void)y; (void)w; (void)h; (void)color;
}

void ui_draw_rect_rounded(int x, int y, int w, int h, int radius, uint32_t color) {
    (void)x; (void)y; (void)w; (void)h; (void)radius; (void)color;
}

void ui_draw_text(int x, int y, const char* text, uint32_t color, int font_size) {
    (void)x; (void)y; (void)text; (void)color; (void)font_size;
}

void ui_draw_text_centered(int x, int y, int w, const char* text, uint32_t color, int font_size) {
    (void)x; (void)y; (void)w; (void)text; (void)color; (void)font_size;
}

void ui_draw_button(int x, int y, int w, int h, const char* label, bool selected, bool disabled) {
    (void)x; (void)y; (void)w; (void)h; (void)label; (void)selected; (void)disabled;
}

void ui_draw_progress_bar(int x, int y, int w, int h, float progress, uint32_t bg_color, uint32_t fg_color) {
    (void)x; (void)y; (void)w; (void)h; (void)progress; (void)bg_color; (void)fg_color;
}

void ui_draw_panel(int x, int y, int w, int h, const char* title) {
    (void)x; (void)y; (void)w; (void)h; (void)title;
}

/* Get input state */
void ui_get_input_state(ui_input_state_t* state) {
    if (!state) return;
    memset(state, 0, sizeof(ui_input_state_t));
}

/* ============ Screen Renderers ============ */

void ui_render_dashboard(ui_context_t* ui) {
    int x = UI_MARGIN;
    int y = UI_MARGIN;
    int w = ui->fb_width - 2 * UI_MARGIN;
    int h = ui->fb_height - 2 * UI_MARGIN;

    /* Title */
    ui_draw_text(x + 20, y + 20, "OrbisMC", UI_COLOR_HIGHLIGHT, UI_FONT_TITLE);
    ui_draw_text(x + 20, y + 60, "PS4 Minecraft Server Manager", UI_COLOR_TEXT_DIM, UI_FONT_NORMAL);

    /* Server status panel */
    int panel_w = (w - 3 * UI_SPACING) / 4;
    int panel_h = 140;

    if (ui->server) {
        server_state_t state = server_manager_get_state(ui->server);
        server_stats_t stats;
        server_manager_get_stats(ui->server, &stats);

        const char* status_text[] = {"STOPPED", "STARTING", "RUNNING", "STOPPING", "CRASHED", "ERROR"};
        uint32_t status_color[] = {UI_COLOR_RED, UI_COLOR_YELLOW, UI_COLOR_GREEN, UI_COLOR_YELLOW, UI_COLOR_RED, UI_COLOR_RED};

        ui_draw_panel(x + 20, y + 100, panel_w, panel_h, "SERVER STATUS");
        ui_draw_text(x + 30, y + 130, "State:", UI_COLOR_TEXT, UI_FONT_NORMAL);
        ui_draw_text(x + 100, y + 130, status_text[state], status_color[state], UI_FONT_NORMAL);

        char uptime_str[64];
        snprintf(uptime_str, sizeof(uptime_str), "Uptime: %llu ms", stats.uptime_ms);
        ui_draw_text(x + 30, y + 160, uptime_str, UI_COLOR_TEXT, UI_FONT_SMALL);

        char players_str[64];
        snprintf(players_str, sizeof(players_str), "Players: %d/%d", stats.player_count, stats.max_players);
        ui_draw_text(x + 30, y + 180, players_str, UI_COLOR_TEXT, UI_FONT_SMALL);

        char tps_str[64];
        snprintf(tps_str, sizeof(tps_str), "TPS: %.1f", stats.tps);
        ui_draw_text(x + 30, y + 200, tps_str, UI_COLOR_TEXT, UI_FONT_SMALL);
    } else {
        ui_draw_panel(x + 20, y + 100, panel_w, panel_h, "SERVER STATUS");
        ui_draw_text(x + 30, y + 130, "Not Connected", UI_COLOR_RED, UI_FONT_NORMAL);
    }

    /* System info panel */
    ui_draw_panel(x + 40 + panel_w, y + 100, panel_w, panel_h, "SYSTEM INFO");
    orbisjvm_platform_info_t platform;
    orbisjvm_get_platform_info(&platform);
    ui_draw_text(x + 50 + panel_w, y + 130, platform.os_name, UI_COLOR_TEXT, UI_FONT_SMALL);
    char cpu_str[64];
    snprintf(cpu_str, sizeof(cpu_str), "CPU: %s (%u cores)", platform.cpu.vendor, platform.cpu.core_count);
    ui_draw_text(x + 50 + panel_w, y + 160, cpu_str, UI_COLOR_TEXT, UI_FONT_SMALL);
    char mem_str[64];
    snprintf(mem_str, sizeof(mem_str), "RAM: %llu MB / %llu MB",
             platform.memory.available_physical / (1024*1024),
             platform.memory.total_physical / (1024*1024));
    ui_draw_text(x + 50 + panel_w, y + 190, mem_str, UI_COLOR_TEXT, UI_FONT_SMALL);

    /* Network panel */
    ui_draw_panel(x + 60 + 2 * panel_w, y + 100, panel_w, panel_h, "NETWORK");
    orbisjvm_net_stats_t net_stats;
    orbisjvm_get_net_stats(&net_stats);
    ui_draw_text(x + 70 + 2 * panel_w, y + 130, "TCP Connections:", UI_COLOR_TEXT, UI_FONT_SMALL);
    char conn_str[64];
    snprintf(conn_str, sizeof(conn_str), "Current: %u", net_stats.current_sockets);
    ui_draw_text(x + 70 + 2 * panel_w, y + 160, conn_str, UI_COLOR_TEXT, UI_FONT_SMALL);
    char bw_str[64];
    snprintf(bw_str, sizeof(bw_str), "RX: %llu KB  TX: %llu KB",
             net_stats.bytes_received / 1024, net_stats.bytes_sent / 1024);
    ui_draw_text(x + 70 + 2 * panel_w, y + 190, bw_str, UI_COLOR_TEXT, UI_FONT_SMALL);

    /* Quick actions panel */
    ui_draw_panel(x + 80 + 3 * panel_w, y + 100, panel_w, panel_h, "QUICK ACTIONS");
    const char* actions[] = {"Server Control", "World Management", "Player Management", "Configuration", "Console", "Settings"};
    for (int i = 0; i < 6; i++) {
        bool selected = (ui->selected_index[UI_SCREEN_DASHBOARD] == i);
        ui_draw_button(x + 90 + 3 * panel_w, y + 130 + i * 45, panel_w - 20, 35, actions[i], selected, false);
    }

    /* Version info */
    ui_draw_text(x + 20, ui->fb_height - 40, "OrbisMC v0.1.0 | OrbisJVM v0.1.0 | Press OPTIONS for Settings", UI_COLOR_TEXT_DIM, UI_FONT_SMALL);
}

void ui_render_server_control(ui_context_t* ui) {
    int x = UI_MARGIN;
    int y = UI_MARGIN;

    ui_draw_text(x + 20, y + 20, "Server Control", UI_COLOR_HIGHLIGHT, UI_FONT_TITLE);

    if (!ui->server) {
        ui_draw_text(x + 20, y + 60, "No server configured", UI_COLOR_RED, UI_FONT_NORMAL);
        return;
    }

    server_state_t state = server_manager_get_state(ui->server);
    server_stats_t stats;
    server_manager_get_stats(ui->server, &stats);

    const char* status_text[] = {"STOPPED", "STARTING", "RUNNING", "STOPPING", "CRASHED", "ERROR"};
    uint32_t status_color[] = {UI_COLOR_RED, UI_COLOR_YELLOW, UI_COLOR_GREEN, UI_COLOR_YELLOW, UI_COLOR_RED, UI_COLOR_RED};

    /* Status display */
    ui_draw_panel(x + 20, y + 60, 600, 200, "SERVER STATUS");
    ui_draw_text(x + 40, y + 90, "State:", UI_COLOR_TEXT, UI_FONT_LARGE);
    ui_draw_text(x + 150, y + 90, status_text[state], status_color[state], UI_FONT_LARGE);

    ui_draw_text(x + 40, y + 130, "Uptime:", UI_COLOR_TEXT, UI_FONT_NORMAL);
    char uptime_str[64];
    snprintf(uptime_str, sizeof(uptime_str), "%.1f seconds", stats.uptime_ms / 1000.0);
    ui_draw_text(x + 150, y + 130, uptime_str, UI_COLOR_TEXT, UI_FONT_NORMAL);

    ui_draw_text(x + 40, y + 160, "Players:", UI_COLOR_TEXT, UI_FONT_NORMAL);
    char players_str[64];
    snprintf(players_str, sizeof(players_str), "%d / %d", stats.player_count, stats.max_players);
    ui_draw_text(x + 150, y + 160, players_str, UI_COLOR_TEXT, UI_FONT_NORMAL);

    /* Control buttons */
    const char* buttons[] = {"Start Server", "Stop Server", "Restart Server", "Force Kill", "Execute Command", "View Logs"};
    bool disabled[] = {
        state != SERVER_STATE_STOPPED && state != SERVER_STATE_CRASHED,
        state != SERVER_STATE_RUNNING,
        state != SERVER_STATE_RUNNING,
        state != SERVER_STATE_RUNNING,
        state != SERVER_STATE_RUNNING,
        state != SERVER_STATE_RUNNING
    };

    for (int i = 0; i < 6; i++) {
        bool selected = (ui->selected_index[UI_SCREEN_SERVER_CONTROL] == i);
        ui_draw_button(x + 40, y + 300 + i * 60, 500, 50, buttons[i], selected, disabled[i]);
    }

    /* Server info */
    ui_draw_panel(x + 650, y + 60, 500, 400, "SERVER INFO");
    ui_draw_text(x + 670, y + 90, "Configuration:", UI_COLOR_HIGHLIGHT, UI_FONT_NORMAL);
    char config_str[256];
    snprintf(config_str, sizeof(config_str), "Java: %s", "java");
    ui_draw_text(x + 670, y + 120, config_str, UI_COLOR_TEXT, UI_FONT_SMALL);
    snprintf(config_str, sizeof(config_str), "Jar: %s", "server.jar");
    ui_draw_text(x + 670, y + 150, config_str, UI_COLOR_TEXT, UI_FONT_SMALL);
    snprintf(config_str, sizeof(config_str), "Heap: %zu MB / %zu MB", 1024, 3072);
    ui_draw_text(x + 670, y + 180, config_str, UI_COLOR_TEXT, UI_FONT_SMALL);
    snprintf(config_str, sizeof(config_str), "Port: %d", 25565);
    ui_draw_text(x + 670, y + 210, config_str, UI_COLOR_TEXT, UI_FONT_SMALL);
}

void ui_render_world_management(ui_context_t* ui) {
    int x = UI_MARGIN;
    int y = UI_MARGIN;

    ui_draw_text(x + 20, y + 20, "World Management", UI_COLOR_HIGHLIGHT, UI_FONT_TITLE);

    /* World list */
    ui_draw_panel(x + 20, y + 60, 600, 500, "WORLDS");

    /* Would list worlds from world manager */
    ui_draw_text(x + 40, y + 90, "No worlds found", UI_COLOR_TEXT_DIM, UI_FONT_NORMAL);
    ui_draw_text(x + 40, y + 130, "Press CROSS to create new world", UI_COLOR_TEXT_DIM, UI_FONT_SMALL);

    /* Actions */
    const char* actions[] = {"Create World", "Import World", "Export World", "Backup Selected", "Delete Selected", "Validate World"};
    for (int i = 0; i < 6; i++) {
        bool selected = (ui->selected_index[UI_SCREEN_WORLD_MANAGEMENT] == i);
        ui_draw_button(x + 650, y + 60 + i * 60, 500, 50, actions[i], selected, false);
    }
}

void ui_render_configuration(ui_context_t* ui) {
    int x = UI_MARGIN;
    int y = UI_MARGIN;

    ui_draw_text(x + 20, y + 20, "Configuration", UI_COLOR_HIGHLIGHT, UI_FONT_TITLE);

    /* Config sections */
    const char* sections[] = {"Server Settings", "Java Settings", "Network", "Remote API", "Authentication", "Auto-Backup", "Logging", "Advanced"};
    for (int i = 0; i < 8; i++) {
        bool selected = (ui->selected_index[UI_SCREEN_CONFIGURATION] == i);
        ui_draw_button(x + 20, y + 60 + i * 55, 1130, 50, sections[i], selected, false);
    }
}

void ui_render_console(ui_context_t* ui) {
    int x = UI_MARGIN;
    int y = UI_MARGIN;

    ui_draw_text(x + 20, y + 20, "Console", UI_COLOR_HIGHLIGHT, UI_FONT_TITLE);

    /* Console output area */
    ui_draw_panel(x + 20, y + 60, 1130, 500, "OUTPUT");

    if (ui->console) {
        char* lines[50];
        int count = 0;
        console_get_lines(ui->console, lines, 50, &count);

        for (int i = 0; i < count; i++) {
            ui_draw_text(x + 40, y + 90 + i * 18, lines[i], UI_COLOR_TEXT, UI_FONT_SMALL);
        }
    } else {
        ui_draw_text(x + 40, y + 90, "Console not available", UI_COLOR_TEXT_DIM, UI_FONT_NORMAL);
    }

    /* Input area */
    ui_draw_panel(x + 20, y + 580, 1130, 60, "INPUT");
    ui_draw_text(x + 40, y + 600, "> ", UI_COLOR_HIGHLIGHT, UI_FONT_NORMAL);
}

void ui_render_player_management(ui_context_t* ui) {
    int x = UI_MARGIN;
    int y = UI_MARGIN;

    ui_draw_text(x + 20, y + 20, "Player Management", UI_COLOR_HIGHLIGHT, UI_FONT_TITLE);

    /* Online players */
    ui_draw_panel(x + 20, y + 60, 600, 500, "ONLINE PLAYERS");

    if (ui->player_mgr) {
        player_info_t players[20];
        int count = 0;
        player_manager_get_online_players(ui->player_mgr, players, 20, &count);

        for (int i = 0; i < count; i++) {
            char player_line[256];
            snprintf(player_line, sizeof(player_line), "%s %s [Ping: %dms]%s%s",
                     players[i].name,
                     players[i].op ? "[OP]" : "",
                     players[i].ping,
                     players[i].whitelisted ? " [WL]" : "",
                     players[i].banned ? " [BANNED]" : "");
            ui_draw_text(x + 40, y + 90 + i * 25, player_line, UI_COLOR_TEXT, UI_FONT_SMALL);
        }

        if (count == 0) {
            ui_draw_text(x + 40, y + 90, "No players online", UI_COLOR_TEXT_DIM, UI_FONT_NORMAL);
        }
    }

    /* Player actions */
    const char* actions[] = {"Kick Selected", "Ban Selected", "Unban", "OP/Deop", "Teleport", "Give Item", "Whitelist Add", "Whitelist Remove"};
    for (int i = 0; i < 8; i++) {
        bool selected = (ui->selected_index[UI_SCREEN_PLAYER_MANAGEMENT] == i);
        ui_draw_button(x + 650, y + 60 + i * 60, 500, 50, actions[i], selected, false);
    }
}

void ui_render_settings(ui_context_t* ui) {
    int x = UI_MARGIN;
    int y = UI_MARGIN;

    ui_draw_text(x + 20, y + 20, "Settings", UI_COLOR_HIGHLIGHT, UI_FONT_TITLE);

    const char* settings[] = {"Display", "Audio", "Network", "Storage", "Security", "Updates", "Developer", "Reset to Defaults"};
    for (int i = 0; i < 8; i++) {
        bool selected = (ui->selected_index[UI_SCREEN_SETTINGS] == i);
        ui_draw_button(x + 20, y + 60 + i * 60, 500, 50, settings[i], selected, false);
    }

    /* Current settings display */
    ui_draw_panel(x + 550, y + 60, 600, 500, "CURRENT SETTINGS");
    ui_draw_text(x + 570, y + 90, "Language: English", UI_COLOR_TEXT, UI_FONT_SMALL);
    ui_draw_text(x + 570, y + 120, "Theme: Dark", UI_COLOR_TEXT, UI_FONT_SMALL);
    ui_draw_text(x + 570, y + 150, "Auto-start Server: Off", UI_COLOR_TEXT, UI_FONT_SMALL);
    ui_draw_text(x + 570, y + 180, "Remote API: Enabled", UI_COLOR_TEXT, UI_FONT_SMALL);
    ui_draw_text(x + 570, y + 210, "TLS: Disabled", UI_COLOR_TEXT, UI_FONT_SMALL);
    ui_draw_text(x + 570, y + 240, "Auto-backup: Disabled", UI_COLOR_TEXT, UI_FONT_SMALL);
    ui_draw_text(x + 570, y + 270, "Log Level: INFO", UI_COLOR_TEXT, UI_FONT_SMALL);
}

void ui_render_about(ui_context_t* ui) {
    int x = UI_MARGIN;
    int y = UI_MARGIN;

    ui_draw_text(x + 20, y + 20, "About OrbisMC", UI_COLOR_HIGHLIGHT, UI_FONT_TITLE);

    ui_draw_panel(x + 20, y + 60, 1130, 500, "INFORMATION");

    ui_draw_text(x + 40, y + 90, "OrbisMC v0.1.0", UI_COLOR_HIGHLIGHT, UI_FONT_LARGE);
    ui_draw_text(x + 40, y + 130, "PS4 Minecraft Server Management Application", UI_COLOR_TEXT, UI_FONT_NORMAL);
    ui_draw_text(x + 40, y + 170, "Built on OrbisJVM v0.1.0", UI_COLOR_TEXT, UI_FONT_NORMAL);
    ui_draw_text(x + 40, y + 210, "Using OpenOrbis PS4 Toolchain", UI_COLOR_TEXT, UI_FONT_NORMAL);

    ui_draw_text(x + 40, y + 270, "Credits:", UI_COLOR_HIGHLIGHT, UI_FONT_NORMAL);
    ui_draw_text(x + 40, y + 310, "OpenOrbis Team - PS4 Toolchain & Research", UI_COLOR_TEXT, UI_FONT_SMALL);
    ui_draw_text(x + 40, y + 340, "Mojang Studios - Minecraft", UI_COLOR_TEXT, UI_FONT_SMALL);
    ui_draw_text(x + 40, y + 370, "PaperMC Team - Paper Server Software", UI_COLOR_TEXT, UI_FONT_SMALL);
    ui_draw_text(x + 40, y + 400, "OpenSSL Project - Cryptography", UI_COLOR_TEXT, UI_FONT_SMALL);

    ui_draw_text(x + 40, y + 450, "License: GPLv3", UI_COLOR_TEXT_DIM, UI_FONT_SMALL);
    ui_draw_text(x + 40, y + 480, "https://github.com/OpenOrbis", UI_COLOR_TEXT_DIM, UI_FONT_SMALL);
}