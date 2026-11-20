/**
 * @file test_filesystem_path.c
 * @brief Path utility tests
 */

#include <orbisjvm/filesystem.h>
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

int test_path_join(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing path join...");

    char buffer[512];

    int ret = orbisjvm_path_join("/home/user", "documents", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_join should succeed");
    TEST_ASSERT(strcmp(buffer, "/home/user/documents") == 0, "joined path should be correct");

    ret = orbisjvm_path_join("/home/user/", "documents", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_join with trailing slash should succeed");
    TEST_ASSERT(strcmp(buffer, "/home/user/documents") == 0, "joined path should not have double slash");

    ret = orbisjvm_path_join("", "relative", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_join with empty base should succeed");
    TEST_ASSERT(strcmp(buffer, "relative") == 0, "joined path should be relative");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "path join test passed");
    return 0;
}

int test_path_dirname_basename(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing dirname/basename...");

    char buffer[512];

    int ret = orbisjvm_path_dirname("/home/user/file.txt", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_dirname should succeed");
    TEST_ASSERT(strcmp(buffer, "/home/user") == 0, "dirname should be /home/user");

    ret = orbisjvm_path_dirname("relative/file.txt", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_dirname for relative should succeed");
    TEST_ASSERT(strcmp(buffer, "relative") == 0, "dirname should be relative");

    ret = orbisjvm_path_dirname("/file.txt", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_dirname for root file should succeed");
    TEST_ASSERT(strcmp(buffer, "/") == 0, "dirname should be /");

    ret = orbisjvm_path_basename("/home/user/file.txt", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_basename should succeed");
    TEST_ASSERT(strcmp(buffer, "file.txt") == 0, "basename should be file.txt");

    ret = orbisjvm_path_basename("relative/file.txt", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_basename for relative should succeed");
    TEST_ASSERT(strcmp(buffer, "file.txt") == 0, "basename should be file.txt");

    ret = orbisjvm_path_basename("/home/user/", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_basename for directory should succeed");
    TEST_ASSERT(strcmp(buffer, "user") == 0, "basename should be user");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "dirname/basename test passed");
    return 0;
}

int test_path_extension(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing path extension...");

    char buffer[512];

    int ret = orbisjvm_path_extension("/home/user/file.txt", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_extension should succeed");
    TEST_ASSERT(strcmp(buffer, "txt") == 0, "extension should be txt");

    ret = orbisjvm_path_extension("/home/user/archive.tar.gz", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_extension for double extension should succeed");
    TEST_ASSERT(strcmp(buffer, "gz") == 0, "extension should be gz (last)");

    ret = orbisjvm_path_extension("/home/user/README", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_extension for no extension should succeed");
    TEST_ASSERT(buffer[0] == '\0', "extension should be empty");

    ret = orbisjvm_path_extension("/home/user/.hidden", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_extension for hidden file should succeed");
    TEST_ASSERT(strcmp(buffer, "hidden") == 0, "extension should be hidden");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "path extension test passed");
    return 0;
}

int test_path_normalize(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing path normalize...");

    char buffer[512];

    int ret = orbisjvm_path_normalize("/home/user/../user/./documents", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_normalize should succeed");
    TEST_ASSERT(strcmp(buffer, "/home/user/documents") == 0, "normalized path should be correct");

    ret = orbisjvm_path_normalize("./relative/../file.txt", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_normalize for relative should succeed");
    TEST_ASSERT(strcmp(buffer, "file.txt") == 0, "normalized relative path should be correct");

    ret = orbisjvm_path_normalize("/home//user///documents", buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "path_normalize for multiple slashes should succeed");
    TEST_ASSERT(strcmp(buffer, "/home/user/documents") == 0, "normalized path should collapse slashes");

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "path normalize test passed");
    return 0;
}

int test_get_app_dirs(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Testing app directory functions...");

    char buffer[512];

    int ret = orbisjvm_get_app_data_dir(buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "get_app_data_dir should succeed");
    TEST_ASSERT(buffer[0] != '\0', "data dir should not be empty");
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "App data dir: %s", buffer);

    ret = orbisjvm_get_app_cache_dir(buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "get_app_cache_dir should succeed");
    TEST_ASSERT(buffer[0] != '\0', "cache dir should not be empty");
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "App cache dir: %s", buffer);

    ret = orbisjvm_get_app_config_dir(buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "get_app_config_dir should succeed");
    TEST_ASSERT(buffer[0] != '\0', "config dir should not be empty");
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "App config dir: %s", buffer);

    ret = orbisjvm_get_temp_dir(buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "get_temp_dir should succeed");
    TEST_ASSERT(buffer[0] != '\0', "temp dir should not be empty");
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Temp dir: %s", buffer);

    ret = orbisjvm_get_cwd(buffer, sizeof(buffer));
    TEST_ASSERT(ret == 0, "get_cwd should succeed");
    TEST_ASSERT(buffer[0] != '\0', "cwd should not be empty");
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "CWD: %s", buffer);

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "app dirs test passed");
    return 0;
}

int main(void) {
    orbisjvm_init();

    int failed = 0;

    if (test_path_join() < 0) failed++;
    if (test_path_dirname_basename() < 0) failed++;
    if (test_path_extension() < 0) failed++;
    if (test_path_normalize() < 0) failed++;
    if (test_get_app_dirs() < 0) failed++;

    orbisjvm_shutdown();

    if (failed > 0) {
        orbisjvm_log_error(ORBISJVM_LOG_CAT_GENERAL, "%d test(s) failed", failed);
        return 1;
    }

    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "All filesystem path tests passed!");
    return 0;
}