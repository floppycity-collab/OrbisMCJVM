/**
 * @file test_runtime_config.c
 * @brief Runtime configuration tests
 */

#include <orbisjvm/runtime.h>
#include <orbisjvm/diagnostics.h>
#include <stdio.h>
#include <string.h>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "ASSERT FAILED: %s: %s", #cond, msg); \
            return -1; \
        } \
    } while (0)

int test_default_config(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing default config...");

    orbisjvm_config_t config;
    orbisjvm_get_default_config(&config);

    TEST_ASSERT(config.enable_diagnostics == 1, "diagnostics should be enabled by default");
    TEST_ASSERT(config.enable_memory_tracking == 1, "memory tracking should be enabled by default");
    TEST_ASSERT(config.enable_thread_tracking == 1, "thread tracking should be enabled by default");
    TEST_ASSERT(config.max_heap_size > 0, "max heap size should be > 0");
    TEST_ASSERT(config.initial_heap_size > 0, "initial heap size should be > 0");
    TEST_ASSERT(config.initial_heap_size <= config.max_heap_size, "initial should be <= max");
    TEST_ASSERT(config.temp_dir != NULL, "temp dir should be set");
    TEST_ASSERT(config.config_dir != NULL, "config dir should be set");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Default config: max_heap=%zu, initial_heap=%zu",
                      config.max_heap_size, config.initial_heap_size);

    /* Clean up */
    if (config.temp_dir) free((void*)config.temp_dir);
    if (config.config_dir) free((void*)config.config_dir);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "default config test passed");
    return 0;
}

int test_set_get_config(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing set/get config...");

    orbisjvm_config_t config = {0};
    config.enable_diagnostics = 1;
    config.enable_memory_tracking = 1;
    config.enable_thread_tracking = 0;
    config.max_heap_size = 256 * 1024 * 1024;
    config.initial_heap_size = 64 * 1024 * 1024;
    config.temp_dir = "/tmp/orbisjvm_test";
    config.config_dir = "/tmp/orbisjvm_config";

    int ret = orbisjvm_set_config(&config);
    TEST_ASSERT(ret == 0, "set_config should succeed");

    orbisjvm_config_t retrieved;
    orbisjvm_get_config(&retrieved);

    TEST_ASSERT(retrieved.enable_diagnostics == config.enable_diagnostics, "diagnostics should match");
    TEST_ASSERT(retrieved.enable_memory_tracking == config.enable_memory_tracking, "memory tracking should match");
    TEST_ASSERT(retrieved.enable_thread_tracking == config.enable_thread_tracking, "thread tracking should match");
    TEST_ASSERT(retrieved.max_heap_size == config.max_heap_size, "max heap should match");
    TEST_ASSERT(retrieved.initial_heap_size == config.initial_heap_size, "initial heap should match");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "set/get config test passed");
    return 0;
}

int main(void) {
    int failed = 0;

    if (test_default_config() < 0) failed++;
    if (test_set_get_config() < 0) failed++;

    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All runtime config tests passed!");
    return 0;
}