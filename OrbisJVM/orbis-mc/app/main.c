/**
 * @file main.c
 * @brief OrbisMC Main Entry Point
 *
 * PS4 Minecraft Server Management Application
 */

#include <orbisjvm/runtime.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/filesystem.h>
#include <orbisjvm/threading.h>
#include <orbisjvm/networking.h>
#include <orbisjvm/process.h>
#include <orbisjvm/environment.h>
#include <orbisjvm/time.h>

#include "server_manager.h"
#include "world_manager.h"
#include "config_manager.h"
#include "console.h"
#include "player_manager.h"
#include "remote_api.h"
#include "../ui/ui.h"
#include "../launcher/launcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

/* Global application state */
typedef struct {
    int running;
    orbisjvm_thread_t ui_thread;
    orbisjvm_thread_t server_thread;
    orbisjvm_thread_t remote_api_thread;
    orbisjvm_thread_t monitor_thread;
    server_manager_t* server;
    world_manager_t* world_mgr;
    config_manager_t* config_mgr;
    console_t* console;
    player_manager_t* player_mgr;
    remote_api_t* remote_api;
    ui_context_t* ui;
} orbis_mc_app_t;

static orbis_mc_app_t g_app = {0};

/* Signal handler for graceful shutdown */
static void signal_handler(int signum) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Received signal %d, shutting down...", signum);
    g_app.running = 0;
}

/* Initialize application subsystems */
static int app_init(orbis_mc_app_t* app) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Initializing OrbisMC...");

    /* Initialize OrbisJVM runtime */
    int ret = orbisjvm_init();
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to initialize OrbisJVM: %d", ret);
        return ret;
    }

    /* Set up signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize configuration manager */
    app->config_mgr = config_manager_create();
    if (!app->config_mgr) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to create config manager");
        return -1;
    }

    ret = config_manager_load(app->config_mgr);
    if (ret < 0) {
        orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "No existing config, creating defaults");
        config_manager_create_defaults(app->config_mgr);
        config_manager_save(app->config_mgr);
    }

    /* Initialize world manager */
    app->world_mgr = world_manager_create(app->config_mgr);
    if (!app->world_mgr) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to create world manager");
        return -1;
    }

    /* Initialize server manager */
    app->server = server_manager_create(app->config_mgr, app->world_mgr);
    if (!app->server) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to create server manager");
        return -1;
    }

    /* Initialize console */
    app->console = console_create();
    if (!app->console) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to create console");
        return -1;
    }

    /* Initialize player manager */
    app->player_mgr = player_manager_create(app->server);
    if (!app->player_mgr) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to create player manager");
        return -1;
    }

    /* Initialize remote API */
    app->remote_api = remote_api_create(app->config_mgr, app->server, app->console, app->player_mgr);
    if (!app->remote_api) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to create remote API");
        return -1;
    }

    /* Initialize UI */
    app->ui = ui_create(app->server, app->console, app->player_mgr, app->config_mgr);
    if (!app->ui) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to create UI");
        return -1;
    }

    app->running = 1;
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "OrbisMC initialized successfully");
    return 0;
}

/* Start background threads */
static int app_start_threads(orbis_mc_app_t* app) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Starting background threads...");

    /* UI thread (main thread on PS4) */
    /* UI runs on main thread, so we don't create a separate thread for it */

    /* Server monitor thread */
    int ret = orbisjvm_thread_create(&app->monitor_thread, NULL, server_monitor_thread, app);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to create monitor thread: %d", ret);
        return ret;
    }

    /* Remote API thread */
    ret = orbisjvm_thread_create(&app->remote_api_thread, NULL, remote_api_thread, app->remote_api);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to create remote API thread: %d", ret);
        return ret;
    }

    /* Auto-start server if configured */
    if (config_manager_get_auto_start(app->config_mgr)) {
        orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Auto-starting server...");
        server_manager_start(app->server);
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Background threads started");
    return 0;
}

/* Main application loop */
static void app_run(orbis_mc_app_t* app) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Starting main loop...");

    uint64_t last_frame = orbisjvm_get_monotonic_ms();
    const uint64_t target_frame_time = 16; /* ~60 FPS */

    while (app->running) {
        uint64_t now = orbisjvm_get_monotonic_ms();
        uint64_t delta = now - last_frame;

        /* Run UI frame */
        ui_frame(app->ui, delta);

        /* Process console input */
        console_process_input(app->console);

        /* Check server status */
        server_manager_update(app->server);

        /* Frame rate limiting */
        uint64_t frame_time = orbisjvm_get_monotonic_ms() - now;
        if (frame_time < target_frame_time) {
            orbisjvm_thread_sleep_ms(target_frame_time - frame_time);
        }

        last_frame = now;
    }
}

/* Cleanup application */
static void app_cleanup(orbis_mc_app_t* app) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Cleaning up OrbisMC...");

    app->running = 0;

    /* Stop server if running */
    if (app->server && server_manager_is_running(app->server)) {
        orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Stopping server...");
        server_manager_stop(app->server, 30000); /* 30 second timeout */
    }

    /* Wait for threads */
    if (app->monitor_thread) {
        orbisjvm_thread_join(app->monitor_thread, NULL);
    }
    if (app->remote_api_thread) {
        orbisjvm_thread_join(app->remote_api_thread, NULL);
    }

    /* Cleanup subsystems */
    if (app->remote_api) remote_api_destroy(app->remote_api);
    if (app->player_mgr) player_manager_destroy(app->player_mgr);
    if (app->console) console_destroy(app->console);
    if (app->server) server_manager_destroy(app->server);
    if (app->world_mgr) world_manager_destroy(app->world_mgr);
    if (app->config_mgr) config_manager_destroy(app->config_mgr);
    if (app->ui) ui_destroy(app->ui);

    /* Shutdown OrbisJVM */
    orbisjvm_shutdown();

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "OrbisMC shutdown complete");
}

/* Server monitor thread */
void* server_monitor_thread(void* arg) {
    orbis_mc_app_t* app = (orbis_mc_app_t*)arg;

    while (app->running) {
        if (app->server && server_manager_is_running(app->server)) {
            server_manager_check_health(app->server);

            /* Auto-restart on crash if configured */
            if (server_manager_has_crashed(app->server) &&
                config_manager_get_auto_restart(app->config_mgr)) {
                orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "Server crashed, auto-restarting...");
                server_manager_restart(app->server);
            }
        }

        orbisjvm_thread_sleep_ms(5000); /* Check every 5 seconds */
    }

    return NULL;
}

/* Print usage */
static void print_usage(const char* progname) {
    printf("OrbisMC - PS4 Minecraft Server Manager\n");
    printf("Usage: %s [options]\n", progname);
    printf("\nOptions:\n");
    printf("  --help, -h           Show this help\n");
    printf("  --version, -v        Show version\n");
    printf("  --config <path>      Config file path\n");
    printf("  --auto-start         Auto-start server on launch\n");
    printf("  --no-ui              Run headless (no UI)\n");
    printf("  --port <port>        Remote API port\n");
    printf("\n");
}

/* Main entry point */
int main(int argc, char** argv) {
    orbisjvm_set_args(argc, argv);

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            orbisjvm_version_info_t version;
            orbisjvm_get_version(&version);
            printf("OrbisMC %s\n", version.version_string);
            return 0;
        }
    }

    /* Run launcher first-time setup if needed */
    if (launcher_needs_setup()) {
        orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "First-time setup required");
        int ret = launcher_run_setup();
        if (ret < 0) {
            orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Setup failed");
            return 1;
        }
    }

    /* Initialize application */
    int ret = app_init(&g_app);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Application initialization failed: %d", ret);
        app_cleanup(&g_app);
        return 1;
    }

    /* Start background threads */
    ret = app_start_threads(&g_app);
    if (ret < 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "Failed to start threads: %d", ret);
        app_cleanup(&g_app);
        return 1;
    }

    /* Run main loop */
    app_run(&g_app);

    /* Cleanup */
    app_cleanup(&g_app);

    return 0;
}