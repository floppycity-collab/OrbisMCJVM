/**
 * @file launcher.c
 * @brief OrbisMC Launcher Implementation
 */

#include "launcher.h"
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/filesystem.h>
#include <orbisjvm/networking.h>
#include <orbisjvm/environment.h>
#include <orbisjvm/security.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

/* Launcher configuration file */
#define LAUNCHER_CONFIG_FILE "launcher.conf"
#define LAUNCHER_PROFILES_DIR "profiles"
#define LAUNCHER_LOGS_DIR "logs"
#define LAUNCHER_TEMP_DIR "tmp"

/* Default install paths */
#define DEFAULT_INSTALL_PATH "/opt/orbismc"
#define DEFAULT_DATA_PATH "/var/lib/orbismc"

/* Launcher state */
struct launcher_config_t {
    char install_path[512];
    char data_path[512];
    char config_path[512];
    char logs_path[512];
    bool auto_update;
    bool check_updates_on_start;
    char update_channel[32];
    char last_update_check[64];
};

static launcher_config_t g_launcher_config = {0};
static bool g_launcher_initialized = false;

/* Initialize launcher */
static int launcher_init(void) {
    if (g_launcher_initialized) return 0;

    /* Get paths */
    char app_data_dir[512];
    orbisjvm_get_app_data_dir(app_data_dir, sizeof(app_data_dir));

    strncpy(g_launcher_config.install_path, DEFAULT_INSTALL_PATH, sizeof(g_launcher_config.install_path) - 1);
    snprintf(g_launcher_config.data_path, sizeof(g_launcher_config.data_path), "%s", app_data_dir);
    snprintf(g_launcher_config.config_path, sizeof(g_launcher_config.config_path), "%s/%s", app_data_dir, LAUNCHER_CONFIG_FILE);
    snprintf(g_launcher_config.logs_path, sizeof(g_launcher_config.logs_path), "%s/%s", app_data_dir, LAUNCHER_LOGS_DIR);

    g_launcher_config.auto_update = false;
    g_launcher_config.check_updates_on_start = true;
    strncpy(g_launcher_config.update_channel, "stable", sizeof(g_launcher_config.update_channel) - 1);
    g_launcher_config.last_update_check[0] = '\0';

    /* Create directories */
    orbisjvm_dir_create_recursive(g_launcher_config.data_path, ORBISJVM_DEFAULT_DIR_MODE);
    orbisjvm_dir_create_recursive(g_launcher_config.logs_path, ORBISJVM_DEFAULT_DIR_MODE);
    char profiles_dir[512];
    snprintf(profiles_dir, sizeof(profiles_dir), "%s/%s", g_launcher_config.data_path, LAUNCHER_PROFILES_DIR);
    orbisjvm_dir_create_recursive(profiles_dir, ORBISJVM_DEFAULT_DIR_MODE);

    /* Load config */
    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, g_launcher_config.config_path, ORBISJVM_O_RDONLY, 0);
    if (ret == 0) {
        char line[512];
        size_t read;
        while ((ret = orbisjvm_file_read(file, line, sizeof(line) - 1, &read)) == 0 && read > 0) {
            line[read] = '\0';
            char* nl = strchr(line, '\n');
            if (nl) *nl = '\0';

            char* eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char* key = line;
                char* value = eq + 1;

                if (strcmp(key, "auto_update") == 0) g_launcher_config.auto_update = (strcmp(value, "true") == 0);
                else if (strcmp(key, "check_updates_on_start") == 0) g_launcher_config.check_updates_on_start = (strcmp(value, "true") == 0);
                else if (strcmp(key, "update_channel") == 0) strncpy(g_launcher_config.update_channel, value, sizeof(g_launcher_config.update_channel) - 1);
                else if (strcmp(key, "last_update_check") == 0) strncpy(g_launcher_config.last_update_check, value, sizeof(g_launcher_config.last_update_check) - 1);
            }
        }
        orbisjvm_file_close(file);
    }

    g_launcher_initialized = true;
    return 0;
}

/* Save launcher config */
static int launcher_save_config(void) {
    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, g_launcher_config.config_path, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret < 0) return ret;

    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);

    char buffer[2048];
    int len = snprintf(buffer, sizeof(buffer),
        "# OrbisMC Launcher Configuration\n"
        "# Generated: %s\n\n"
        "auto_update=%s\n"
        "check_updates_on_start=%s\n"
        "update_channel=%s\n"
        "last_update_check=%s\n",
        timestamp,
        g_launcher_config.auto_update ? "true" : "false",
        g_launcher_config.check_updates_on_start ? "true" : "false",
        g_launcher_config.update_channel,
        g_launcher_config.last_update_check);

    size_t written;
    orbisjvm_file_write(file, buffer, len, &written);
    orbisjvm_file_close(file);
    return 0;
}

/* Check if first-time setup is needed */
bool launcher_needs_setup(void) {
    launcher_init();

    /* Check if critical files exist */
    char server_dir[512];
    snprintf(server_dir, sizeof(server_dir), "%s/server", g_launcher_config.data_path);

    int ret = orbisjvm_path_is_dir(server_dir);
    if (ret != 1) return true;

    /* Check for server JAR */
    char server_jar[512];
    snprintf(server_jar, sizeof(server_jar), "%s/server.jar", server_dir);
    ret = orbisjvm_file_exists(server_jar);
    if (ret != 1) return true;

    /* Check for Java */
    char java_path[256];
    ret = launcher_detect_java(java_path, sizeof(java_path));
    if (ret != 0) return true;

    return false;
}

/* Run first-time setup wizard */
int launcher_run_setup(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Running first-time setup...");

    printf("\n=== OrbisMC First-Time Setup ===\n\n");

    /* 1. Detect Java */
    printf("Step 1: Detecting Java installation...\n");
    char java_path[256];
    int ret = launcher_detect_java(java_path, sizeof(java_path));
    if (ret == 0) {
        printf("  Found Java at: %s\n", java_path);
    } else {
        printf("  Java not found. Please install Java 17 or later.\n");
        printf("  On PS4, Java must be provided by OrbisJVM.\n");
        return -1;
    }

    /* 2. Create server directory */
    printf("\nStep 2: Creating server directory...\n");
    char server_dir[512];
    snprintf(server_dir, sizeof(server_dir), "%s/server", g_launcher_config.data_path);
    ret = orbisjvm_dir_create_recursive(server_dir, ORBISJVM_DEFAULT_DIR_MODE);
    if (ret < 0) {
        printf("  Failed to create server directory: %d\n", ret);
        return ret;
    }
    printf("  Created: %s\n", server_dir);

    /* 3. Download server JAR */
    printf("\nStep 3: Downloading server JAR...\n");
    printf("  Available versions:\n");
    printf("    1. Paper 1.20.4 (recommended)\n");
    printf("    2. Vanilla 1.20.4\n");
    printf("    3. Paper 1.19.4\n");
    printf("    4. Vanilla 1.19.4\n");
    printf("  Select version [1]: ");

    char input[64];
    fgets(input, sizeof(input), stdin);
    int choice = atoi(input);
    if (choice <= 0) choice = 1;

    const char* version = "1.20.4";
    const char* type = "paper";
    switch (choice) {
        case 1: version = "1.20.4"; type = "paper"; break;
        case 2: version = "1.20.4"; type = "vanilla"; break;
        case 3: version = "1.19.4"; type = "paper"; break;
        case 4: version = "1.19.4"; type = "vanilla"; break;
    }

    printf("  Downloading %s %s...\n", type, version);
    ret = launcher_download_server_jar(version, type);
    if (ret < 0) {
        printf("  Failed to download server JAR: %d\n", ret);
        return ret;
    }
    printf("  Download complete!\n");

    /* 4. Accept EULA */
    printf("\nStep 4: Accepting EULA...\n");
    char eula_path[512];
    snprintf(eula_path, sizeof(eula_path), "%s/eula.txt", server_dir);
    orbisjvm_file_t file;
    ret = orbisjvm_file_open(&file, eula_path, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret == 0) {
        const char* eula_content = "#By changing the setting below to TRUE you are indicating your agreement to our EULA (https://account.mojang.com/documents/minecraft_eula).\n"
                                   "eula=true\n";
        orbisjvm_file_write(file, eula_content, strlen(eula_content), NULL);
        orbisjvm_file_close(file);
        printf("  EULA accepted.\n");
    }

    /* 5. Create default server.properties */
    printf("\nStep 5: Creating default server.properties...\n");
    char props_path[512];
    snprintf(props_path, sizeof(props_path), "%s/server.properties", server_dir);
    ret = orbisjvm_file_open(&file, props_path, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret == 0) {
        const char* props =
            "# Minecraft server properties\n"
            "enable-jmx-monitoring=false\n"
            "rcon.port=25575\n"
            "level-seed=\n"
            "gamemode=survival\n"
            "enable-command-block=false\n"
            "enable-query=false\n"
            "generator-settings={}\n"
            "enforce-secure-profile=true\n"
            "level-name=world\n"
            "motd=OrbisMC PS4 Server\n"
            "query.port=25565\n"
            "pvp=true\n"
            "generate-structures=true\n"
            "difficulty=easy\n"
            "network-compression-threshold=256\n"
            "max-tick-time=60000\n"
            "require-resource-pack=false\n"
            "use-native-transport=true\n"
            "max-players=20\n"
            "online-mode=true\n"
            "enable-status=true\n"
            "allow-flight=false\n"
            "initial-disabled-packs=\n"
            "broadcast-rcon-to-ops=true\n"
            "view-distance=10\n"
            "server-ip=\n"
            "resource-pack-prompt=\n"
            "allow-nether=true\n"
            "server-port=25565\n"
            "enable-rcon=false\n"
            "sync-chunk-writes=true\n"
            "op-permission-level=4\n"
            "prevent-proxy-connections=false\n"
            "resource-pack=\n"
            "entity-broadcast-range-percentage=100\n"
            "simulation-distance=10\n"
            "rcon.password=\n"
            "player-idle-timeout=0\n"
            "debug=false\n"
            "force-gamemode=false\n"
            "rate-limit=0\n"
            "hardcore=false\n"
            "white-list=false\n"
            "broadcast-console-to-ops=true\n"
            "spawn-npcs=true\n"
            "spawn-animals=true\n"
            "function-permission-level=2\n"
            "level-type=minecraft\\:normal\n"
            "spawn-monsters=true\n"
            "enforce-whitelist=false\n"
            "spawn-protection=16\n"
            "resource-pack-sha1=\n"
            "max-world-size=29999984\n";
        orbisjvm_file_write(file, props, strlen(props), NULL);
        orbisjvm_file_close(file);
        printf("  server.properties created.\n");
    }

    /* 6. Save launcher config */
    printf("\nStep 6: Saving launcher configuration...\n");
    launcher_save_config();
    printf("  Configuration saved.\n");

    printf("\n=== Setup Complete! ===\n");
    printf("You can now run OrbisMC.\n\n");

    return 0;
}

/* Check for updates */
int launcher_check_updates(bool* update_available, char* latest_version, size_t size) {
    if (!update_available || !latest_version || size == 0) return -1;

    launcher_init();

    /* In a real implementation, this would check GitHub releases or a version API */
    /* For now, simulate no updates */
    *update_available = false;
    strncpy(latest_version, "0.1.0", size - 1);
    latest_version[size - 1] = '\0';

    /* Update last check time */
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    strftime(g_launcher_config.last_update_check, sizeof(g_launcher_config.last_update_check), "%Y-%m-%d %H:%M:%S", tm);
    launcher_save_config();

    return 0;
}

/* Apply update */
int launcher_apply_update(const char* version) {
    (void)version;
    orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "Auto-update not yet implemented");
    return -1;
}

/* Verify installation */
int launcher_verify_installation(char* error_buffer, size_t size) {
    launcher_init();

    /* Check install path */
    int ret = orbisjvm_path_is_dir(g_launcher_config.install_path);
    if (ret != 1) {
        if (error_buffer && size > 0) {
            snprintf(error_buffer, size, "Install directory not found: %s", g_launcher_config.install_path);
        }
        return -1;
    }

    /* Check data path */
    ret = orbisjvm_path_is_dir(g_launcher_config.data_path);
    if (ret != 1) {
        if (error_buffer && size > 0) {
            snprintf(error_buffer, size, "Data directory not found: %s", g_launcher_config.data_path);
        }
        return -1;
    }

    /* Check server JAR */
    char server_jar[512];
    snprintf(server_jar, sizeof(server_jar), "%s/server/server.jar", g_launcher_config.data_path);
    ret = orbisjvm_file_exists(server_jar);
    if (ret != 1) {
        if (error_buffer && size > 0) {
            snprintf(error_buffer, size, "Server JAR not found: %s", server_jar);
        }
        return -1;
    }

    /* Check Java */
    char java_path[256];
    ret = launcher_detect_java(java_path, sizeof(java_path));
    if (ret != 0) {
        if (error_buffer && size > 0) {
            snprintf(error_buffer, size, "Java not found");
        }
        return -1;
    }

    return 0;
}

/* Repair installation */
int launcher_repair_installation(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Repairing installation...");

    /* Re-create directories */
    orbisjvm_dir_create_recursive(g_launcher_config.data_path, ORBISJVM_DEFAULT_DIR_MODE);
    orbisjvm_dir_create_recursive(g_launcher_config.logs_path, ORBISJVM_DEFAULT_DIR_MODE);
    char profiles_dir[512];
    snprintf(profiles_dir, sizeof(profiles_dir), "%s/%s", g_launcher_config.data_path, LAUNCHER_PROFILES_DIR);
    orbisjvm_dir_create_recursive(profiles_dir, ORBISJVM_DEFAULT_DIR_MODE);

    /* Re-create server directory structure */
    char server_dir[512];
    snprintf(server_dir, sizeof(server_dir), "%s/server", g_launcher_config.data_path);
    orbisjvm_dir_create_recursive(server_dir, ORBISJVM_DEFAULT_DIR_MODE);

    /* Re-download server JAR if missing */
    char server_jar[512];
    snprintf(server_jar, sizeof(server_jar), "%s/server.jar", server_dir);
    if (orbisjvm_file_exists(server_jar) != 1) {
        launcher_download_server_jar("1.20.4", "paper");
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Installation repair complete");
    return 0;
}

/* Uninstall */
int launcher_uninstall(bool keep_data) {
    orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "Uninstalling OrbisMC...");

    if (!keep_data) {
        /* Remove all data */
        // orbisjvm_remove_recursive(g_launcher_config.data_path); /* Would need recursive remove */
    }

    /* Remove config */
    orbisjvm_file_remove(g_launcher_config.config_path);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Uninstall complete");
    return 0;
}

/* Get version */
int launcher_get_version(char* buffer, size_t size) {
    if (!buffer || size == 0) return -1;
    strncpy(buffer, "0.1.0", size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

/* Get install path */
int launcher_get_install_path(char* buffer, size_t size) {
    launcher_init();
    if (!buffer || size == 0) return -1;
    strncpy(buffer, g_launcher_config.install_path, size - 1);
    buffer[size - 1] = '\0';
    return 0;
}

/* Create shortcut */
int launcher_create_shortcut(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Creating shortcut...");
    /* On PS4, this would create a PKG or homebrew entry */
    return 0;
}

/* Remove shortcut */
int launcher_remove_shortcut(void) {
    return 0;
}

/* Backup user data */
int launcher_backup_user_data(const char* backup_path) {
    if (!backup_path) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Backing up user data to: %s", backup_path);

    char data_path[512];
    snprintf(data_path, sizeof(data_path), "%s", g_launcher_config.data_path);

    /* Use recursive copy */
    // orbisjvm_copy_recursive(data_path, backup_path);

    return 0;
}

/* Restore user data */
int launcher_restore_user_data(const char* backup_path) {
    if (!backup_path) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Restoring user data from: %s", backup_path);

    char data_path[512];
    snprintf(data_path, sizeof(data_path), "%s", g_launcher_config.data_path);

    // orbisjvm_copy_recursive(backup_path, data_path);

    return 0;
}

/* Migrate from older version */
int launcher_migrate_from_version(const char* from_version) {
    (void)from_version;
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Migration from %s not needed", from_version);
    return 0;
}

/* Get config */
int launcher_get_config(launcher_config_t* config) {
    launcher_init();
    if (!config) return -1;
    *config = g_launcher_config;
    return 0;
}

/* Set config */
int launcher_set_config(const launcher_config_t* config) {
    launcher_init();
    if (!config) return -1;
    g_launcher_config = *config;
    return launcher_save_config();
}

/* Detect Java */
int launcher_detect_java(char* java_path, size_t size) {
    if (!java_path || size == 0) return -1;

    /* Check common locations */
    const char* paths[] = {
        "/usr/bin/java",
        "/usr/local/bin/java",
        "/opt/java/bin/java",
        "/usr/lib/jvm/default-java/bin/java",
        "/usr/lib/jvm/java-17-openjdk/bin/java",
        "/usr/lib/jvm/java-21-openjdk/bin/java",
        getenv("JAVA_HOME")
    };

    for (int i = 0; i < 7; i++) {
        if (!paths[i]) continue;
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s/bin/java", paths[i]);
        if (i == 0 || i == 1) {
            strncpy(test_path, paths[i], sizeof(test_path) - 1);
        } else if (i == 6) {
            snprintf(test_path, sizeof(test_path), "%s/bin/java", paths[i]);
        }

        if (orbisjvm_file_exists(test_path) == 1) {
            /* Verify it's Java 17+ */
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "%s -version 2>&1", test_path);
            // Would execute and parse version

            strncpy(java_path, test_path, size - 1);
            java_path[size - 1] = '\0';
            return 0;
        }
    }

    return -1;
}

/* Install Java (placeholder) */
int launcher_install_java(const char* version) {
    (void)version;
    orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "Java installation not supported on PS4 - must use OrbisJVM");
    return -1;
}

/* Set Java path */
int launcher_set_java_path(const char* path) {
    if (!path) return -1;
    /* Save to config */
    return 0;
}

/* Download server JAR */
int launcher_download_server_jar(const char* version, const char* type) {
    if (!version || !type) return -1;

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Downloading %s server JAR version %s...", type, version);

    /* In real implementation, download from official sources:
     * - Paper: https://api.papermc.io/v2/projects/paper/versions/{version}/builds/{build}/downloads/paper-{version}-{build}.jar
     * - Vanilla: https://launcher.mojang.com/v1/objects/{hash}/server.jar
     * - Fabric: https://meta.fabricmc.net/v2/versions/loader/{version}/...
     */

    /* For now, create placeholder */
    char server_dir[512];
    snprintf(server_dir, sizeof(server_dir), "%s/server", g_launcher_config.data_path);
    orbisjvm_dir_create_recursive(server_dir, ORBISJVM_DEFAULT_DIR_MODE);

    char server_jar[512];
    snprintf(server_jar, sizeof(server_jar), "%s/server.jar", server_dir);

    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, server_jar, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret < 0) return ret;

    char placeholder[256];
    snprintf(placeholder, sizeof(placeholder), "PLACEHOLDER: %s %s server.jar - Replace with actual JAR", type, version);
    orbisjvm_file_write(file, placeholder, strlen(placeholder), NULL);
    orbisjvm_file_close(file);

    orbisjvm_log_warn(ORBISJVM_LOG_CAT_GENERAL, "Created placeholder server.jar - REPLACE WITH ACTUAL JAR!");
    return 0;
}

/* Verify server JAR */
int launcher_verify_server_jar(const char* path) {
    if (!path) return -1;
    return orbisjvm_file_exists(path) == 1 ? 0 : -1;
}

/* List available versions */
int launcher_list_available_versions(char** versions, int max_versions, int* count) {
    if (!versions || !count || max_versions <= 0) return -1;

    const char* available[] = {
        "1.20.4 (Paper)",
        "1.20.4 (Vanilla)",
        "1.19.4 (Paper)",
        "1.19.4 (Vanilla)",
        "1.18.2 (Paper)",
        "1.18.2 (Vanilla)"
    };

    int num = sizeof(available) / sizeof(available[0]);
    int to_copy = num < max_versions ? num : max_versions;

    for (int i = 0; i < to_copy; i++) {
        versions[i] = strdup(available[i]);
    }
    *count = to_copy;
    return 0;
}

/* Profile management */
int launcher_create_profile(const launcher_profile_t* profile) {
    if (!profile) return -1;

    char profiles_dir[512];
    snprintf(profiles_dir, sizeof(profiles_dir), "%s/%s", g_launcher_config.data_path, LAUNCHER_PROFILES_DIR);
    orbisjvm_dir_create_recursive(profiles_dir, ORBISJVM_DEFAULT_DIR_MODE);

    char profile_path[512];
    snprintf(profile_path, sizeof(profile_path), "%s/%s.profile", profiles_dir, profile->name);

    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, profile_path, ORBISJVM_O_WRONLY | ORBISJVM_O_CREAT | ORBISJVM_O_TRUNC, ORBISJVM_DEFAULT_FILE_MODE);
    if (ret < 0) return ret;

    char buffer[2048];
    int len = snprintf(buffer, sizeof(buffer),
        "name=%s\n"
        "java_path=%s\n"
        "server_jar=%s\n"
        "server_dir=%s\n"
        "jvm_args=%s\n"
        "server_args=%s\n",
        profile->name,
        profile->java_path,
        profile->server_jar,
        profile->server_dir,
        profile->jvm_args,
        profile->server_args);

    size_t written;
    orbisjvm_file_write(file, buffer, len, &written);
    orbisjvm_file_close(file);

    return 0;
}

int launcher_delete_profile(const char* name) {
    if (!name) return -1;

    char profiles_dir[512];
    snprintf(profiles_dir, sizeof(profiles_dir), "%s/%s", g_launcher_config.data_path, LAUNCHER_PROFILES_DIR);

    char profile_path[512];
    snprintf(profile_path, sizeof(profile_path), "%s/%s.profile", profiles_dir, name);

    return orbisjvm_file_remove(profile_path);
}

int launcher_get_profile(const char* name, launcher_profile_t* profile) {
    if (!name || !profile) return -1;

    char profiles_dir[512];
    snprintf(profiles_dir, sizeof(profiles_dir), "%s/%s", g_launcher_config.data_path, LAUNCHER_PROFILES_DIR);

    char profile_path[512];
    snprintf(profile_path, sizeof(profile_path), "%s/%s.profile", profiles_dir, name);

    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, profile_path, ORBISJVM_O_RDONLY, 0);
    if (ret < 0) return ret;

    char line[512];
    size_t read;
    while ((ret = orbisjvm_file_read(file, line, sizeof(line) - 1, &read)) == 0 && read > 0) {
        line[read] = '\0';
        char* nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        char* eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char* key = line;
            char* value = eq + 1;

            if (strcmp(key, "name") == 0) strncpy(profile->name, value, sizeof(profile->name) - 1);
            else if (strcmp(key, "java_path") == 0) strncpy(profile->java_path, value, sizeof(profile->java_path) - 1);
            else if (strcmp(key, "server_jar") == 0) strncpy(profile->server_jar, value, sizeof(profile->server_jar) - 1);
            else if (strcmp(key, "server_dir") == 0) strncpy(profile->server_dir, value, sizeof(profile->server_dir) - 1);
            else if (strcmp(key, "jvm_args") == 0) strncpy(profile->jvm_args, value, sizeof(profile->jvm_args) - 1);
            else if (strcmp(key, "server_args") == 0) strncpy(profile->server_args, value, sizeof(profile->server_args) - 1);
        }
    }

    orbisjvm_file_close(file);
    return 0;
}

int launcher_list_profiles(launcher_profile_t* profiles, int max_profiles, int* count) {
    if (!profiles || !count || max_profiles <= 0) return -1;

    char profiles_dir[512];
    snprintf(profiles_dir, sizeof(profiles_dir), "%s/%s", g_launcher_config.data_path, LAUNCHER_PROFILES_DIR);

    orbisjvm_dir_t dir;
    int ret = orbisjvm_dir_open(&dir, profiles_dir);
    if (ret < 0) {
        *count = 0;
        return 0;
    }

    int found = 0;
    orbisjvm_dirent_t entry;

    while (found < max_profiles) {
        ret = orbisjvm_dir_read(dir, &entry);
        if (ret == 1) break;
        if (ret < 0) continue;

        if (entry.type == ORBISJVM_FILE_TYPE_REGULAR) {
            /* Check if it's a .profile file */
            size_t len = strlen(entry.name);
            if (len > 7 && strcmp(entry.name + len - 7, ".profile") == 0) {
                char name[64];
                strncpy(name, entry.name, len - 7);
                name[len - 7] = '\0';

                launcher_get_profile(name, &profiles[found]);
                found++;
            }
        }
    }

    orbisjvm_dir_close(dir);
    *count = found;
    return 0;
}

int launcher_set_active_profile(const char* name) {
    (void)name;
    return 0;
}

/* Logs */
int launcher_get_logs(char* buffer, size_t size, int max_lines) {
    if (!buffer || size == 0) return -1;

    char log_path[512];
    snprintf(log_path, sizeof(log_path), "%s/orbismc.log", g_launcher_config.logs_path);

    orbisjvm_file_t file;
    int ret = orbisjvm_file_open(&file, log_path, ORBISJVM_O_RDONLY, 0);
    if (ret < 0) {
        buffer[0] = '\0';
        return 0;
    }

    char line[512];
    size_t read;
    int lines = 0;
    size_t pos = 0;

    /* Read from end - simplified: just read last lines */
    while ((ret = orbisjvm_file_read(file, line, sizeof(line) - 1, &read)) == 0 && read > 0 && lines < max_lines) {
        line[read] = '\0';
        if (pos + read + 1 < size) {
            memcpy(buffer + pos, line, read);
            pos += read;
            buffer[pos++] = '\n';
        }
        lines++;
    }

    buffer[pos] = '\0';
    orbisjvm_file_close(file);
    return 0;
}

int launcher_clear_logs(void) {
    char log_path[512];
    snprintf(log_path, sizeof(log_path), "%s/orbismc.log", g_launcher_config.logs_path);
    return orbisjvm_file_remove(log_path);
}