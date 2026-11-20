/**
 * @file console.h
 * @brief OrbisMC Console
 *
 * Manages server console output, command input, and log display.
 */

#ifndef ORBISMC_CONSOLE_H
#define ORBISMC_CONSOLE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct server_manager server_manager_t;

/* Console */
typedef struct console console_t;

/* Create console */
console_t* console_create(void);

/* Destroy console */
void console_destroy(console_t* console);

/* Attach to server manager for output capture */
void console_attach_server(console_t* console, server_manager_t* server);

/* Process input (call regularly in main loop) */
void console_process_input(console_t* console);

/* Add log line to console */
void console_add_line(console_t* console, const char* line);

/* Get recent console lines */
int console_get_lines(console_t* console, char** lines, int max_lines, int* count);

/* Execute command */
int console_execute_command(console_t* console, const char* command);

/* Get command history */
int console_get_history(console_t* console, char** history, int max_history, int* count);

/* Clear console */
void console_clear(console_t* console);

/* Set log filter */
void console_set_filter(console_t* console, const char* filter);

/* Get filter */
const char* console_get_filter(const console_t* console);

/* Enable/disable timestamps */
void console_set_timestamps(console_t* console, bool enabled);

/* Set max lines in buffer */
void console_set_max_lines(console_t* console, int max_lines);

/* Callback for command execution */
typedef int (*console_command_callback_t)(const char* command, void* user_data);
void console_set_command_callback(console_t* console, console_command_callback_t callback, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* ORBISMC_CONSOLE_H */