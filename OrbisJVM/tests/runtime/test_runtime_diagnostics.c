/**
 * @file test_runtime_diagnostics.c
 * @brief Runtime diagnostics tests
 */

#include <orbisjvm/runtime.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/memory.h>
#include <orbisjvm/threading.h>
#include <orbisjvm/networking.h>
#include <stdio.h>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "ASSERT FAILED: %s: %s", #cond, msg); \
            return -1; \
        } \
    } while (0)

static int g_log_callback_count = 0;

void log_callback(const orbisjvm_log_entry_t* entry, void* user_data) {
    (void)user_data;
    g_log_callback_count++;
    orbisjvm_log_debug(ORBISJVM_LOG_CAT_GENERAL, "Log callback: level=%d cat=%u msg=%s",
                       entry->level, entry->category, entry->message);
}

int test_log_levels(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing log levels...");

    orbisjvm_diag_set_log_level(ORBISJVM_LOG_DEBUG);
    TEST_ASSERT(orbisjvm_diag_get_log_level() == ORBISJVM_LOG_DEBUG, "log level should be DEBUG");

    orbisjvm_diag_set_log_level(ORBISJVM_LOG_WARN);
    TEST_ASSERT(orbisjvm_diag_get_log_level() == ORBISJVM_LOG_WARN, "log level should be WARN");

    orbisjvm_diag_set_log_level(ORBISJVM_LOG_INFO);
    TEST_ASSERT(orbisjvm_diag_get_log_level() == ORBISJVM_LOG_INFO, "log level should be INFO");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "log levels test passed");
    return 0;
}

int test_log_categories(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing log categories...");

    orbisjvm_diag_set_categories(ORBISJVM_LOG_CAT_MEMORY | ORBISJVM_LOG_CAT_THREADING);
    uint32_t cats = orbisjvm_diag_get_categories();
    TEST_ASSERT((cats & ORBISJVM_LOG_CAT_MEMORY) != 0, "should have MEMORY category");
    TEST_ASSERT((cats & ORBISJVM_LOG_CAT_THREADING) != 0, "should have THREADING category");
    TEST_ASSERT((cats & ORBISJVM_LOG_CAT_NETWORK) == 0, "should not have NETWORK category");

    orbisjvm_diag_set_categories(ORBISJVM_LOG_CAT_ALL);
    TEST_ASSERT(orbisjvm_diag_get_categories() == ORBISJVM_LOG_CAT_ALL, "should have all categories");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "log categories test passed");
    return 0;
}

int test_log_callback(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing log callback...");

    g_log_callback_count = 0;
    int ret = orbisjvm_diag_register_callback(log_callback, NULL);
    TEST_ASSERT(ret == 0, "register_callback should succeed");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Test message 1");
    orbisjvm_log_warn(ORBISJVM_LOG_CAT_MEMORY, "Test message 2");
    orbisjvm_log_error(ORBISJVM_LOG_CAT_THREADING, "Test message 3");

    TEST_ASSERT(g_log_callback_count == 3, "callback should be called 3 times");

    ret = orbisjvm_diag_unregister_callback(log_callback);
    TEST_ASSERT(ret == 0, "unregister_callback should succeed");

    g_log_callback_count = 0;
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Test message after unregister");
    TEST_ASSERT(g_log_callback_count == 0, "callback should not be called after unregister");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "log callback test passed");
    return 0;
}

int test_profiling(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing profiling...");

    int profiling_events = 0;
    orbisjvm_profile_callback_t profile_cb = [](const orbisjvm_profile_event_t* event, void* user_data) {
        (void)user_data;
        int* count = (int*)user_data;
        (*count)++;
    };

    int ret = orbisjvm_profile_start(profile_cb, &profiling_events);
    TEST_ASSERT(ret == 0, "profile_start should succeed");

    orbisjvm_profile_enter("test_function", "test", NULL);
    orbisjvm_thread_sleep_ms(10);
    orbisjvm_profile_exit("test_function", "test", NULL);

    orbisjvm_profile_instant("instant_event", "test", NULL);

    ret = orbisjvm_profile_stop();
    TEST_ASSERT(ret == 0, "profile_stop should succeed");

    TEST_ASSERT(profiling_events >= 3, "should have at least 3 profile events");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "profiling test passed (events: %d)", profiling_events);
    return 0;
}

int test_diagnostic_report(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing diagnostic report generation...");

    const char* report_file = "/tmp/orbisjvm_test_report.txt";
    int ret = orbisjvm_generate_diagnostic_report(report_file);
    TEST_ASSERT(ret == 0, "generate_diagnostic_report should succeed");

    /* Verify file exists and has content */
    orbisjvm_file_t file;
    ret = orbisjvm_file_open(&file, report_file, ORBISJVM_O_RDONLY, 0);
    TEST_ASSERT(ret == 0, "report file should be readable");

    char buffer[1024];
    size_t read;
    ret = orbisjvm_file_read(file, buffer, sizeof(buffer) - 1, &read);
    TEST_ASSERT(ret == 0, "report file read should succeed");
    TEST_ASSERT(read > 0, "report should have content");

    buffer[read] = '\0';
    TEST_ASSERT(strstr(buffer, "OrbisJVM Diagnostic Report") != NULL, "report should contain header");
    TEST_ASSERT(strstr(buffer, "Version:") != NULL, "report should contain version");

    orbisjvm_file_close(file);
    orbisjvm_file_remove(report_file);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "diagnostic report test passed");
    return 0;
}

int main(void) {
    orbisjvm_init();

    int failed = 0;

    if (test_log_levels() < 0) failed++;
    if (test_log_categories() < 0) failed++;
    if (test_log_callback() < 0) failed++;
    if (test_profiling() < 0) failed++;
    if (test_diagnostic_report() < 0) failed++;

    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All runtime diagnostics tests passed!");
    return 0;
}