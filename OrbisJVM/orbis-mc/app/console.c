/**
 * @file console.c
 * @brief OrbisMC Console Implementation
 */

#include "console.h"
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/threading.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Console structure */
struct console {
    char** lines;
    int count;
    int capacity;
    int max_lines;
    orbisjvm_mutex_t mutex;

    char** history;
    int history_count;
    int history_capacity;

    char input_buffer[1024];
    int input_pos;

    char filter[256];
    bool show_timestamps;
    bool running;

    server_manager_t* server;
    console_command_callback_t command_callback;
    void* command_callback_data;
};

/* Create console */
console_t* console_create(void) {
    console_t* c = orbisjvm_malloc(sizeof(console_t), ORBISJVM_MEM_ZERO);
    if (!c) return NULL;

    c->capacity = 1000;
    c->lines = orbisjvm_malloc(c->capacity * sizeof(char*), ORBISJVM_MEM_ZERO);
    c->count = 0;

    c->history_capacity = 100;
    c->history = orbisjvm_malloc(c->history_capacity * sizeof(char*), ORBISJVM_MEM_ZERO);
    c->history_count = 0;

    c->input_pos = 0;
    c->filter[0] = '\0';
    c->show_timestamps = true;
    c->running = true;

    orbisjvm_mutex_create(&c->mutex, NULL);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Console created");
    return c;
}

/* Destroy console */
void console_destroy(console_t* c) {
    if (!c) return;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Destroying console...");

    c->running = false;

    /* Free lines */
    orbisjvm_mutex_lock(&c->mutex);
    for (int i = 0; i < c->count; i++) {
        orbisjvm_free(c->lines[i]);
    }
    orbisjvm_free(c->lines);

    for (int i = 0; i < c->history_count; i++) {
        orbisjvm_free(c->history[i]);
    }
    orbisjvm_free(c->history);
    orbisjvm_mutex_unlock(&c->mutex);

    orbisjvm_mutex_destroy(c->mutex);
    orbisjvm_free(c);
}

/* Attach to server */
void console_attach_server(console_t* c, server_manager_t* server) {
    if (!c) return;
    c->server = server;

    /* Set output callback on server */
    if (server) {
        /* server_manager_set_output_callback(server, console_server_output_callback, c); */
    }
}

/* Process input */
void console_process_input(console_t* c) {
    if (!c) return;

    /* In a real implementation, this would read from stdin or UI input */
    /* For now, it's a placeholder */
}

/* Add log line */
void console_add_line(console_t* c, const char* line) {
    if (!c || !line) return;

    /* Apply filter */
    if (c->filter[0] != '\0' && strstr(line, c->filter) == NULL) {
        return;
    }

    char timestamp[32] = "";
    if (c->show_timestamps) {
        time_t now = time(NULL);
        struct tm* tm = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "[%H:%M:%S] ", tm);
    }

    char* full_line = orbisjvm_malloc(strlen(timestamp) + strlen(line) + 1, ORBISJVM_MEM_ZERO);
    if (!full_line) return;
    strcpy(full_line, timestamp);
    strcat(full_line, line);

    orbisjvm_mutex_lock(&c->mutex);

    /* Resize if needed */
    if (c->count >= c->capacity) {
        /* Remove oldest line */
        orbisjvm_free(c->lines[0]);
        memmove(c->lines, c->lines + 1, (c->capacity - 1) * sizeof(char*));
        c->count--;
    }

    c->lines[c->count++] = full_line;

    /* Trim to max_lines */
    if (c->max_lines > 0 && c->count > c->max_lines) {
        int to_remove = c->count - c->max_lines;
        for (int i = 0; i < to_remove; i++) {
            orbisjvm_free(c->lines[i]);
        }
        memmove(c->lines, c->lines + to_remove, c->max_lines * sizeof(char*));
        c->count = c->max_lines;
    }

    orbisjvm_mutex_unlock(&c->mutex);
}

/* Get console lines */
int console_get_lines(console_t* c, char** lines, int max_lines, int* count) {
    if (!c || !lines || !count || max_lines <= 0) return -1;

    orbisjvm_mutex_lock(&c->mutex);
    int to_copy = c->count < max_lines ? c->count : max_lines;
    for (int i = 0; i < to_copy; i++) {
        lines[i] = c->lines[c->count - to_copy + i];
    }
    *count = to_copy;
    orbisjvm_mutex_unlock(&c->mutex);
    return 0;
}

/* Execute command */
int console_execute_command(console_t* c, const char* command) {
    if (!c || !command) return -1;

    /* Trim whitespace */
    while (*command && isspace(*command)) command++;
    if (*command == '\0') return 0;

    /* Add to history */
    orbisjvm_mutex_lock(&c->mutex);
    if (c->history_count >= c->history_capacity) {
        orbisjvm_free(c->history[0]);
        memmove(c->history, c->history + 1, (c->history_capacity - 1) * sizeof(char*));
        c->history_count--;
    }
    c->history[c->history_count++] = strdup(command);
    orbisjvm_mutex_unlock(&c->mutex);

    /* Log command */
    char log_line[1024];
    snprintf(log_line, sizeof(log_line), "> %s", command);
    console_add_line(c, log_line);

    /* Execute callback */
    if (c->command_callback) {
        return c->command_callback(command, c->command_callback_data);
    }

    /* Built-in commands */
    if (strcmp(command, "help") == 0) {
        console_add_line(c, "Available commands:");
        console_add_line(c, "  help        - Show this help");
        console_add_line(c, "  clear       - Clear console");
        console_add_line(c, "  history     - Show command history");
        console_add_line(c, "  filter <str> - Set log filter");
        console_add_line(c, "  timestamps  - Toggle timestamps");
        console_add_line(c, "  stop        - Stop server");
        console_add_line(c, "  restart     - Restart server");
        console_add_line(c, "  status      - Show server status");
        return 0;
    } else if (strcmp(command, "clear") == 0) {
        console_clear(c);
        return 0;
    } else if (strcmp(command, "history") == 0) {
        console_add_line(c, "Command history:");
        orbisjvm_mutex_lock(&c->mutex);
        for (int i = 0; i < c->history_count; i++) {
            char buf[256];
            snprintf(buf, sizeof(buf), "  %d: %s", i + 1, c->history[i]);
            console_add_line(c, buf);
        }
        orbisjvm_mutex_unlock(&c->mutex);
        return 0;
    } else if (strncmp(command, "filter ", 7) == 0) {
        console_set_filter(c, command + 7);
        console_add_line(c, "Filter set");
        return 0;
    } else if (strcmp(command, "timestamps") == 0) {
        c->show_timestamps = !c->show_timestamps;
        console_add_line(c, c->show_timestamps ? "Timestamps enabled" : "Timestamps disabled");
        return 0;
    } else if (strcmp(command, "stop") == 0) {
        if (c->server) {
            console_add_line(c, "Stopping server...");
            /* server_manager_stop(c->server, 30000); */
        } else {
            console_add_line(c, "No server attached");
        }
        return 0;
    } else if (strcmp(command, "restart") == 0) {
        if (c->server) {
            console_add_line(c, "Restarting server...");
            /* server_manager_restart(c->server); */
        } else {
            console_add_line(c, "No server attached");
        }
        return 0;
    } else if (strcmp(command, "status") == 0) {
        if (c->server) {
            /* server_stats_t stats; */
            /* server_manager_get_stats(c->server, &stats); */
            console_add_line(c, "Server: Running");
        } else {
            console_add_line(c, "Server: Not attached");
        }
        return 0;
    }

    console_add_line(c, "Unknown command. Type 'help' for available commands.");
    return -1;
}

/* Get command history */
int console_get_history(console_t* c, char** history, int max_history, int* count) {
    if (!c || !history || !count || max_history <= 0) return -1;

    orbisjvm_mutex_lock(&c->mutex);
    int to_copy = c->history_count < max_history ? c->history_count : max_history;
    for (int i = 0; i < to_copy; i++) {
        history[i] = c->history[c->history_count - to_copy + i];
    }
    *count = to_copy;
    orbisjvm_mutex_unlock(&c->mutex);
    return 0;
}

/* Clear console */
void console_clear(console_t* c) {
    if (!c) return;

    orbisjvm_mutex_lock(&c->mutex);
    for (int i = 0; i < c->count; i++) {
        orbisjvm_free(c->lines[i]);
    }
    c->count = 0;
    orbisjvm_mutex_unlock(&c->mutex);
}

/* Set filter */
void console_set_filter(console_t* c, const char* filter) {
    if (!c) return;
    strncpy(c->filter, filter ? filter : "", sizeof(c->filter) - 1);
    c->filter[sizeof(c->filter) - 1] = '\0';
}

/* Get filter */
const char* console_get_filter(const console_t* c) {
    if (!c) return "";
    return c->filter;
}

/* Set timestamps */
void console_set_timestamps(console_t* c, bool enabled) {
    if (!c) return;
    c->show_timestamps = enabled;
}

/* Set max lines */
void console_set_max_lines(console_t* c, int max_lines) {
    if (!c) return;
    c->max_lines = max_lines > 0 ? max_lines : 0;
}

/* Set command callback */
void console_set_command_callback(console_t* c, console_command_callback_t callback, void* user_data) {
    if (!c) return;
    c->command_callback = callback;
    c->command_callback_data = user_data;
}