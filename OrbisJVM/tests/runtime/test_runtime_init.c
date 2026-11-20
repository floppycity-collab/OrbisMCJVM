/**
 * @file test_runtime_init.c
 * @brief Runtime initialization tests
 */

#include <orbisjvm/runtime.h>
#include <orbisjvm/diagnostics.h>
#include <stdio.h>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "ASSERT FAILED: %s: %s", #cond, msg); \
            return -1; \
        } \
    } while (0)

int test_init_shutdown(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing runtime init/shutdown...");

    int ret = orbisjvm_init();
    TEST_ASSERT(ret == 0, "init should succeed");

    ret = orbisjvm_init();
    TEST_ASSERT(ret == 0, "double init should succeed (idempotent)");

    orbisjvm_shutdown();

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "init/shutdown test passed");
    return 0;
}

int test_version_info(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing version info...");

    orbisjvm_version_info_t version;
    orbisjvm_get_version(&version);

    TEST_ASSERT(version.major == ORBISJVM_VERSION_MAJOR, "major version should match");
    TEST_ASSERT(version.minor == ORBISJVM_VERSION_MINOR, "minor version should match");
    TEST_ASSERT(version.patch == ORBISJVM_VERSION_PATCH, "patch version should match");
    TEST_ASSERT(version.version_string != NULL, "version string should not be NULL");
    TEST_ASSERT(version.build_date != NULL, "build date should not be NULL");
    TEST_ASSERT(version.build_commit != NULL, "build commit should not be NULL");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Version: %s", version.version_string);
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Build Date: %s", version.build_date);
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Build Commit: %s", version.build_commit);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "version info test passed");
    return 0;
}

int test_platform_info(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing platform info...");

    orbisjvm_platform_info_t info;
    int ret = orbisjvm_get_platform_info(&info);
    TEST_ASSERT(ret == 0, "get_platform_info should succeed");

    TEST_ASSERT(info.platform_name[0] != '\0', "platform name should not be empty");
    TEST_ASSERT(info.os_name[0] != '\0', "os name should not be empty");
    TEST_ASSERT(info.architecture[0] != '\0', "architecture should not be empty");
    TEST_ASSERT(info.memory.total_physical > 0, "total physical memory should be > 0");
    TEST_ASSERT(info.memory.page_size > 0, "page size should be > 0");
    TEST_ASSERT(info.cpu.core_count > 0, "cpu core count should be > 0");
    TEST_ASSERT(info.cpu.frequency_hz > 0, "cpu frequency should be > 0");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Platform: %s %s (%s)", info.os_name, info.os_version, info.architecture);
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "CPU: %s (%u cores, %llu Hz)", info.cpu.vendor, info.cpu.core_count, info.cpu.frequency_hz);
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Memory: %llu MB total", info.memory.total_physical / (1024*1024));

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "platform info test passed");
    return 0;
}

int test_capabilities(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing capability detection...");

    int has_mem = orbisjvm_has_capability(ORBISJVM_CAP_MEMORY_BASIC);
    TEST_ASSERT(has_mem == 1, "should have basic memory capability");

    int has_thread = orbisjvm_has_capability(ORBISJVM_CAP_THREADING_BASIC);
    TEST_ASSERT(has_thread == 1, "should have basic threading capability");

    int has_net = orbisjvm_has_capability(ORBISJVM_CAP_NETWORK_TCP);
    TEST_ASSERT(has_net == 1, "should have TCP networking capability");

    int has_fs = orbisjvm_has_capability(ORBISJVM_CAP_FILESYSTEM_BASIC);
    TEST_ASSERT(has_fs == 1, "should have basic filesystem capability");

    int has_time = orbisjvm_has_capability(ORBISJVM_CAP_TIME_MONOTONIC);
    TEST_ASSERT(has_time == 1, "should have monotonic time capability");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "capability detection test passed");
    return 0;
}

int main(void) {
    int failed = 0;

    if (test_init_shutdown() < 0) failed++;
    if (test_version_info() < 0) failed++;
    if (test_platform_info() < 0) failed++;
    if (test_capabilities() < 0) failed++;

    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All runtime init tests passed!");
    return 0;
}