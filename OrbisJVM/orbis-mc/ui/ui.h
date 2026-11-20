/**
 * @file ui.h
 * @brief OrbisMC UI
 *
 * PS4-native UI for server management using GNM/gnmx or simple framebuffer.
 */

#ifndef ORBISMC_UI_H
#define ORBISMC_UI_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct server_manager server_manager_t;
typedef struct console console_t;
typedef struct player_manager player_manager_t;
typedef struct config_manager config_manager_t;

/* UI context */
typedef struct ui_context ui_context_t;

/* UI screen IDs */
typedef enum {
    UI_SCREEN_DASHBOARD = 0,
    UI_SCREEN_SERVER_CONTROL,
    UI_SCREEN_WORLD_MANAGEMENT,
    UI_SCREEN_CONFIGURATION,
    UI_SCREEN_CONSOLE,
    UI_SCREEN_PLAYER_MANAGEMENT,
    UI_SCREEN_SETTINGS,
    UI_SCREEN_ABOUT,
    UI_SCREEN_COUNT
} ui_screen_t;

/* Create UI */
ui_context_t* ui_create(server_manager_t* server, console_t* console, player_manager_t* player_mgr, config_manager_t* config);

/* Destroy UI */
void ui_destroy(ui_context_t* ui);

/* Run UI frame (call every frame) */
void ui_frame(ui_context_t* ui, uint64_t delta_ms);

/* Handle input event */
void ui_handle_input(ui_context_t* ui, int button, bool pressed);

/* Set current screen */
void ui_set_screen(ui_context_t* ui, ui_screen_t screen);

/* Get current screen */
ui_screen_t ui_get_screen(const ui_context_t* ui);

/* Render functions */
void ui_render_dashboard(ui_context_t* ui);
void ui_render_server_control(ui_context_t* ui);
void ui_render_world_management(ui_context_t* ui);
void ui_render_configuration(ui_context_t* ui);
void ui_render_console(ui_context_t* ui);
void ui_render_player_management(ui_context_t* ui);
void ui_render_settings(ui_context_t* ui);
void ui_render_about(ui_context_t* ui);

/* PS4 button constants */
#define UI_BUTTON_CROSS     0
#define UI_BUTTON_CIRCLE    1
#define UI_BUTTON_TRIANGLE  2
#define UI_BUTTON_SQUARE    3
#define UI_BUTTON_DPAD_UP   4
#define UI_BUTTON_DPAD_DOWN 5
#define UI_BUTTON_DPAD_LEFT 6
#define UI_BUTTON_DPAD_RIGHT 7
#define UI_BUTTON_L1        8
#define UI_BUTTON_R1        9
#define UI_BUTTON_L2        10
#define UI_BUTTON_R2        11
#define UI_BUTTON_L3        12
#define UI_BUTTON_R3        13
#define UI_BUTTON_OPTIONS   14
#define UI_BUTTON_TOUCHPAD  15
#define UI_BUTTON_PS        16

/* Color definitions (RGBA) */
#define UI_COLOR_BG         0x1A1A2EFF
#define UI_COLOR_PANEL      0x16213EFF
#define UI_COLOR_ACCENT     0x0F3460FF
#define UI_COLOR_HIGHLIGHT  0xE94560FF
#define UI_COLOR_TEXT       0xE8E8E8FF
#define UI_COLOR_TEXT_DIM   0x888888FF
#define UI_COLOR_GREEN      0x00D474FF
#define UI_COLOR_RED        0xFF4757FF
#define UI_COLOR_YELLOW     0xFFA502FF
#define UI_COLOR_BLUE       0x1E90FFFF

/* Font sizes */
#define UI_FONT_SMALL   14
#define UI_FONT_NORMAL  18
#define UI_FONT_LARGE   24
#define UI_FONT_TITLE   32

/* Layout constants */
#define UI_MARGIN       20
#define UI_PADDING      12
#define UI_SPACING      8
#define UI_BUTTON_HEIGHT 40
#define UI_PANEL_RADIUS  8

/* Draw helpers */
void ui_draw_rect(int x, int y, int w, int h, uint32_t color);
void ui_draw_rect_rounded(int x, int y, int w, int h, int radius, uint32_t color);
void ui_draw_text(int x, int y, const char* text, uint32_t color, int font_size);
void ui_draw_text_centered(int x, int y, int w, const char* text, uint32_t color, int font_size);
void ui_draw_button(int x, int y, int w, int h, const char* label, bool selected, bool disabled);
void ui_draw_progress_bar(int x, int y, int w, int h, float progress, uint32_t bg_color, uint32_t fg_color);
void ui_draw_panel(int x, int y, int w, int h, const char* title);

/* Input state */
typedef struct {
    bool buttons[17];
    int analog_lx, analog_ly;
    int analog_rx, analog_ry;
    int touch_x, touch_y;
    bool touch_active;
} ui_input_state_t;

void ui_get_input_state(ui_input_state_t* state);

/* Notifications */
void ui_show_notification(const char* message, uint32_t color, int duration_ms);

#ifdef __cplusplus
}
#endif

#endif /* ORBISMC_UI_H */